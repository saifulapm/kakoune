#include "syntax_tree.hh"

#include "changes.hh"
#include "client.hh"
#include "client_manager.hh"
#include "debug.hh"
#include "exception.hh"
#include "format.hh"
#include "hash_map.hh"
#include "language_registry.hh"
#include "meta.hh"

#include <chrono>
#include <fcntl.h>
#include <unistd.h>

namespace Kakoune
{

// Wall-clock parse budget. Tree-sitter's progress_callback fires periodically
// during a parse; returning true aborts it. A cancelled parse returns null,
// and callers keep the previous tree and retry. Same idea as Helix's
// PARSE_TIMEOUT (helix-core/src/syntax.rs); we use a tighter bound since the
// worker join in ~AsyncSyntaxTree blocks teardown for at most this long.
static constexpr std::chrono::milliseconds parse_time_budget{150};

namespace
{
struct ParseDeadline
{
    std::chrono::steady_clock::time_point deadline;
};

// tree-sitter only invokes this every OP_COUNT_PER_PARSER_CALLBACK_CHECK (100)
// parser operations, so a steady_clock read here is already well-amortised —
// no extra throttling needed.
bool parse_progress_callback(TSParseState* state)
{
    auto* dl = static_cast<ParseDeadline*>(state->payload);
    return std::chrono::steady_clock::now() >= dl->deadline;
}

// ts_parser_parse with a wall-clock budget. Returns null on timeout (parser
// state is left consistent for a later retry).
TSTree* parse_with_budget(TSParser* parser, const TSTree* old_tree, TSInput input)
{
    ParseDeadline dl;
    dl.deadline = std::chrono::steady_clock::now() + parse_time_budget;
    TSParseOptions options{};
    options.payload = &dl;
    options.progress_callback = parse_progress_callback;
    return ts_parser_parse_with_options(parser, const_cast<TSTree*>(old_tree),
                                        input, options);
}
}

void LineByteIndex::rebuild(const Buffer& buffer)
{
    const int count = (int)buffer.line_count();
    m_line_start_bytes.resize(count);
    uint32_t offset = 0;
    for (int i = 0; i < count; ++i)
    {
        m_line_start_bytes[i] = offset;
        offset += (uint32_t)(int)buffer[LineCount{i}].length();
    }
}

void LineByteIndex::rebuild_from_offsets(const Vector<uint32_t>& offsets)
{
    m_line_start_bytes.resize((int)offsets.size());
    for (int i = 0; i < (int)offsets.size(); ++i)
        m_line_start_bytes[i] = offsets[i];
}

uint32_t LineByteIndex::byte_offset(BufferCoord coord) const
{
    if (m_line_start_bytes.empty())
        return 0;
    int line = std::min((int)coord.line, (int)m_line_start_bytes.size() - 1);
    return m_line_start_bytes[line] + (uint32_t)(int)coord.column;
}

static const char* ts_input_read(void* payload, uint32_t byte_index,
                                 TSPoint position, uint32_t* bytes_read)
{
    const auto& buffer = *static_cast<const Buffer*>(payload);
    const auto line = LineCount{(int)position.row};

    if (line >= buffer.line_count())
    {
        *bytes_read = 0;
        return "";
    }

    const StringView line_content = buffer[line];
    const auto col = (int)position.column;

    if (col >= (int)line_content.length())
    {
        *bytes_read = 0;
        return "";
    }

    *bytes_read = (uint32_t)((int)line_content.length() - col);
    return line_content.data() + col;
}

InjectionLayer::~InjectionLayer()
{
    if (tree)
        ts_tree_delete(tree);
    if (parser)
        ts_parser_delete(parser);
}

InjectionLayer::InjectionLayer(InjectionLayer&& other) noexcept
    : parser(other.parser),
      tree(other.tree),
      language_name(std::move(other.language_name)),
      config(other.config),
      ranges(std::move(other.ranges)),
      content_node_id(other.content_node_id),
      combined(other.combined)
{
    other.parser = nullptr;
    other.tree = nullptr;
    other.config = nullptr;
    other.content_node_id = 0;
    other.combined = false;
}

InjectionLayer& InjectionLayer::operator=(InjectionLayer&& other) noexcept
{
    if (this != &other)
    {
        if (tree)
            ts_tree_delete(tree);
        if (parser)
            ts_parser_delete(parser);

        parser = other.parser;
        tree = other.tree;
        language_name = std::move(other.language_name);
        config = other.config;
        ranges = std::move(other.ranges);
        content_node_id = other.content_node_id;
        combined = other.combined;

        other.parser = nullptr;
        other.tree = nullptr;
        other.config = nullptr;
        other.content_node_id = 0;
        other.combined = false;
    }
    return *this;
}

void SyntaxTree::full_parse(const Buffer& buffer)
{
    m_byte_index.rebuild(buffer);

    TSInput input{};
    input.payload = const_cast<Buffer*>(&buffer);
    input.read = ts_input_read;
    input.encoding = TSInputEncodingUTF8;

    TSTree* parsed = parse_with_budget(m_parser, nullptr, input);
    if (parsed)
    {
        if (m_tree)
            ts_tree_delete(m_tree);
        m_tree = parsed;
        m_timestamp = buffer.timestamp();
    }
    // On timeout (parsed == null) keep the previous m_tree so highlighting
    // falls back to the last good tree; m_timestamp stays behind so the next
    // frame retries the parse.

    // Host subtrees were all regenerated, so content_node_id on any existing
    // layer is stale — by_node matches would return wrong old_trees. Drop
    // the layer trees (keep parsers for reuse). Pending edits are likewise
    // meaningless for this case since layers can't reach back through a
    // full reparse.
    for (auto& layer : m_injection_layers)
    {
        if (layer.tree)
        {
            ts_tree_delete(layer.tree);
            layer.tree = nullptr;
        }
        layer.content_node_id = 0;
    }
    m_pending_edits.clear();
}

void SyntaxTree::adopt_tree(TSTree* tree, LineByteIndex&& byte_index, size_t ts)
{
    if (not tree)
        return;
    if (m_tree)
        ts_tree_delete(m_tree);
    m_tree = tree;
    m_byte_index = std::move(byte_index);
    m_timestamp = ts;

    // The worker reparsed the whole host from scratch, so every host subtree
    // is fresh: existing layer content_node_ids are stale. Drop layer trees
    // (keep parsers for reuse) exactly like the from-scratch path in
    // full_parse, then let detect_injections rebuild from this tree.
    for (auto& layer : m_injection_layers)
    {
        if (layer.tree)
        {
            ts_tree_delete(layer.tree);
            layer.tree = nullptr;
        }
        layer.content_node_id = 0;
    }
    m_pending_edits.clear();
    // Force detect_injections (it early-returns when injection ts == m_timestamp).
    m_injection_timestamp = ts - 1;
}

SyntaxTree::SyntaxTree(const Buffer& buffer, const LanguageConfig* config)
    : m_language(config->language()),
      m_highlight_query(config->highlight_query()),
      m_language_name(config->name())
{
    m_parser = ts_parser_new();
    if (not m_parser)
        throw runtime_error("failed to create tree-sitter parser");

    if (not ts_parser_set_language(m_parser, m_language))
        throw runtime_error("failed to set tree-sitter language");

    full_parse(buffer);
}

SyntaxTree::SyntaxTree(DeferParse, const LanguageConfig* config)
    : m_language(config->language()),
      m_highlight_query(config->highlight_query()),
      m_language_name(config->name())
{
    m_parser = ts_parser_new();
    if (not m_parser)
        throw runtime_error("failed to create tree-sitter parser");
    if (not ts_parser_set_language(m_parser, m_language))
        throw runtime_error("failed to set tree-sitter language");
    // No initial parse: m_tree stays null until adopt_tree() supplies one.
}

const LanguageConfig* SyntaxTree::config() const
{
    if (not LanguageRegistry::has_instance() or m_language_name.empty())
        return nullptr;
    return LanguageRegistry::instance().get(m_language_name);
}

SyntaxTree::~SyntaxTree()
{
    if (m_tree)
        ts_tree_delete(m_tree);
    if (m_parser)
        ts_parser_delete(m_parser);
}

SyntaxTree::SyntaxTree(SyntaxTree&& other) noexcept
    : m_parser(other.m_parser),
      m_tree(other.m_tree),
      m_language(other.m_language),
      m_highlight_query(other.m_highlight_query),
      m_language_name(std::move(other.m_language_name)),
      m_byte_index(std::move(other.m_byte_index)),
      m_timestamp(other.m_timestamp),
      m_injection_timestamp(other.m_injection_timestamp),
      m_injection_layers(std::move(other.m_injection_layers)),
      m_pending_edits(std::move(other.m_pending_edits))
{
    other.m_parser = nullptr;
    other.m_tree = nullptr;
}

SyntaxTree& SyntaxTree::operator=(SyntaxTree&& other) noexcept
{
    if (this != &other)
    {
        if (m_tree)
            ts_tree_delete(m_tree);
        if (m_parser)
            ts_parser_delete(m_parser);

        m_parser = other.m_parser;
        m_tree = other.m_tree;
        m_language = other.m_language;
        m_highlight_query = other.m_highlight_query;
        m_language_name = std::move(other.m_language_name);
        m_byte_index = std::move(other.m_byte_index);
        m_timestamp = other.m_timestamp;
        m_injection_timestamp = other.m_injection_timestamp;
        m_injection_layers = std::move(other.m_injection_layers);
        m_pending_edits = std::move(other.m_pending_edits);

        other.m_parser = nullptr;
        other.m_tree = nullptr;
    }
    return *this;
}

void SyntaxTree::update(const Buffer& buffer)
{
    if (buffer.timestamp() == m_timestamp)
        return;

    if (not m_tree)
    {
        full_parse(buffer);
        return;
    }

    auto changes = buffer.changes_since(m_timestamp);

    // Undo/redo produces backward-sorted changes. Our incremental edit
    // logic uses m_byte_index which is only valid for forward-sorted
    // (normal editing) changes. If we detect any backward-sorted changes,
    // fall back to full reparse for correctness.
    {
        auto forward_end = forward_sorted_until(changes.begin(), changes.end());
        if (forward_end != changes.end())
        {
            // Full reparse — any buffered injection edits are meaningless
            // since we're rebuilding from scratch. detect_injections will
            // see an empty edit list and take the no-pool-match path.
            m_pending_edits.clear();
            full_parse(buffer);
            return;
        }
    }

    // Apply edits in REVERSE order (like Helix/tree-house). When applied
    // in reverse, each edit only affects byte positions to its LEFT, which
    // haven't been processed yet. This means the pre-change byte index
    // remains valid for all edits — no adjustment tracking needed.
    //
    // For byte offsets: use the OLD m_byte_index which reflects the tree's
    // current coordinate space. For Insert changes, new_end_byte needs the
    // byte length of the inserted content — we compute this from the change
    // coordinates since we know the exact line/column extent.

    m_pending_edits.reserve(m_pending_edits.size() + changes.size());
    for (int i = (int)changes.size() - 1; i >= 0; --i)
    {
        auto& change = changes[(size_t)i];

        TSInputEdit edit{};
        edit.start_byte = m_byte_index.byte_offset(change.begin);
        edit.start_point = {(uint32_t)(int)change.begin.line,
                            (uint32_t)(int)change.begin.column};

        if (change.type == Buffer::Change::Insert)
        {
            edit.old_end_byte = edit.start_byte;
            edit.old_end_point = edit.start_point;
            edit.new_end_point = {(uint32_t)(int)change.end.line,
                                  (uint32_t)(int)change.end.column};

            // Compute exact byte length of inserted content.
            // For single-line: column difference gives exact bytes.
            // For multi-line: sum the actual line lengths from the
            // final buffer. This works because reverse-order edits
            // to the RIGHT have already been applied to the tree,
            // but we only need the OLD byte index for start_byte
            // (which is to the LEFT and unaffected).
            if (change.begin.line == change.end.line)
            {
                edit.new_end_byte = edit.start_byte +
                    (uint32_t)((int)change.end.column - (int)change.begin.column);
            }
            else
            {
                // Multi-line insert: compute byte length by summing lines
                // from the FINAL buffer (the inserted content is there)
                uint32_t byte_len = 0;
                for (auto line = change.begin.line; line <= change.end.line
                     and line < buffer.line_count(); ++line)
                {
                    auto line_content = buffer[line];
                    if (line == change.begin.line)
                        byte_len += (uint32_t)((int)line_content.length() - (int)change.begin.column);
                    else if (line == change.end.line)
                        byte_len += (uint32_t)(int)change.end.column;
                    else
                        byte_len += (uint32_t)(int)line_content.length();
                }
                edit.new_end_byte = edit.start_byte + byte_len;
            }
        }
        else // Erase
        {
            edit.new_end_byte = edit.start_byte;
            edit.new_end_point = edit.start_point;
            edit.old_end_point = {(uint32_t)(int)change.end.line,
                                  (uint32_t)(int)change.end.column};

            // For erase: old_end_byte from the OLD byte index (exact)
            edit.old_end_byte = m_byte_index.byte_offset(change.end);
        }

        ts_tree_edit(m_tree, &edit);
        // Record for later replay onto pooled injection layer trees so they
        // land in the same post-edit byte coordinate space.
        m_pending_edits.push_back(edit);
    }

    // Rebuild byte index from the final buffer state
    m_byte_index.rebuild(buffer);

    // Incremental reparse with all edits applied
    TSInput input{};
    input.payload = const_cast<Buffer*>(&buffer);
    input.read = ts_input_read;
    input.encoding = TSInputEncodingUTF8;

    TSTree* new_tree = parse_with_budget(m_parser, m_tree, input);
    if (new_tree)
    {
        ts_tree_delete(m_tree);
        m_tree = new_tree;
        m_timestamp = buffer.timestamp();
    }
    // On timeout, m_tree already has the edits applied (ts_tree_edit above),
    // so it stays usable for approximate highlighting; m_timestamp lags so
    // detect_injections and the next update() retry the reparse.
}

static String node_text(TSNode node, const Buffer& buffer)
{
    const TSPoint start = ts_node_start_point(node);
    const TSPoint end = ts_node_end_point(node);
    String result;

    for (uint32_t row = start.row; row <= end.row and row < (uint32_t)(int)buffer.line_count(); ++row)
    {
        const StringView line = buffer[LineCount{(int)row}];
        const uint32_t col_start = (row == start.row) ? start.column : 0;
        const uint32_t col_end = (row == end.row) ? end.column : (uint32_t)(int)line.length();

        if (col_start < (uint32_t)(int)line.length() and col_start < col_end)
        {
            const uint32_t len = std::min(col_end, (uint32_t)(int)line.length()) - col_start;
            result += StringView{line.data() + col_start, (int)len};
        }
    }
    return result;
}

struct PendingInjection
{
    String language_name;
    Vector<TSRange, MemoryDomain::Highlight> ranges;
    bool combined;
    // Identity key for the host content node that produced this injection
    // (the first one, for combined injections). Used to match against the
    // pool of surviving layers from the previous pass.
    uintptr_t content_node_id = 0;
};

// Collect injection ranges from a tree using an injection query
static void collect_injections_from_tree(
    TSTree* tree,
    const LanguageConfig* cfg,
    const Buffer& buffer,
    Vector<PendingInjection>& pending)
{
    if (not cfg or not cfg->injection_query() or not tree)
        return;

    QueryCursorGuard cursor;
    if (not cursor.cursor)
        return;

    ts_query_cursor_set_match_limit(cursor, 256);
    ts_query_cursor_exec(cursor, cfg->injection_query(), ts_tree_root_node(tree));

    const uint32_t content_capture = cfg->injection_content_capture();
    const uint32_t language_capture = cfg->injection_language_capture();
    const auto& patterns = cfg->injection_patterns();
    const auto& inj_preds = cfg->injection_predicates();

    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match))
    {
        if (match.pattern_index < (uint32_t)inj_preds.size()
            and not inj_preds[(int)match.pattern_index].empty()
            and not predicates_match(inj_preds[(int)match.pattern_index], match, buffer))
            continue;

        TSNode content_node = {};
        bool has_content = false;
        String lang_name;

        for (uint16_t i = 0; i < match.capture_count; ++i)
        {
            const TSQueryCapture& cap = match.captures[i];
            if (cap.index == content_capture)
            {
                content_node = cap.node;
                has_content = true;
            }
            else if (cap.index == language_capture)
            {
                lang_name = node_text(cap.node, buffer);
            }
        }

        if (not has_content)
            continue;

        if (match.pattern_index < (uint32_t)patterns.size())
        {
            const auto& pattern = patterns[(int)match.pattern_index];
            if (not pattern.language.empty())
                lang_name = pattern.language;
        }

        if (lang_name.empty())
            continue;

        for (auto& c : lang_name)
            if (c >= 'A' and c <= 'Z')
                c = c - 'A' + 'a';

        TSRange range{};
        range.start_point = ts_node_start_point(content_node);
        range.end_point = ts_node_end_point(content_node);
        range.start_byte = ts_node_start_byte(content_node);
        range.end_byte = ts_node_end_byte(content_node);

        if (range.start_byte >= range.end_byte)
            continue;

        bool is_combined = match.pattern_index < (uint32_t)patterns.size()
                           and patterns[(int)match.pattern_index].combined;

        bool merged = false;
        if (is_combined)
        {
            for (auto& p : pending)
            {
                if (p.combined and p.language_name == lang_name)
                {
                    p.ranges.push_back(range);
                    merged = true;
                    break;
                }
            }
        }

        if (not merged)
        {
            PendingInjection p;
            p.language_name = std::move(lang_name);
            p.ranges.push_back(range);
            p.combined = is_combined;
            // Node id survives incremental reparse when the subtree is reused,
            // letting us match this pending injection against a pooled layer
            // tree from the previous pass.
            p.content_node_id = reinterpret_cast<uintptr_t>(content_node.id);
            pending.push_back(std::move(p));
        }
    }
}

