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

struct LanguageConfig
{
    String name;
    TSLanguage* language = nullptr;
    TSQuery* highlight_query = nullptr;
    Vector<String> capture_faces;
    void* grammar_handle = nullptr;

    LanguageConfig() = default;
    ~LanguageConfig();

    LanguageConfig(const LanguageConfig&) = delete;
    LanguageConfig& operator=(const LanguageConfig&) = delete;

    LanguageConfig(LanguageConfig&& other) noexcept;
    LanguageConfig& operator=(LanguageConfig&& other) noexcept;
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
