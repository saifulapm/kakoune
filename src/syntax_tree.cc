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
    const int line = (int)coord.line;
    kak_assert(line >= 0 and line < (int)m_line_start_bytes.size());
    return m_line_start_bytes[line] + (uint32_t)(int)coord.column;
}

TSInputEdit make_ts_input_edit(const LineByteIndex& index,
                               const Buffer::Change& change)
{
    const TSPoint start_point = {(uint32_t)(int)change.begin.line,
                                 (uint32_t)(int)change.begin.column};
    const uint32_t start_byte = index.byte_offset(change.begin);

    TSInputEdit edit{};
    edit.start_byte = start_byte;
    edit.start_point = start_point;

    if (change.type == Buffer::Change::Insert)
    {
        // Insert: old_end == start (nothing was removed), new_end = change.end
        edit.old_end_byte = start_byte;
        edit.old_end_point = start_point;
        edit.new_end_byte = index.byte_offset(change.end);
        edit.new_end_point = {(uint32_t)(int)change.end.line,
                              (uint32_t)(int)change.end.column};
    }
    else
    {
        // Erase: old_end = change.end (content was removed), new_end == start
        edit.old_end_byte = index.byte_offset(change.end);
        edit.old_end_point = {(uint32_t)(int)change.end.line,
                              (uint32_t)(int)change.end.column};
        edit.new_end_byte = start_byte;
        edit.new_end_point = start_point;
    }

    return edit;
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
    m_timestamp = buffer.timestamp();
}

SyntaxTree::SyntaxTree(const Buffer& buffer, const LanguageConfig* config)
    : m_language(config->language()),
      m_highlight_query(config->highlight_query()),
      m_config(config)
{
    m_parser = ts_parser_new();
    if (not m_parser)
        throw runtime_error("failed to create tree-sitter parser");

    if (not ts_parser_set_language(m_parser, m_language))
        throw runtime_error("failed to set tree-sitter language");

    full_parse(buffer);
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
      m_config(other.m_config),
      m_byte_index(std::move(other.m_byte_index)),
      m_timestamp(other.m_timestamp),
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
        m_config = other.m_config;
        m_byte_index = std::move(other.m_byte_index);
        m_timestamp = other.m_timestamp;
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

    if (changes.size() == 1)
    {
        // Single change: m_byte_index is still valid for the pre-change state
        auto edit = make_ts_input_edit(m_byte_index, changes[0]);
        ts_tree_edit(m_tree, &edit);
        m_byte_index.rebuild(buffer);

        TSInput input{};
        input.payload = const_cast<Buffer*>(&buffer);
        input.read = ts_input_read;
        input.encoding = TSInputEncodingUTF8;

        TSTree* new_tree = ts_parser_parse(m_parser, m_tree, input);
        if (new_tree)
        {
            ts_tree_delete(m_tree);
            m_tree = new_tree;
        }
    }
    else
    {
        // Multiple changes: byte index becomes stale after the first
        // edit, so fall back to a full reparse for correctness.
        full_parse(buffer);
        return;
    }

    m_timestamp = buffer.timestamp();
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
    if (not m_config or not m_config->injection_query() or not m_tree)
        return;

    m_injection_layers.clear();

    TSQueryCursor* cursor = ts_query_cursor_new();
    if (not cursor)
        return;

    TSNode root = ts_tree_root_node(m_tree);
    ts_query_cursor_exec(cursor, m_config->injection_query(), root);

    const uint32_t content_capture = m_config->injection_content_capture();
    const uint32_t language_capture = m_config->injection_language_capture();
    const auto& patterns = m_config->injection_patterns();

    // Collect ranges per (language, pattern_index) for combined injections
    // Key: language name, Value: vector of TSRange
    struct PendingInjection
    {
        String language_name;
        const LanguageConfig* config;
        Vector<TSRange, MemoryDomain::Highlight> ranges;
    };
    // For combined: group by pattern_index + language
    HashMap<String, PendingInjection, MemoryDomain::Highlight> combined_injections;

    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match))
    {
        // Find content and language captures
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

        // Determine language from pattern metadata or capture
        if (match.pattern_index < (uint16_t)patterns.size())
        {
            const auto& pattern = patterns[match.pattern_index];
            if (not pattern.language.empty())
                lang_name = pattern.language;
        }

        if (lang_name.empty())
            continue;

        // Normalize language name to lowercase
        for (auto& c : lang_name)
            if (c >= 'A' and c <= 'Z')
                c = c - 'A' + 'a';

        // Build TSRange from content node
        TSRange range{};
        range.start_point = ts_node_start_point(content_node);
        range.end_point = ts_node_end_point(content_node);
        range.start_byte = ts_node_start_byte(content_node);
        range.end_byte = ts_node_end_byte(content_node);

        // Skip empty ranges
        if (range.start_byte >= range.end_byte)
            continue;

        bool is_combined = match.pattern_index < (uint16_t)patterns.size()
                           and patterns[match.pattern_index].combined;

        if (is_combined)
        {
            // Group by pattern_index + language for combined injections
            String key = format("{}:{}", match.pattern_index, lang_name);
            auto it = combined_injections.find(key);
            if (it == combined_injections.end())
            {
                PendingInjection pending;
                pending.language_name = lang_name;
                pending.config = nullptr;
                pending.ranges.push_back(range);
                combined_injections[key] = std::move(pending);
            }
            else
            {
                it->value.ranges.push_back(range);
            }
        }
        else
        {
            // Non-combined: create an injection layer immediately
            if (not LanguageRegistry::has_instance())
                continue;

            const auto* inj_config = LanguageRegistry::instance().get(lang_name);
            if (not inj_config or not inj_config->language())
                continue;

            InjectionLayer layer;
            layer.language_name = std::move(lang_name);
            layer.config = inj_config;
            layer.ranges.push_back(range);

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

    // Process combined injections
    if (LanguageRegistry::has_instance())
    {
        for (auto& [key, pending] : combined_injections)
        {
            const auto* inj_config = LanguageRegistry::instance().get(pending.language_name);
            if (not inj_config or not inj_config->language())
                continue;

            InjectionLayer layer;
            layer.language_name = std::move(pending.language_name);
            layer.config = inj_config;
            layer.ranges = std::move(pending.ranges);

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

    ts_query_cursor_delete(cursor);

    if (not m_injection_layers.empty())
        write_to_debug_buffer(format("tree-sitter: detected {} injection layer(s)",
                                     m_injection_layers.size()));
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