void SyntaxTree::detect_injections(const Buffer& buffer)
{
    const auto* cfg = config();
    if (not cfg or not cfg->injection_query() or not m_tree)
        return;

    if (m_injection_timestamp == m_timestamp)
        return;
    m_injection_timestamp = m_timestamp;

    if (not LanguageRegistry::has_instance())
    {
        m_injection_layers.clear();
        m_pending_edits.clear();
        return;
    }

    // Move previous layers into two pools:
    //  - by_node: keyed on host content node id. Survives incremental host
    //    reparse iff the subtree was reused, so a match here means we can
    //    reuse the layer's tree as old_tree for ts_parser_parse (incremental
    //    reparse of the injection).
    //  - by_lang: fallback when node identity is lost. Parser is reused. For
    //    NON-combined injections the stale tree is dropped (a different host
    //    node means a different region). For COMBINED injections the tree is
    //    kept and reused as old_tree: a combined injection is a per-(host
    //    layer, language) singleton, so even though the host content node id
    //    churns on every edit, the edit-translated old tree is still the
    //    correct incremental-parse base. Without this, the giant single CSS
    //    layer of a *.css.liquid file is reparsed from scratch every keystroke.
    struct PooledLayer
    {
        TSParser* parser;
        TSTree* tree;  // edit-translated; usable as old_tree, or null
        String language_name;
        bool combined = false;
    };
    HashMap<uintptr_t, PooledLayer, MemoryDomain::Highlight> by_node;
    HashMap<String, Vector<PooledLayer>, MemoryDomain::Highlight> by_lang;

    const bool have_edits = not m_pending_edits.empty();
    for (auto& layer : m_injection_layers)
    {
        if (not layer.parser)
            continue;

        // Translate the layer tree into the post-edit coordinate space.
        // Without this, byte offsets embedded in the tree reference a buffer
        // state that no longer exists.
        if (layer.tree and have_edits)
        {
            for (auto& edit : m_pending_edits)
                ts_tree_edit(layer.tree, &edit);
        }

        PooledLayer p{layer.parser, layer.tree, layer.language_name,
                      layer.combined};
        // Combined injections always go to the language pool: their host
        // content node id is unstable across edits, so node-id matching is
        // pointless and the by_lang path now preserves their tree anyway.
        if (not layer.combined and layer.content_node_id
            and by_node.find(layer.content_node_id) == by_node.end())
            by_node.insert({layer.content_node_id, p});
        else
            by_lang[layer.language_name].push_back(p);

        layer.parser = nullptr;
        layer.tree = nullptr;
    }
    m_injection_layers.clear();
    m_pending_edits.clear();

    auto acquire_for = [&](uintptr_t node_id, const String& lang,
                           TSLanguage* ts_lang, bool combined) -> PooledLayer
    {
        // 1) Exact node-id match → parser + old tree for incremental reparse.
        //    (Combined injections never populate by_node — see pooling above.)
        if (node_id and not combined)
        {
            auto it = by_node.find(node_id);
            if (it != by_node.end())
            {
                PooledLayer p = it->value;
                by_node.remove(it);
                if (p.language_name == lang)
                    return p;
                // Language drift at the same node id (shouldn't happen but
                // defend): drop stale tree, fall through to fresh parser.
                if (p.tree)
                    ts_tree_delete(p.tree);
                if (p.parser)
                    ts_parser_delete(p.parser);
            }
        }

        // 2) Language-pool match. For combined injections, KEEP the
        //    edit-translated tree as the incremental-parse base (the layer is
        //    a per-(host layer, language) singleton, so the tree is still
        //    valid even though the host node id changed). For non-combined,
        //    a language-only match means a different host region — the old
        //    tree no longer corresponds to it, so drop it.
        auto lit = by_lang.find(lang);
        if (lit != by_lang.end() and not lit->value.empty())
        {
            PooledLayer p = lit->value.back();
            lit->value.pop_back();
            if (p.tree and not combined)
            {
                ts_tree_delete(p.tree);
                p.tree = nullptr;
            }
            return p;
        }

        // 3) Fresh parser.
        TSParser* parser = ts_parser_new();
        if (not parser)
            return {nullptr, nullptr, {}};
        if (not ts_parser_set_language(parser, ts_lang))
        {
            ts_parser_delete(parser);
            return {nullptr, nullptr, {}};
        }
        return {parser, nullptr, lang};
    };

    // Queue-based recursive injection detection (matches Helix algorithm):
    // Process root tree first, then each injection layer's tree, discovering
    // sub-injections until no more are found.
    struct QueueEntry
    {
        TSTree* tree;
        const LanguageConfig* config;
    };

    Vector<QueueEntry> queue;
    queue.push_back({m_tree, cfg});

    constexpr int max_depth = 8;  // prevent infinite recursion
    int depth = 0;

    while (not queue.empty() and depth < max_depth)
    {
        // Process current queue level
        Vector<QueueEntry> next_queue;

        for (auto& entry : queue)
        {
            Vector<PendingInjection> pending;
            collect_injections_from_tree(entry.tree, entry.config, buffer, pending);

            for (auto& p : pending)
            {
                const auto* inj_config = LanguageRegistry::instance().get(p.language_name);
                if (not inj_config or not inj_config->language())
                    continue;

                InjectionLayer layer;
                layer.language_name = std::move(p.language_name);
                layer.config = inj_config;
                layer.ranges = std::move(p.ranges);
                layer.content_node_id = p.content_node_id;
                layer.combined = p.combined;

                PooledLayer pooled = acquire_for(p.content_node_id,
                                                 layer.language_name,
                                                 inj_config->language(),
                                                 layer.combined);
                if (not pooled.parser)
                    continue;

                layer.parser = pooled.parser;

                ts_parser_set_included_ranges(layer.parser, layer.ranges.data(),
                                              (uint32_t)layer.ranges.size());

                TSInput input{};
                input.payload = const_cast<Buffer*>(&buffer);
                input.read = ts_input_read;
                input.encoding = TSInputEncodingUTF8;

                // Incremental reparse when we have an edit-translated old
                // tree; otherwise from-scratch. Tree-sitter does not free
                // the old tree on our behalf, so dispose of it if the parse
                // returned a distinct new tree.
                TSTree* old_tree = pooled.tree;

                // Tree-sitter's lexer can crash if the old tree is reused but
                // doesn't span the full extent of the new included ranges
                // (it reads past the old tree's end). Helix guards combined
                // injections the same way (tree-house parse.rs): only reuse
                // the old tree when its root byte range covers the new ranges.
                if (old_tree and not layer.ranges.empty())
                {
                    const uint32_t want_start = layer.ranges.front().start_byte;
                    const uint32_t want_end = layer.ranges.back().end_byte;
                    const TSNode old_root = ts_tree_root_node(old_tree);
                    const uint32_t have_start = ts_node_start_byte(old_root);
                    const uint32_t have_end = ts_node_end_byte(old_root);
                    if (have_start > want_start or have_end < want_end)
                    {
                        ts_tree_delete(old_tree);
                        old_tree = nullptr;
                    }
                }

                layer.tree = parse_with_budget(layer.parser, old_tree, input);
                // On timeout layer.tree is null. Fall back to the edit-translated
                // old tree (still approximately correct) rather than dropping
                // the layer entirely, so injected CSS keeps its highlighting.
                if (not layer.tree and old_tree)
                {
                    layer.tree = old_tree;
                    old_tree = nullptr;
                    // Keep timestamp behind so the next pass retries.
                    m_injection_timestamp = m_timestamp - 1;
                }
                if (old_tree and old_tree != layer.tree)
                    ts_tree_delete(old_tree);

                if (layer.tree)
                {
                    // Queue this layer for sub-injection detection if it has an injection query
                    if (inj_config->injection_query())
                        next_queue.push_back({layer.tree, inj_config});
                    m_injection_layers.push_back(std::move(layer));
                }
                else if (layer.parser)
                {
                    ts_parser_delete(layer.parser);
                    layer.parser = nullptr;
                }
            }
        }

        queue = std::move(next_queue);
        ++depth;
    }

    // Dispose of pool leftovers we didn't reuse.
    auto dispose = [](PooledLayer& p)
    {
        if (p.tree)
            ts_tree_delete(p.tree);
        if (p.parser)
            ts_parser_delete(p.parser);
    };
    for (auto& slot : by_node)
        dispose(slot.value);
    for (auto& slot : by_lang)
        for (auto& p : slot.value)
            dispose(p);
}

