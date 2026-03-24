#ifndef language_registry_hh_INCLUDED
#define language_registry_hh_INCLUDED

#include "hash_map.hh"
#include "string.hh"
#include "utils.hh"
#include "vector.hh"
#include "tree_sitter.hh"

namespace Kakoune
{

String capture_to_face_name(StringView capture_name);

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

private:
    friend class LanguageRegistry;

    String m_name;
    TSLanguage* m_language = nullptr;
    TSQuery* m_highlight_query = nullptr;
    Vector<String> m_capture_faces;
    void* m_grammar_handle = nullptr;
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
    HashMap<String, LanguageConfig, MemoryDomain::Highlight> m_languages;
};

} // namespace Kakoune

#endif // language_registry_hh_INCLUDED
