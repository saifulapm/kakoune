#ifndef language_registry_hh_INCLUDED
#define language_registry_hh_INCLUDED

#include "hash_map.hh"
#include "string.hh"
#include "unique_ptr.hh"
#include "utils.hh"
#include "vector.hh"
#include "optional.hh"
#include "regex.hh"
#include "tree_sitter.hh"

#include <climits>

namespace Kakoune
{

class Buffer;

String capture_to_face_name(StringView capture_name);

struct InjectionPattern
{
    String language;                // from #set! injection.language "..."
    bool combined = false;          // from #set! injection.combined
    bool include_children = false;  // from #set! injection.include-children
    // from #set! injection.include-unnamed-children: only the content node's
    // NAMED children are excluded from the injected ranges
    bool include_unnamed_children = false;
};

enum class PredicateType { Eq, NotEq, Match, NotMatch, AnyOf, NotAnyOf,
                           NotKindEq, SameLine, NotSameLine, OneLine, NotOneLine };

enum class IndentScope { All, Tail };

struct QueryPredicate
{
    PredicateType type;
    uint32_t capture_id;            // capture to test
    String value;                   // literal string (Eq/NotEq)
    Optional<uint32_t> capture_id2; // second capture (capture-vs-capture Eq/NotEq)
    Vector<String> values;          // string set (AnyOf/NotAnyOf)
    Optional<Regex> regex;          // compiled regex (Match/NotMatch)
    // true: the predicate must hold for ALL nodes captured under capture_id
    // (#eq?/#match?); false: at least one node suffices (#any-eq?/#any-match?,
    // #any-of?). Mirrors tree-house's TextPredicate::match_all.
    bool match_all = true;
};

using PatternPredicates = Vector<Vector<QueryPredicate>>;

PatternPredicates parse_query_predicates(const TSQuery* query);
bool predicates_match(const Vector<QueryPredicate>& predicates,
                      const TSQueryMatch& match,
                      const Buffer& buffer,
                      Optional<uint32_t> new_line_byte_pos = {});
// Standalone-text variant for queries run outside any buffer
// (e.g. tree-sitter-highlight): node byte offsets index into `text`.
bool predicates_match(const Vector<QueryPredicate>& predicates,
                      const TSQueryMatch& match,
                      StringView text);

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
    uint32_t injection_shebang_capture() const { return m_injection_shebang_capture; }
    uint32_t injection_filename_capture() const { return m_injection_filename_capture; }

    TSQuery* textobject_query() const { return m_textobject_query; }
    TSQuery* indent_query() const { return m_indent_query; }
    TSQuery* locals_query() const { return m_locals_query; }

    const PatternPredicates& highlight_predicates() const { return m_highlight_predicates; }
    const PatternPredicates& injection_predicates() const { return m_injection_predicates; }
    const PatternPredicates& textobject_predicates() const { return m_textobject_predicates; }
    const PatternPredicates& indent_predicates() const { return m_indent_predicates; }
    const PatternPredicates& locals_predicates() const { return m_locals_predicates; }

    const Vector<Optional<IndentScope>>& indent_scopes() const { return m_indent_scopes; }

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
    uint32_t m_injection_shebang_capture = UINT32_MAX;
    uint32_t m_injection_filename_capture = UINT32_MAX;

    TSQuery* m_textobject_query = nullptr;
    TSQuery* m_indent_query = nullptr;
    TSQuery* m_locals_query = nullptr;

    PatternPredicates m_highlight_predicates;
    PatternPredicates m_injection_predicates;
    PatternPredicates m_textobject_predicates;
    PatternPredicates m_indent_predicates;
    PatternPredicates m_locals_predicates;

    Vector<Optional<IndentScope>> m_indent_scopes;
};

class LanguageRegistry : public Singleton<LanguageRegistry>
{
public:
    LanguageRegistry(String helix_runtime_dir, String helix_config_dir);

    const LanguageConfig* get(StringView name);

    static StringView filetype_to_language(StringView filetype);

    // True if name is safe to use as a grammar/query path component
    // (conservative charset, no way to escape the grammars directory).
    // Injection language names come straight from document text.
    static bool is_valid_language_name(StringView name);

    // Resolve a "#!/usr/bin/env python"-style shebang (first two lines of
    // text) to a language name; empty string if no shebang is found.
    static String language_for_shebang(StringView text);

    // Resolve a filename to a language name via its extension (or basename
    // for extension-less files); empty string if nothing matches.
    static String language_for_filename(StringView filename);

    const String& helix_runtime_dir() const { return m_helix_runtime_dir; }
    const String& helix_config_dir() const { return m_helix_config_dir; }

private:
    const LanguageConfig* load_language(StringView name);
    String resolve_query_inherits(StringView query_text, StringView query_type);
    String resolve_query_inherits_rec(StringView query_text, StringView query_type,
                                      HashSet<String>& visited);

    String m_helix_runtime_dir;
    String m_helix_config_dir;
    // Store UniquePtr so HashMap reallocation doesn't move/invalidate LanguageConfig objects
    HashMap<String, UniquePtr<LanguageConfig>, MemoryDomain::Highlight> m_languages;
};

} // namespace Kakoune

#endif // language_registry_hh_INCLUDED