// Background (off-render-thread) host parsing — see syntax_tree.hh.

void BufferSnapshot::build(const Buffer& buffer)
{
    const int count = (int)buffer.line_count();
    lines.resize(count);
    line_start_bytes.resize(count);
    uint32_t offset = 0;
    for (int i = 0; i < count; ++i)
    {
        // Deep copy: the worker must own this memory outright. StringData
        // refcounts are non-atomic, so we cannot share the buffer's storage.
        StringView line = buffer[LineCount{i}];
        lines[i] = String{line};
        line_start_bytes[i] = offset;
        offset += (uint32_t)(int)line.length();
    }
    timestamp = buffer.timestamp();
}

namespace
{
// tree-sitter input reader over a worker-owned BufferSnapshot. Mirrors the
// live-buffer ts_input_read but touches only the snapshot.
const char* snapshot_input_read(void* payload, uint32_t,
                                 TSPoint position, uint32_t* bytes_read)
{
    const auto& snap = *static_cast<const BufferSnapshot*>(payload);
    const int row = (int)position.row;
    if (row < 0 or row >= (int)snap.lines.size())
    {
        *bytes_read = 0;
        return "";
    }
    const String& line = snap.lines[row];
    const int col = (int)position.column;
    if (col >= (int)line.length())
    {
        *bytes_read = 0;
        return "";
    }
    *bytes_read = (uint32_t)((int)line.length() - col);
    return line.data() + col;
}

// Runs entirely on the worker thread. Parses the host language from the
// snapshot, from scratch, with the same wall-clock budget as the sync path.
// Touches no kakoune singletons and no live Buffer.
TSTree* worker_parse_host(TSLanguage* language, const BufferSnapshot& snap)
{
    TSParser* parser = ts_parser_new();
    if (not parser)
        return nullptr;
    if (not ts_parser_set_language(parser, language))
    {
        ts_parser_delete(parser);
        return nullptr;
    }
    TSInput input{};
    input.payload = const_cast<BufferSnapshot*>(&snap);
    input.read = snapshot_input_read;
    input.encoding = TSInputEncodingUTF8;

    TSTree* tree = parse_with_budget(parser, nullptr, input);
    ts_parser_delete(parser);
    return tree;
}
}

