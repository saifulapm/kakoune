#include "language_registry.hh"
#include "shared_string.hh"
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
    // Upstream detection names must resolve to helix grammar names
    kak_assert(LanguageRegistry::filetype_to_language("makefile") == "make");
    kak_assert(LanguageRegistry::filetype_to_language("make") == "make");
}};

UnitTest test_injection_pattern_defaults{[]()
{
    InjectionPattern pattern;
    kak_assert(pattern.language.empty());
    kak_assert(pattern.combined == false);
    kak_assert(pattern.include_children == false);
    kak_assert(pattern.include_unnamed_children == false);

    InjectionPattern with_lang;
    with_lang.language = "rust";
    with_lang.combined = true;
    with_lang.include_children = true;
    kak_assert(with_lang.language == "rust");
    kak_assert(with_lang.combined == true);
    kak_assert(with_lang.include_children == true);
}};

UnitTest test_line_byte_index_byte_offset{[]()
{
    auto make_lines = [](auto&&... lines) { return BufferLines{StringData::create(lines)...}; };

    Buffer buffer("ts_test", Buffer::Flags::None,
                  make_lines("foo\n", "barbaz\n", "qux\n"));
    LineByteIndex index;
    index.rebuild(buffer);

    // Line starts
    kak_assert(index.byte_offset({LineCount{0}, ByteCount{0}}) == 0);
    kak_assert(index.byte_offset({LineCount{1}, ByteCount{0}}) == 4);
    kak_assert(index.byte_offset({LineCount{2}, ByteCount{0}}) == 11);

    // Within a line
    kak_assert(index.byte_offset({LineCount{1}, ByteCount{3}}) == 7);
    // The newline itself
    kak_assert(index.byte_offset({LineCount{2}, ByteCount{3}}) == 14);

    // end_coord()-style coordinate (line_count, 0) is one past the last
    // line: it must resolve to the total byte count, not the start of the
    // last line (a whole-trailing-line erase records exactly this coord).
    kak_assert(index.byte_offset({LineCount{3}, ByteCount{0}}) == 15);
    kak_assert(index.byte_offset({LineCount{42}, ByteCount{7}}) == 15);

    // Out-of-line columns are clamped to the line length
    kak_assert(index.byte_offset({LineCount{0}, ByteCount{100}}) == 4);
    kak_assert(index.byte_offset({LineCount{2}, ByteCount{100}}) == 15);
}};

UnitTest test_is_valid_language_name{[]()
{
    kak_assert(LanguageRegistry::is_valid_language_name("rust"));
    kak_assert(LanguageRegistry::is_valid_language_name("c-sharp"));
    kak_assert(LanguageRegistry::is_valid_language_name("markdown.inline"));
    kak_assert(LanguageRegistry::is_valid_language_name("c++"));
    kak_assert(LanguageRegistry::is_valid_language_name("jinja2"));

    kak_assert(not LanguageRegistry::is_valid_language_name(""));
    kak_assert(not LanguageRegistry::is_valid_language_name("../../evil"));
    kak_assert(not LanguageRegistry::is_valid_language_name("a/b"));
    kak_assert(not LanguageRegistry::is_valid_language_name("a\\b"));
    kak_assert(not LanguageRegistry::is_valid_language_name("name with space"));
    kak_assert(not LanguageRegistry::is_valid_language_name("lang\nname"));
    kak_assert(not LanguageRegistry::is_valid_language_name(String{'x', CharCount{65}}));
}};

UnitTest test_language_for_shebang{[]()
{
    kak_assert(LanguageRegistry::language_for_shebang("#!/usr/bin/env python\nprint(1)\n") == "python");
    kak_assert(LanguageRegistry::language_for_shebang("#!/bin/sh\necho hi\n") == "bash");
    // trailing digits are not part of the interpreter name
    kak_assert(LanguageRegistry::language_for_shebang("#!/usr/bin/python3\n") == "python");
    kak_assert(LanguageRegistry::language_for_shebang("#!/usr/bin/env -S deno run\nfoo\n") == "deno");
    // a shebang on the second line is still found
    kak_assert(LanguageRegistry::language_for_shebang("\n#!/bin/bash\n") == "bash");
    // ... but not past the first two lines
    kak_assert(LanguageRegistry::language_for_shebang("echo hi\necho ho\n#!/bin/sh\n").empty());
    kak_assert(LanguageRegistry::language_for_shebang("no shebang here").empty());
    kak_assert(LanguageRegistry::language_for_shebang("").empty());
}};

