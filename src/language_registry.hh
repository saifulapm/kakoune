#ifndef language_registry_hh_INCLUDED
#define language_registry_hh_INCLUDED

#include "hash_map.hh"
#include "string.hh"
#include "unique_ptr.hh"
#include "utils.hh"
#include "vector.hh"
#include "tree_sitter.hh"

#include <climits>

namespace Kakoune
{

String capture_to_face_name(StringView capture_name);

struct InjectionPattern
{
    String language;                // from #set! injection.language "..."
    bool combined = false;          // from #set! injection.combined
    bool include_children = false;  // from #set! injection.include-children
};

class LanguageConfig
{
public:
    LanguageConfig() = default;
    ~LanguageConfig();

    LanguageConfig(const LanguageConfig&) = delete;
    LanguageConfig& operator=(const LanguageConfig&) = delete;

    LanguageConfig(LanguageConfig&& other) noexcept;
    LanguageConfig& operator=(LanguageConfig&& other) noexcept;

    const String& name() const { return m_name; }
    TSLanguage* language() const { return m_language; }
    TSQuery* highlight_query() const { return m_highlight_query; }
    const Vector<String>& capture_faces() const { return m_capture_faces; }

    TSQuery* injection_query() const { return m_injection_query; }
    const Vector<InjectionPattern, MemoryDomain::Highlight>& injection_patterns() const { return m_injection_patterns; }
    uint32_t injection_content_capture() const { return m_injection_content_capture; }
    uint32_t injection_language_capture() const { return m_injection_language_capture; }

    TSQuery* textobject_query() const { return m_textobject_query; }
    TSQuery* indent_query() const { return m_indent_query; }
    TSQuery* locals_query() const { return m_locals_query; }

private:
    friend class LanguageRegistry;

    String m_name;
    TSLanguage* m_language = nullptr;
    TSQuery* m_highlight_query = nullptr;
    Vector<String> m_capture_faces;
    void* m_grammar_handle = nullptr;

    TSQuery* m_injection_query = nullptr;
    Vector<InjectionPattern, MemoryDomain::Highlight> m_injection_patterns;
    uint32_t m_injection_content_capture = UINT32_MAX;
    uint32_t m_injection_language_capture = UINT32_MAX;

    TSQuery* m_textobject_query = nullptr;
    TSQuery* m_indent_query = nullptr;
    TSQuery* m_locals_query = nullptr;
};

class LanguageRegistry : public Singleton<LanguageRegistry>
{
public:
    LanguageRegistry(String runtime_dir);

    const LanguageConfig* get(StringView name);

    static StringView filetype_to_language(StringView filetype);

private:
    const LanguageConfig* load_language(StringView name);

    String m_runtime_dir;
    // Store UniquePtr so HashMap reallocation doesn't move/invalidate LanguageConfig objects
    HashMap<String, UniquePtr<LanguageConfig>, MemoryDomain::Highlight> m_languages;
};

} // namespace Kakoune

#endif // language_registry_hh_INCLUDED