ParsedSyntax::~ParsedSyntax()
{
    if (tree)
        ts_tree_delete(tree);
}

ParsedSyntax::ParsedSyntax(ParsedSyntax&& o) noexcept
    : tree(o.tree), byte_index(std::move(o.byte_index)), timestamp(o.timestamp)
{
    o.tree = nullptr;
}

ParsedSyntax& ParsedSyntax::operator=(ParsedSyntax&& o) noexcept
{
    if (this != &o)
    {
        if (tree)
            ts_tree_delete(tree);
        tree = o.tree;
        byte_index = std::move(o.byte_index);
        timestamp = o.timestamp;
        o.tree = nullptr;
    }
    return *this;
}

// A parse slower than this on the render thread flips the buffer into
// background mode. Generous enough that small/normal files never trip it
// (and so never pay the worker-thread machinery), tight enough that a slow
// file only stalls one frame before going async.
static constexpr std::chrono::milliseconds async_threshold{12};

AsyncSyntaxTree::AsyncSyntaxTree(const Buffer& buffer, const LanguageConfig* config)
    : m_config(config), m_language_name(config->name()), m_buffer(&buffer)
{
    // Start synchronous: no pipe, no thread. The first parse runs here and
    // flips to background mode only if it is slow.
    parse_sync(buffer);
}

