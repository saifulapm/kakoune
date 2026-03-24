#ifndef syntax_tree_hh_INCLUDED
#define syntax_tree_hh_INCLUDED

#include "tree_sitter.hh"
#include "buffer.hh"
#include "utils.hh"
#include "vector.hh"

namespace Kakoune
{

struct LineByteIndex
{
    void rebuild(const Buffer& buffer);
    uint32_t byte_offset(BufferCoord coord) const;

private:
    Vector<uint32_t, MemoryDomain::Highlight> m_line_start_bytes;
};

TSInputEdit make_ts_input_edit(const LineByteIndex& index,
                               const Buffer::Change& change);

class SyntaxTree
{
public:
    SyntaxTree(const Buffer& buffer, TSLanguage* language,
               TSQuery* highlight_query);
    ~SyntaxTree();

    SyntaxTree(SyntaxTree&& other) noexcept;
    SyntaxTree& operator=(SyntaxTree&&) noexcept;
    SyntaxTree(const SyntaxTree&) = delete;
    SyntaxTree& operator=(const SyntaxTree&) = delete;

    void update(const Buffer& buffer);

    TSTree* tree() const { return m_tree; }
    TSQuery* highlight_query() const { return m_highlight_query; }
    const LineByteIndex& byte_index() const { return m_byte_index; }
    bool is_valid() const { return m_tree != nullptr; }
    size_t timestamp() const { return m_timestamp; }

private:
    void full_parse(const Buffer& buffer);

    TSParser* m_parser = nullptr;
    TSTree* m_tree = nullptr;
    TSLanguage* m_language = nullptr;
    TSQuery* m_highlight_query = nullptr;  // not owned — owned by LanguageConfig
    LineByteIndex m_byte_index;
    size_t m_timestamp = 0;
};

void create_syntax_tree(const Buffer& buffer, TSLanguage* language,
                        TSQuery* highlight_query);
SyntaxTree& get_syntax_tree(const Buffer& buffer);
void remove_syntax_tree(const Buffer& buffer);
bool has_syntax_tree(const Buffer& buffer);

} // namespace Kakoune

#endif // syntax_tree_hh_INCLUDED
