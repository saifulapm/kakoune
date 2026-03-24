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

UnitTest test_query_predicate_defaults{[]()
{
    QueryPredicate pred;
    pred.type = PredicateType::Eq;
    pred.capture_id = 0;
    pred.value = "test";
    kak_assert(pred.type == PredicateType::Eq);
    kak_assert(pred.capture_id == 0);
    kak_assert(pred.value == "test");
    kak_assert(not pred.capture_id2);
    kak_assert(pred.values.empty());
    kak_assert(not pred.regex);

    QueryPredicate match_pred;
    match_pred.type = PredicateType::Match;
    match_pred.capture_id = 1;
    match_pred.regex = Regex{"^[A-Z]+$"};
    kak_assert(match_pred.type == PredicateType::Match);
    kak_assert((bool)match_pred.regex);

    QueryPredicate anyof_pred;
    anyof_pred.type = PredicateType::AnyOf;
    anyof_pred.capture_id = 2;
    anyof_pred.values = {"if", "else", "while"};
    kak_assert(anyof_pred.values.size() == 3);
}};

} // namespace Kakoune