AsyncSyntaxTree::~AsyncSyntaxTree()
{
    // Worker only touches data it owns; let it finish (bounded by the parse
    // budget) so its member writes complete before teardown.
    if (m_worker.joinable())
        m_worker.join();
    m_watcher.reset();
    if (m_wake_pipe[0] >= 0) close(m_wake_pipe[0]);
    if (m_wake_pipe[1] >= 0) close(m_wake_pipe[1]);
}

void AsyncSyntaxTree::parse_sync(const Buffer& buffer)
{
    if (not m_config or not m_config->language())
        return;

    TSParser* parser = ts_parser_new();
    if (not parser)
        return;
    if (not ts_parser_set_language(parser, m_config->language()))
    {
        ts_parser_delete(parser);
        return;
    }
    TSInput input{};
    input.payload = const_cast<Buffer*>(&buffer);
    input.read = ts_input_read;
    input.encoding = TSInputEncodingUTF8;

    const auto t0 = std::chrono::steady_clock::now();
    TSTree* tree = parse_with_budget(parser, nullptr, input);
    const auto elapsed = std::chrono::steady_clock::now() - t0;
    ts_parser_delete(parser);

    if (tree)
    {
        ParsedSyntax result;
        result.tree = tree;
        result.timestamp = buffer.timestamp();
        result.byte_index.rebuild(buffer);
        m_current = std::move(result);
    }

    // Too slow on the render thread — switch this buffer to background mode
    // so subsequent edits never block again.
    if (elapsed >= async_threshold)
        enter_background_mode();
}

