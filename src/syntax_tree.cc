#include "syntax_tree.hh"

#include "changes.hh"
#include "debug.hh"
#include "exception.hh"
#include "format.hh"
#include "hash_map.hh"
#include "language_registry.hh"

namespace Kakoune
{

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
      content_node_id(other.content_node_id)
{
    other.parser = nullptr;
    other.tree = nullptr;
    other.config = nullptr;
    other.content_node_id = 0;
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

        other.parser = nullptr;
        other.tree = nullptr;
        other.config = nullptr;
        other.content_node_id = 0;
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

    if (m_tree)
        ts_tree_delete(m_tree);

    m_tree = ts_parser_parse(m_parser, nullptr, input);
    if (m_tree)
        m_timestamp = buffer.timestamp();

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

    TSTree* new_tree = ts_parser_parse(m_parser, m_tree, input);
    if (new_tree)
    {
        ts_tree_delete(m_tree);
        m_tree = new_tree;
        m_timestamp = buffer.timestamp();
    }
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
    //  - by_lang: fallback when node identity is lost. Parser is still
    //    reused but the stale tree is dropped.
    struct PooledLayer
    {
        TSParser* parser;
        TSTree* tree;  // edit-translated; usable as old_tree, or null
        String language_name;
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

        PooledLayer p{layer.parser, layer.tree, layer.language_name};
        if (layer.content_node_id and by_node.find(layer.content_node_id) == by_node.end())
            by_node.insert({layer.content_node_id, p});
        else
            by_lang[layer.language_name].push_back(p);

        layer.parser = nullptr;
        layer.tree = nullptr;
    }
    m_injection_layers.clear();
    m_pending_edits.clear();

    auto acquire_for = [&](uintptr_t node_id, const String& lang,
                           TSLanguage* ts_lang) -> PooledLayer
    {
        // 1) Exact node-id match → parser + old tree for incremental reparse.
        if (node_id)
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

        // 2) Language-pool match → parser only, no old tree.
        auto lit = by_lang.find(lang);
        if (lit != by_lang.end() and not lit->value.empty())
        {
            PooledLayer p = lit->value.back();
            lit->value.pop_back();
            if (p.tree)
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

                PooledLayer pooled = acquire_for(p.content_node_id,
                                                 layer.language_name,
                                                 inj_config->language());
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
                layer.tree = ts_parser_parse(layer.parser, old_tree, input);
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

static const ValueId syntax_tree_id = get_free_value_id();

void create_syntax_tree(const Buffer& buffer, const LanguageConfig* config)
{
    buffer.values()[syntax_tree_id] = Value(SyntaxTree{buffer, config});
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

} // namespace Kakoune
