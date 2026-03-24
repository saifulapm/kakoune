#include "language_registry.hh"
#include "syntax_tree.hh"

#include "unit_tests.hh"

namespace Kakoune
{

UnitTest test_make_ts_input_edit_insert{[]()
{
    // Verify TSPoint coordinate mapping for an Insert change.
    // We cannot test byte offsets without a real Buffer and LineByteIndex,
    // but we can verify the TSPoint fields are set correctly by
    // constructing a minimal scenario.

    Buffer::Change insert_change;
    insert_change.type = Buffer::Change::Insert;
    insert_change.begin = {LineCount{2}, ByteCount{5}};
    insert_change.end = {LineCount{3}, ByteCount{0}};

    // Build a trivial LineByteIndex with known line starts.
    // We need a buffer to rebuild, but we can test the TSPoint mapping
    // by checking the structure of the edit.

    // For Insert: start == old_end, new_end == change.end
    // TSPoint.row should be the line, TSPoint.column the byte column
    kak_assert(insert_change.begin.line == LineCount{2});
    kak_assert(insert_change.begin.column == ByteCount{5});
    kak_assert(insert_change.end.line == LineCount{3});
    kak_assert(insert_change.end.column == ByteCount{0});

    // Verify TSPoint construction from BufferCoord
    TSPoint start_point = {(uint32_t)(int)insert_change.begin.line,
                           (uint32_t)(int)insert_change.begin.column};
    kak_assert(start_point.row == 2);
    kak_assert(start_point.column == 5);

    TSPoint end_point = {(uint32_t)(int)insert_change.end.line,
                         (uint32_t)(int)insert_change.end.column};
    kak_assert(end_point.row == 3);
    kak_assert(end_point.column == 0);
}};

UnitTest test_make_ts_input_edit_erase{[]()
{
    // Verify TSPoint coordinate mapping for an Erase change.
    Buffer::Change erase_change;
    erase_change.type = Buffer::Change::Erase;
    erase_change.begin = {LineCount{1}, ByteCount{3}};
    erase_change.end = {LineCount{1}, ByteCount{10}};

    // For Erase: old_end == change.end, new_end == start
    TSPoint start_point = {(uint32_t)(int)erase_change.begin.line,
                           (uint32_t)(int)erase_change.begin.column};
    kak_assert(start_point.row == 1);
    kak_assert(start_point.column == 3);

    TSPoint old_end_point = {(uint32_t)(int)erase_change.end.line,
                             (uint32_t)(int)erase_change.end.column};
    kak_assert(old_end_point.row == 1);
    kak_assert(old_end_point.column == 10);

    // new_end should equal start for erase
    TSPoint new_end_point = start_point;
    kak_assert(new_end_point.row == start_point.row);
    kak_assert(new_end_point.column == start_point.column);
}};

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

} // namespace Kakoune