void AsyncSyntaxTree::enter_background_mode()
{
    if (m_background_mode)
        return;

    if (pipe(m_wake_pipe) != 0)
        return;   // stay synchronous if we can't get a pipe
    bool committed = false;
    auto pipe_guard = OnScopeEnd([&] {
        if (committed) return;
        if (m_wake_pipe[0] >= 0) { close(m_wake_pipe[0]); m_wake_pipe[0] = -1; }
        if (m_wake_pipe[1] >= 0) { close(m_wake_pipe[1]); m_wake_pipe[1] = -1; }
    });

    fcntl(m_wake_pipe[0], F_SETFL,
          fcntl(m_wake_pipe[0], F_GETFL, 0) | O_NONBLOCK);

    m_watcher = make_unique_ptr<FDWatcher>(
        m_wake_pipe[0], FdEvents::Read, EventMode::Normal,
        [this](FDWatcher&, FdEvents, EventMode) { on_worker_done(); });

    committed = true;
    m_background_mode = true;
}

void AsyncSyntaxTree::start_job(const Buffer& buffer)
{
    // Single guard for "a job is in flight": callers don't pre-check.
    if (m_job_running.load(std::memory_order_acquire))
        return;
    if (m_worker.joinable())
        m_worker.join();           // reap the previous finished worker

    if (not m_config or not m_config->language())
        return;

    // After a timed-out parse, skip rebuilding an identical snapshot if the
    // buffer hasn't moved — the next attempt would copy 300KB for nothing.
    if (m_last_parse_timed_out
        and m_dispatched_timestamp == buffer.timestamp())
        return;

    m_pending_snapshot.build(buffer);   // main thread: deep copy
    m_dispatched_timestamp = buffer.timestamp();
    m_job_running.store(true, std::memory_order_release);

    TSLanguage* language = m_config->language();
    int wake_fd = m_wake_pipe[1];
    m_worker = std::thread([this, language, wake_fd]() {
        TSTree* tree = worker_parse_host(language, m_pending_snapshot);

        ParsedSyntax result;
        result.tree = tree;
        result.timestamp = m_pending_snapshot.timestamp;
        if (tree)
            result.byte_index.rebuild_from_offsets(
                m_pending_snapshot.line_start_bytes);
        m_worker_result = std::move(result);

        m_job_running.store(false, std::memory_order_release);
        // Wake the event loop; on_worker_done drains the pipe.
        const char b = 1;
        ssize_t w = ::write(wake_fd, &b, 1);
        (void)w;
    });
}