UnitTest test_language_for_filename{[]()
{
    kak_assert(LanguageRegistry::language_for_filename("build.py") == "python");
    kak_assert(LanguageRegistry::language_for_filename("/path/to/main.rs") == "rust");
    kak_assert(LanguageRegistry::language_for_filename("component.TSX") == "tsx");
    // extension-less files fall back to the (lowercased) basename
    kak_assert(LanguageRegistry::language_for_filename("Dockerfile") == "dockerfile");
    kak_assert(LanguageRegistry::language_for_filename("").empty());
    kak_assert(LanguageRegistry::language_for_filename("dir/").empty());
    kak_assert(LanguageRegistry::language_for_filename("trailing-dot.").empty());
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
    kak_assert(pred.match_all);  // #eq?/#match? default to match-all semantics

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

UnitTest test_clip_to_included_ranges{[]()
{
    // Two included ranges on lines 0 and 2, with host text in-between:
    // bytes [10, 20) at line 0 and [30, 40) at line 2.
    const TSRange ranges[] = {
        {{0, 10}, {0, 20}, 10, 20},
        {{2, 0}, {2, 10}, 30, 40},
    };
    Vector<TSRange, MemoryDomain::Highlight> out;

    // A capture spanning the gap is split into one segment per range,
    // truncated at the range boundaries.
    clip_to_included_ranges(15, 35, {0, 15}, {2, 5}, ranges, out);
    kak_assert(out.size() == 2);
    kak_assert(out[0].start_byte == 15 and out[0].end_byte == 20);
    kak_assert(out[0].start_point.row == 0 and out[0].start_point.column == 15);
    kak_assert(out[0].end_point.row == 0 and out[0].end_point.column == 20);
    kak_assert(out[1].start_byte == 30 and out[1].end_byte == 35);
    kak_assert(out[1].start_point.row == 2 and out[1].start_point.column == 0);
    kak_assert(out[1].end_point.row == 2 and out[1].end_point.column == 5);

    // A capture entirely inside one range is passed through unchanged.
    out.clear();
    clip_to_included_ranges(12, 18, {0, 12}, {0, 18}, ranges, out);
    kak_assert(out.size() == 1);
    kak_assert(out[0].start_byte == 12 and out[0].end_byte == 18);
    kak_assert(out[0].start_point.column == 12 and out[0].end_point.column == 18);

    // A capture entirely in the gap between ranges produces nothing.
    out.clear();
    clip_to_included_ranges(22, 28, {1, 0}, {1, 6}, ranges, out);
    kak_assert(out.empty());

    // A zero-length intersection at a range boundary produces nothing.
    out.clear();
    clip_to_included_ranges(20, 25, {0, 20}, {1, 3}, ranges, out);
    kak_assert(out.empty());
}};

UnitTest test_sort_and_merge_included_ranges{[]()
{
    using Ranges = Vector<TSRange, MemoryDomain::Highlight>;

    // Out-of-order disjoint ranges are sorted.
    Ranges sorted = {
        {{2, 0}, {2, 10}, 30, 40},
        {{0, 10}, {0, 20}, 10, 20},
    };
    sort_and_merge_included_ranges(sorted);
    kak_assert(sorted.size() == 2);
    kak_assert(sorted[0].start_byte == 10 and sorted[0].end_byte == 20);
    kak_assert(sorted[1].start_byte == 30 and sorted[1].end_byte == 40);

    // Overlapping and touching ranges are merged; the merged range keeps the
    // earliest start point and the latest end point.
    Ranges merged = {
        {{0, 10}, {0, 20}, 10, 20},
        {{0, 15}, {1, 5}, 15, 25},   // overlaps [10,20)
        {{1, 5}, {1, 10}, 25, 30},   // touches [15,25)
        {{3, 0}, {3, 5}, 50, 55},    // disjoint
    };
    sort_and_merge_included_ranges(merged);
    kak_assert(merged.size() == 2);
    kak_assert(merged[0].start_byte == 10 and merged[0].end_byte == 30);
    kak_assert(merged[0].start_point.row == 0 and merged[0].start_point.column == 10);
    kak_assert(merged[0].end_point.row == 1 and merged[0].end_point.column == 10);
    kak_assert(merged[1].start_byte == 50 and merged[1].end_byte == 55);

    // A duplicate range collapses to one; a nested range disappears into its
    // container (end point stays the container's).
    Ranges dupes = {
        {{0, 0}, {5, 0}, 0, 100},
        {{0, 0}, {5, 0}, 0, 100},
        {{1, 0}, {2, 0}, 20, 40},
    };
    sort_and_merge_included_ranges(dupes);
    kak_assert(dupes.size() == 1);
    kak_assert(dupes[0].start_byte == 0 and dupes[0].end_byte == 100);
    kak_assert(dupes[0].end_point.row == 5 and dupes[0].end_point.column == 0);

    // Empty and single-element inputs pass through unchanged.
    Ranges empty;
    sort_and_merge_included_ranges(empty);
    kak_assert(empty.empty());
    Ranges single = {{{0, 0}, {0, 5}, 0, 5}};
    sort_and_merge_included_ranges(single);
    kak_assert(single.size() == 1 and single[0].end_byte == 5);
}};

UnitTest test_tree_sitter_disabled_flag{[]()
{
    auto make_lines = [](auto&&... lines) { return BufferLines{StringData::create(lines)...}; };
    Buffer buffer("ts_disabled_test", Buffer::Flags::None,
                  make_lines("int x;\n"));

    kak_assert(not is_tree_sitter_disabled(buffer));
    set_tree_sitter_disabled(buffer, true);
    kak_assert(is_tree_sitter_disabled(buffer));

    // While disabled, sync_async_host_tree must not (re)create any trees —
    // it returns before even touching the config, so null is safe here.
    kak_assert(sync_async_host_tree(buffer, nullptr) == nullptr);
    kak_assert(not has_syntax_tree(buffer));
    kak_assert(not has_async_syntax_tree(buffer));

    set_tree_sitter_disabled(buffer, false);
    kak_assert(not is_tree_sitter_disabled(buffer));
}};

UnitTest test_resolve_capture_precedence{[]()
{
    using Slots = Vector<CaptureSlot, MemoryDomain::Highlight>;

    // Same node captured by two patterns: the later pattern wins (replace,
    // not merge) — the tree-house/Neovim/Zed convention helix queries rely on.
    Slots same_node{{10, 20, 0}, {10, 20, 1}};
    resolve_capture_precedence(same_node);
    kak_assert(same_node.size() == 1);
    kak_assert(same_node[0].index == 1);

    // Parent captured by a later pattern than a child starting at the same
    // byte (css `"#" @punctuation` vs later `((color_value) "#") @string.special`):
    // the parent must be applied first so the child's face stays on top.
    Slots nested{{10, 12, 0}, {10, 20, 1}};
    resolve_capture_precedence(nested);
    kak_assert(nested.size() == 2);
    kak_assert(nested[0].index == 1 and nested[1].index == 0);

    // A child fully inside a parent (distinct start) also comes after it.
    Slots inner{{12, 15, 1}, {10, 20, 0}};
    resolve_capture_precedence(inner);
    kak_assert(inner.size() == 2);
    kak_assert(inner[0].index == 0 and inner[1].index == 1);

    // Disjoint captures are ordered by buffer position.
    Slots disjoint{{30, 40, 0}, {10, 20, 1}};
    resolve_capture_precedence(disjoint);
    kak_assert(disjoint[0].index == 1 and disjoint[1].index == 0);

    // Mixed: three patterns on one node plus a nested child — the last
    // same-node pattern survives, the child stays on top of it.
    Slots mixed{{10, 20, 0}, {10, 20, 1}, {10, 15, 2}, {10, 20, 3}};
    resolve_capture_precedence(mixed);
    kak_assert(mixed.size() == 2);
    kak_assert(mixed[0].index == 3 and mixed[1].index == 2);

    // Empty input is fine.
    Slots empty;
    resolve_capture_precedence(empty);
    kak_assert(empty.empty());
}};

} // namespace Kakoune
