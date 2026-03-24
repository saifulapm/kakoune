#include "syntax_tree.hh"

#include "exception.hh"

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

SyntaxTree::SyntaxTree(const Buffer& buffer, TSLanguage* language,
                       TSQuery* highlight_query)
    : m_language(language), m_highlight_query(highlight_query)
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
      m_byte_index(std::move(other.m_byte_index)),
      m_timestamp(other.m_timestamp)
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
        m_byte_index = std::move(other.m_byte_index);
        m_timestamp = other.m_timestamp;

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

static const ValueId syntax_tree_id = get_free_value_id();

void create_syntax_tree(const Buffer& buffer, TSLanguage* language,
                        TSQuery* highlight_query)
{
    buffer.values()[syntax_tree_id] = Value(SyntaxTree{buffer, language, highlight_query});
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