void AsyncSyntaxTree::on_worker_done()
{
    char buf[64];
    while (::read(m_wake_pipe[0], buf, sizeof(buf)) > 0) {}

    // Job still running: this was a stale byte from a previous job; the
    // running job will signal again on completion.
    if (m_job_running.load(std::memory_order_acquire))
        return;

    if (m_worker.joinable())
        m_worker.join();           // synchronizes the worker's member writes

    const bool got_tree = m_worker_result.is_valid();
    m_last_parse_timed_out = not got_tree;
    if (got_tree)
        m_current = std::move(m_worker_result);
    else
        m_worker_result = ParsedSyntax{};   // keep the previous good tree

    // The worker no longer needs the snapshot; reclaim its ~300KB rather than
    // pinning it for the buffer's whole lifetime.
    m_pending_snapshot = BufferSnapshot{};

    // Repaint only clients displaying this buffer, and only when we actually
    // have a fresher tree (a timed-out parse changed nothing).
    if (got_tree and ClientManager::has_instance())
        for (auto& client : ClientManager::instance())
            if (client->context().has_buffer()
                and &client->context().buffer() == m_buffer)
                client->force_redraw();
}

void AsyncSyntaxTree::poll(const Buffer& buffer)
{
    if (m_current.timestamp == buffer.timestamp())
        return;   // already up to date

    if (not m_background_mode)
    {
        // Parse inline; parse_sync flips to background mode if it's slow.
        parse_sync(buffer);
        return;
    }

    // Background mode: start_job() is itself the in-flight guard. Coalescing
    // falls out for free — keystrokes during a parse don't start jobs; the
    // next poll after completion sees the newer timestamp.
    start_job(buffer);
}

