#ifndef syntax_tree_hh_INCLUDED
#define syntax_tree_hh_INCLUDED

#include "tree_sitter.hh"
#include "array_view.hh"
#include "buffer.hh"
#include "string.hh"
#include "utils.hh"
#include "vector.hh"

namespace Kakoune
{

class LanguageConfig;

struct InjectionLayer
{
    TSParser* parser = nullptr;
    TSTree* tree = nullptr;
    String language_name;
    const LanguageConfig* config = nullptr;  // not owned
    Vector<TSRange, MemoryDomain::Highlight> ranges;

    InjectionLayer() = default;
    ~InjectionLayer();
    InjectionLayer(InjectionLayer&&) noexcept;
    InjectionLayer& operator=(InjectionLayer&&) noexcept;
    InjectionLayer(const InjectionLayer&) = delete;
    InjectionLayer& operator=(const InjectionLayer&) = delete;
};

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
    SyntaxTree(const Buffer& buffer, const LanguageConfig* config);
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

    ConstArrayView<InjectionLayer> injection_layers() const { return m_injection_layers; }
    void detect_injections(const Buffer& buffer);
    const LanguageConfig* config() const;  // re-resolves from registry each call
    const String& language_name() const { return m_language_name; }

private:
    void full_parse(const Buffer& buffer);

    TSParser* m_parser = nullptr;
    TSTree* m_tree = nullptr;
    TSLanguage* m_language = nullptr;
    TSQuery* m_highlight_query = nullptr;  // not owned — owned by LanguageConfig
    String m_language_name;                // used to re-resolve config from registry
    LineByteIndex m_byte_index;
    size_t m_timestamp = 0;
    Vector<InjectionLayer, MemoryDomain::Highlight> m_injection_layers;
};

void create_syntax_tree(const Buffer& buffer, const LanguageConfig* config);
SyntaxTree& get_syntax_tree(const Buffer& buffer);
void remove_syntax_tree(const Buffer& buffer);
bool has_syntax_tree(const Buffer& buffer);

} // namespace Kakoune

#endif // syntax_tree_hh_INCLUDED
