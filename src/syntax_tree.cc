#include "syntax_tree.hh"

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
      ranges(std::move(other.ranges))
{
    other.parser = nullptr;
    other.tree = nullptr;
    other.config = nullptr;
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

        other.parser = nullptr;
        other.tree = nullptr;
        other.config = nullptr;
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
      m_injection_layers(std::move(other.m_injection_layers))
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

    // Apply edits in REVERSE order (like Helix/tree-house). When applied
    // in reverse, each edit only affects byte positions to its LEFT, which
    // haven't been processed yet. This means the pre-change byte index
    // remains valid for all edits — no adjustment tracking needed.
    //
    // For byte offsets: use the OLD m_byte_index which reflects the tree's
    // current coordinate space. For Insert changes, new_end_byte needs the
    // byte length of the inserted content — we compute this from the change
    // coordinates since we know the exact line/column extent.

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

void SyntaxTree::detect_injections(const Buffer& buffer)
{
    const auto* cfg = config();
    if (not cfg or not cfg->injection_query() or not m_tree)
        return;

    if (m_injection_timestamp == m_timestamp)
        return;
    m_injection_timestamp = m_timestamp;

    m_injection_layers.clear();

    // Phase 1: Collect all injection ranges from query matches.
    // IMPORTANT: Do NOT call LanguageRegistry::get() during this phase,
    // because get() can load new languages which modifies the HashMap
    // and may invalidate m_config (which points into the HashMap).

    struct PendingInjection
    {
        String language_name;
        Vector<TSRange, MemoryDomain::Highlight> ranges;
        bool combined;
    };
    Vector<PendingInjection, MemoryDomain::Highlight> pending;

    {
        QueryCursorGuard cursor;
        if (not cursor.cursor)
            return;

        ts_query_cursor_set_match_limit(cursor, 256);

        TSNode root = ts_tree_root_node(m_tree);
        ts_query_cursor_exec(cursor, cfg->injection_query(), root);

        const uint32_t content_capture = cfg->injection_content_capture();
        const uint32_t language_capture = cfg->injection_language_capture();
        const auto& patterns = cfg->injection_patterns();
        const auto& inj_preds = cfg->injection_predicates();

        TSQueryMatch match;
        while (ts_query_cursor_next_match(cursor, &match))
        {
            // Filter by predicates
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

            // Check if we can merge with an existing pending entry (same language + combined)
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
                pending.push_back(std::move(p));
            }
        }

    }

    // Phase 2: Now that the query cursor is done, load languages and create layers.
    // It's safe to call LanguageRegistry::get() here.
    if (not LanguageRegistry::has_instance())
        return;

    for (auto& p : pending)
    {
        const auto* inj_config = LanguageRegistry::instance().get(p.language_name);
        if (not inj_config or not inj_config->language())
            continue;

        InjectionLayer layer;
        layer.language_name = std::move(p.language_name);
        layer.config = inj_config;
        layer.ranges = std::move(p.ranges);

        layer.parser = ts_parser_new();
        if (not layer.parser)
            continue;

        if (not ts_parser_set_language(layer.parser, inj_config->language()))
        {
            ts_parser_delete(layer.parser);
            layer.parser = nullptr;
            continue;
        }

        ts_parser_set_included_ranges(layer.parser, layer.ranges.data(),
                                      (uint32_t)layer.ranges.size());

        TSInput input{};
        input.payload = const_cast<Buffer*>(&buffer);
        input.read = ts_input_read;
        input.encoding = TSInputEncodingUTF8;

        layer.tree = ts_parser_parse(layer.parser, nullptr, input);
        if (layer.tree)
            m_injection_layers.push_back(std::move(layer));
    }
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