static const ValueId async_syntax_tree_id = get_free_value_id();

void create_async_syntax_tree(const Buffer& buffer, const LanguageConfig* config)
{
    // Construct in place inside the Value's heap Model so the object's
    // address is stable: AsyncSyntaxTree is non-movable and its FDWatcher
    // lambda / worker thread capture `this`.
    buffer.values()[async_syntax_tree_id] =
        Value(Meta::Type<AsyncSyntaxTree>{}, buffer, config);
}

AsyncSyntaxTree& get_async_syntax_tree(const Buffer& buffer)
{
    Value& val = buffer.values()[async_syntax_tree_id];
    if (not val)
        throw runtime_error("no async syntax tree for this buffer");
    return val.as<AsyncSyntaxTree>();
}

bool has_async_syntax_tree(const Buffer& buffer)
{
    return buffer.values().contains(async_syntax_tree_id);
}

void remove_async_syntax_tree(const Buffer& buffer)
{
    buffer.values().erase(async_syntax_tree_id);
}

static const ValueId syntax_tree_id = get_free_value_id();

void create_syntax_tree(const Buffer& buffer, const LanguageConfig* config)
{
    buffer.values()[syntax_tree_id] = Value(SyntaxTree{buffer, config});
}

void create_syntax_tree_deferred(const Buffer& buffer, const LanguageConfig* config)
{
    buffer.values()[syntax_tree_id] =
        Value(Meta::Type<SyntaxTree>{}, DeferParse{}, config);
}

SyntaxTree& get_syntax_tree(const Buffer& buffer)
{
    Value& val = buffer.values()[syntax_tree_id];
    if (not val)
        throw runtime_error("no syntax tree for this buffer");
    return val.as<SyntaxTree>();
}

void remove_syntax_tree(const Buffer& buffer)
{
    buffer.values().erase(syntax_tree_id);
}

bool has_syntax_tree(const Buffer& buffer)
{
    return buffer.values().contains(syntax_tree_id);
}

SyntaxTree* sync_async_host_tree(const Buffer& buffer, const LanguageConfig* config)
{
    if (not has_syntax_tree(buffer))
    {
        try { create_syntax_tree_deferred(buffer, config); }
        catch (runtime_error&) { return nullptr; }
    }
    if (not has_async_syntax_tree(buffer))
    {
        try { create_async_syntax_tree(buffer, config); }
        catch (runtime_error&) { return nullptr; }
    }

    auto& async_tree = get_async_syntax_tree(buffer);
    async_tree.poll(buffer);

    const ParsedSyntax& parsed = async_tree.current();
    if (not parsed.is_valid())
        return nullptr;   // first parse still in flight; a redraw will follow

    auto& syntax_tree = get_syntax_tree(buffer);
    if (syntax_tree.timestamp() != parsed.timestamp)
    {
        // ParsedSyntax keeps its tree to serve later non-advancing frames, so
        // hand SyntaxTree an owned copy (ts_tree_copy is O(1) — a refcount
        // bump, not a deep copy). Tree ownership stays out of the render layer.
        LineByteIndex bi = parsed.byte_index;
        syntax_tree.adopt_tree(ts_tree_copy(parsed.tree), std::move(bi),
                               parsed.timestamp);
    }
    return syntax_tree.is_valid() ? &syntax_tree : nullptr;
}

} // namespace Kakoune
