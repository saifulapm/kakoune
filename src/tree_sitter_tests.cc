#include "language_registry.hh"
#include "syntax_tree.hh"

#include "unit_tests.hh"

namespace Kakoune
{

UnitTest test_ts_point_zero_coord{[]()
{
    // Verify zero coordinates map correctly
    BufferCoord origin{LineCount{0}, ByteCount{0}};
    TSPoint point = {(uint32_t)(int)origin.line,
                     (uint32_t)(int)origin.column};
    kak_assert(point.row == 0);
    kak_assert(point.column == 0);
}};

UnitTest test_capture_to_face_name{[]()
{
    kak_assert(capture_to_face_name("keyword") == "ts_keyword");
    kak_assert(capture_to_face_name("constant.builtin") == "ts_constant_builtin");
    kak_assert(capture_to_face_name("punctuation.bracket") == "ts_punctuation_bracket");
}};

UnitTest test_filetype_to_language{[]()
{
    kak_assert(LanguageRegistry::filetype_to_language("sh") == "bash");
    kak_assert(LanguageRegistry::filetype_to_language("bash") == "bash");
    kak_assert(LanguageRegistry::filetype_to_language("zsh") == "bash");
    kak_assert(LanguageRegistry::filetype_to_language("cpp") == "cpp");
    kak_assert(LanguageRegistry::filetype_to_language("objc") == "cpp");
    kak_assert(LanguageRegistry::filetype_to_language("javascript") == "javascript");
    kak_assert(LanguageRegistry::filetype_to_language("jsx") == "javascript");
    kak_assert(LanguageRegistry::filetype_to_language("typescript") == "tsx");
    kak_assert(LanguageRegistry::filetype_to_language("tsx") == "tsx");
    kak_assert(LanguageRegistry::filetype_to_language("rust") == "rust");
    kak_assert(LanguageRegistry::filetype_to_language("python") == "python");
}};

UnitTest test_injection_pattern_defaults{[]()
{
    InjectionPattern pattern;
    kak_assert(pattern.language.empty());
    kak_assert(pattern.combined == false);
    kak_assert(pattern.include_children == false);

    InjectionPattern with_lang;
    with_lang.language = "rust";
    with_lang.combined = true;
    with_lang.include_children = true;
    kak_assert(with_lang.language == "rust");
    kak_assert(with_lang.combined == true);
    kak_assert(with_lang.include_children == true);
}};

} // namespace Kakoune
