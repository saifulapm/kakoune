#include "language_registry.hh"

#include "debug.hh"
#include "exception.hh"
#include "file.hh"
#include "format.hh"

#include <dlfcn.h>

namespace Kakoune
{

String capture_to_face_name(StringView capture_name)
{
    String result = "ts_";
    for (auto c : capture_name)
        result += (c == '.') ? '_' : c;
    return result;
}

LanguageConfig::~LanguageConfig()
{
    if (highlight_query)
        ts_query_delete(highlight_query);
    if (grammar_handle)
        dlclose(grammar_handle);
}

LanguageConfig::LanguageConfig(LanguageConfig&& other) noexcept
    : name(std::move(other.name)),
      language(other.language),
      highlight_query(other.highlight_query),
      capture_faces(std::move(other.capture_faces)),
      grammar_handle(other.grammar_handle)
{
    other.language = nullptr;
    other.highlight_query = nullptr;
    other.grammar_handle = nullptr;
}

LanguageConfig& LanguageConfig::operator=(LanguageConfig&& other) noexcept
{
    if (this != &other)
    {
        if (highlight_query)
            ts_query_delete(highlight_query);
        if (grammar_handle)
            dlclose(grammar_handle);

        name = std::move(other.name);
        language = other.language;
        highlight_query = other.highlight_query;
        capture_faces = std::move(other.capture_faces);
        grammar_handle = other.grammar_handle;

        other.language = nullptr;
        other.highlight_query = nullptr;
        other.grammar_handle = nullptr;
    }
    return *this;
}

LanguageRegistry::LanguageRegistry(String runtime_dir)
    : m_runtime_dir(std::move(runtime_dir))
{
}

StringView LanguageRegistry::filetype_to_language(StringView filetype)
{
    if (filetype == "sh" or filetype == "bash" or filetype == "zsh")
        return "bash";
    if (filetype == "cpp" or filetype == "objc")
        return "cpp";
    if (filetype == "javascript" or filetype == "jsx")
        return "javascript";
    if (filetype == "typescript" or filetype == "tsx")
        return "tsx";
    return filetype;
}

const LanguageConfig* LanguageRegistry::get(StringView name)
{
    auto it = m_languages.find(name);
    if (it != m_languages.end())
    {
        if (it->value.language == nullptr)
            return nullptr;
        return &it->value;
    }
    return load_language(name);
}

const LanguageConfig* LanguageRegistry::load_language(StringView name)
{
    // Try to dlopen the grammar shared library
    String so_path = format("{}/runtime/grammars/{}.so", m_runtime_dir, name);
    void* handle = dlopen(so_path.c_str(), RTLD_LAZY);

#ifdef __APPLE__
    if (not handle)
    {
        String dylib_path = format("{}/runtime/grammars/{}.dylib", m_runtime_dir, name);
        handle = dlopen(dylib_path.c_str(), RTLD_LAZY);
    }
#endif

    if (not handle)
    {
        write_to_debug_buffer(format("tree-sitter: failed to load grammar '{}': {}",
                                     name, dlerror()));
        m_languages[name.str()] = LanguageConfig{};
        return nullptr;
    }

    // Look up the grammar function
    String symbol_name = format("tree_sitter_{}", name);
    using GrammarFn = TSLanguage* (*)();
    auto grammar_fn = reinterpret_cast<GrammarFn>(dlsym(handle, symbol_name.c_str()));
    if (not grammar_fn)
    {
        write_to_debug_buffer(format("tree-sitter: symbol '{}' not found in grammar library: {}",
                                     symbol_name, dlerror()));
        dlclose(handle);
        m_languages[name.str()] = LanguageConfig{};
        return nullptr;
    }

    TSLanguage* lang = grammar_fn();

    // Read the highlights query file
    String query_path = format("{}/runtime/queries/{}/highlights.scm", m_runtime_dir, name);
    String query_text;
    try
    {
        query_text = read_file(query_path);
    }
    catch (runtime_error& err)
    {
        write_to_debug_buffer(format("tree-sitter: failed to read query file '{}': {}",
                                     query_path, err.what()));
        dlclose(handle);
        m_languages[name.str()] = LanguageConfig{};
        return nullptr;
    }

    // Compile the query
    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    TSQuery* query = ts_query_new(lang, query_text.c_str(),
                                  (uint32_t)(int)query_text.length(),
                                  &error_offset, &error_type);
    if (not query)
    {
        write_to_debug_buffer(format("tree-sitter: query compilation failed for '{}' at offset {}, error type {}",
                                     name, error_offset, (int)error_type));
        dlclose(handle);
        m_languages[name.str()] = LanguageConfig{};
        return nullptr;
    }

    // Build capture_faces vector
    uint32_t capture_count = ts_query_capture_count(query);
    Vector<String> faces;
    faces.reserve(capture_count);
    for (uint32_t i = 0; i < capture_count; ++i)
    {
        uint32_t length = 0;
        const char* capture_name = ts_query_capture_name_for_id(query, i, &length);
        faces.push_back(capture_to_face_name({capture_name, (ByteCount)length}));
    }

    // Store the config
    LanguageConfig config;
    config.name = name.str();
    config.language = lang;
    config.highlight_query = query;
    config.capture_faces = std::move(faces);
    config.grammar_handle = handle;

    auto& stored = (m_languages[name.str()] = std::move(config));

    write_to_debug_buffer(format("tree-sitter: loaded grammar '{}' with {} captures",
                                 name, capture_count));
    return &stored;
}

} // namespace Kakoune
