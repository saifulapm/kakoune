# Predicate Support Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Evaluate `#eq?`, `#not-eq?`, `#match?`, `#not-match?`, `#any-of?`, `#not-any-of?` predicates so tree-sitter queries filter matches correctly.

**Architecture:** Parse predicates at grammar load time in `language_registry.cc`, store per-pattern alongside each query. Evaluate with a `predicates_match()` function called at every `next_match`/`next_capture` loop site. Regex uses Kakoune's built-in `Regex`.

**Tech Stack:** C++17, tree-sitter C API, Kakoune's `Regex` from `regex.hh`

**Spec:** `docs/superpowers/specs/2026-03-25-predicate-support-design.md`

---

### Task 1: Data structures in `language_registry.hh`

**Files:**
- Modify: `src/language_registry.hh:1-89`

- [ ] **Step 1: Add `#include "regex.hh"` and `#include "optional.hh"`**

After line 9 (`#include "tree_sitter.hh"`):
```cpp
#include "optional.hh"
#include "regex.hh"
```

- [ ] **Step 2: Add `PredicateType` enum and `QueryPredicate` struct**

After the `InjectionPattern` struct (after line 23), add:
```cpp
enum class PredicateType { Eq, NotEq, Match, NotMatch, AnyOf, NotAnyOf };

struct QueryPredicate
{
    PredicateType type;
    uint32_t capture_id;            // capture to test
    String value;                   // literal string (Eq/NotEq)
    Optional<uint32_t> capture_id2; // second capture (capture-vs-capture Eq/NotEq)
    Vector<String> values;          // string set (AnyOf/NotAnyOf)
    Optional<Regex> regex;          // compiled regex (Match/NotMatch)
};

using PatternPredicates = Vector<Vector<QueryPredicate>>;
```

- [ ] **Step 3: Add `PatternPredicates` fields and getters to `LanguageConfig`**

In the public section, after line 49 (`TSQuery* locals_query()`), add:
```cpp
const PatternPredicates& highlight_predicates() const { return m_highlight_predicates; }
const PatternPredicates& injection_predicates() const { return m_injection_predicates; }
const PatternPredicates& textobject_predicates() const { return m_textobject_predicates; }
const PatternPredicates& indent_predicates() const { return m_indent_predicates; }
const PatternPredicates& locals_predicates() const { return m_locals_predicates; }
```

In the private section, after line 67 (`TSQuery* m_locals_query`), add:
```cpp
PatternPredicates m_highlight_predicates;
PatternPredicates m_injection_predicates;
PatternPredicates m_textobject_predicates;
PatternPredicates m_indent_predicates;
PatternPredicates m_locals_predicates;
```

- [ ] **Step 4: Declare `parse_query_predicates` and `predicates_match` free functions**

Before the `LanguageRegistry` class (before line 70), add:
```cpp
PatternPredicates parse_query_predicates(const TSQuery* query);
bool predicates_match(const Vector<QueryPredicate>& predicates,
                      const TSQueryMatch& match,
                      const Buffer& buffer);
```

This requires a forward declaration of `Buffer`. Add after the namespace opening (after line 14):
```cpp
class Buffer;
```

- [ ] **Step 5: Build to verify header compiles**

Run: `make -j$(sysctl -n hw.ncpu) 2>&1 | head -30`
Expected: Clean build (or only unrelated warnings)

- [ ] **Step 6: Commit**

```bash
git add src/language_registry.hh
git commit -m "feat: add QueryPredicate data structures for tree-sitter predicates"
```

---

### Task 2: Parse predicates at load time in `language_registry.cc`

**Files:**
- Modify: `src/language_registry.cc:37-103` (move ctor/assignment), `src/language_registry.cc:135-420` (load_language)

- [ ] **Step 1: Update move constructor to include predicate fields**

In the move constructor (line 37), add to the member initializer list after `m_locals_query`:
```cpp
m_highlight_predicates(std::move(other.m_highlight_predicates)),
m_injection_predicates(std::move(other.m_injection_predicates)),
m_textobject_predicates(std::move(other.m_textobject_predicates)),
m_indent_predicates(std::move(other.m_indent_predicates)),
m_locals_predicates(std::move(other.m_locals_predicates))
```

- [ ] **Step 2: Update move assignment operator**

In the move assignment operator (line 62), add the corresponding moves in the assignment section and clear in the source section.

- [ ] **Step 3: Implement `parse_query_predicates()`**

Add before the `load_language` method:
```cpp
PatternPredicates parse_query_predicates(const TSQuery* query)
{
    PatternPredicates result;
    if (not query)
        return result;

    uint32_t pattern_count = ts_query_pattern_count(query);
    result.resize((int)pattern_count);

    for (uint32_t p = 0; p < pattern_count; ++p)
    {
        uint32_t step_count = 0;
        const TSQueryPredicateStep* steps =
            ts_query_predicates_for_pattern(query, p, &step_count);

        // Split by Done sentinel into individual predicates
        uint32_t pred_start = 0;
        for (uint32_t s = 0; s <= step_count; ++s)
        {
            if (s == step_count or steps[s].type == TSQueryPredicateStepTypeDone)
            {
                if (s > pred_start)
                    parse_single_predicate(query, steps + pred_start,
                                           s - pred_start, result[(int)p]);
                pred_start = s + 1;
            }
        }
    }
    return result;
}
```

- [ ] **Step 4: Implement `parse_single_predicate()` helper**

Add as a static function before `parse_query_predicates`:
```cpp
static void parse_single_predicate(const TSQuery* query,
                                   const TSQueryPredicateStep* steps,
                                   uint32_t count,
                                   Vector<QueryPredicate>& out)
{
    if (count < 2 or steps[0].type != TSQueryPredicateStepTypeString)
        return;

    uint32_t name_len = 0;
    const char* name_str = ts_query_string_value_for_id(
        query, steps[0].value_id, &name_len);
    StringView name{name_str, (ByteCount)name_len};

    // Helper to get capture ID from a step
    auto get_capture = [](const TSQueryPredicateStep& step) -> Optional<uint32_t> {
        if (step.type == TSQueryPredicateStepTypeCapture)
            return step.value_id;
        return {};
    };

    // Helper to get string value from a step
    auto get_string = [&](const TSQueryPredicateStep& step) -> Optional<String> {
        if (step.type == TSQueryPredicateStepTypeString)
        {
            uint32_t len = 0;
            const char* str = ts_query_string_value_for_id(query, step.value_id, &len);
            return String{str, (ByteCount)len};
        }
        return {};
    };

    if ((name == "eq?" or name == "not-eq?") and count >= 3)
    {
        auto cap = get_capture(steps[1]);
        if (not cap)
            return;

        QueryPredicate pred;
        pred.type = (name == "eq?") ? PredicateType::Eq : PredicateType::NotEq;
        pred.capture_id = *cap;

        // Second arg can be string or capture
        if (steps[2].type == TSQueryPredicateStepTypeString)
            pred.value = *get_string(steps[2]);
        else if (steps[2].type == TSQueryPredicateStepTypeCapture)
            pred.capture_id2 = steps[2].value_id;
        else
            return;

        out.push_back(std::move(pred));
    }
    else if ((name == "match?" or name == "not-match?") and count >= 3)
    {
        auto cap = get_capture(steps[1]);
        auto pattern = get_string(steps[2]);
        if (not cap or not pattern)
            return;

        try
        {
            QueryPredicate pred;
            pred.type = (name == "match?") ? PredicateType::Match : PredicateType::NotMatch;
            pred.capture_id = *cap;
            pred.regex = Regex{*pattern};
            out.push_back(std::move(pred));
        }
        catch (regex_error&)
        {
            write_to_debug_buffer(format("tree-sitter: invalid regex '{}' in predicate, skipping",
                                         *pattern));
        }
    }
    else if ((name == "any-of?" or name == "not-any-of?") and count >= 3)
    {
        auto cap = get_capture(steps[1]);
        if (not cap)
            return;

        QueryPredicate pred;
        pred.type = (name == "any-of?") ? PredicateType::AnyOf : PredicateType::NotAnyOf;
        pred.capture_id = *cap;
        for (uint32_t i = 2; i < count; ++i)
        {
            auto str = get_string(steps[i]);
            if (str)
                pred.values.push_back(std::move(*str));
        }
        out.push_back(std::move(pred));
    }
    else if (name != "set!")
    {
        write_to_debug_buffer(format("tree-sitter: unknown predicate '{}', skipping", name));
    }
}
```

- [ ] **Step 5: Call `parse_query_predicates` in `load_language`**

After each query is compiled, add the parse call. In `load_language`:

After highlights query compilation (~line 237):
```cpp
config.m_highlight_predicates = parse_query_predicates(config.m_highlight_query);
```

After injection query compilation (~line 268, after the existing `#set!` parsing block ends at line 324):
```cpp
config.m_injection_predicates = parse_query_predicates(config.m_injection_query);
```

After textobjects query compilation (~line 362):
```cpp
config.m_textobject_predicates = parse_query_predicates(config.m_textobject_query);
```

After indents query compilation (~line 384):
```cpp
config.m_indent_predicates = parse_query_predicates(config.m_indent_query);
```

After locals query compilation (~line 409):
```cpp
config.m_locals_predicates = parse_query_predicates(config.m_locals_query);
```

- [ ] **Step 6: Build to verify parsing compiles**

Run: `make -j$(sysctl -n hw.ncpu) 2>&1 | head -30`
Expected: Clean build

- [ ] **Step 7: Commit**

```bash
git add src/language_registry.cc
git commit -m "feat: parse tree-sitter query predicates at grammar load time"
```

---

### Task 3: Implement `predicates_match()` evaluation function

**Files:**
- Modify: `src/language_registry.cc`

- [ ] **Step 1: Implement `node_text_for_predicate()` helper**

Add a static helper that properly handles multi-line nodes (needed for predicate text extraction). Add before `parse_single_predicate`:
```cpp
static String node_text_for_predicate(TSNode node, const Buffer& buffer)
{
    const TSPoint start = ts_node_start_point(node);
    const TSPoint end = ts_node_end_point(node);

    if (start.row >= (uint32_t)(int)buffer.line_count())
        return {};

    // Fast path: single-line node (common case for identifiers/keywords)
    if (start.row == end.row)
    {
        const StringView line = buffer[LineCount{(int)start.row}];
        auto col_start = ByteCount{(int)start.column};
        auto col_end = ByteCount{(int)end.column};
        if (col_end > line.length())
            col_end = line.length();
        if (col_start >= col_end)
            return {};
        return line.substr(col_start, col_end - col_start).str();
    }

    // Multi-line: concatenate
    String result;
    for (uint32_t row = start.row; row <= end.row
         and row < (uint32_t)(int)buffer.line_count(); ++row)
    {
        const StringView line = buffer[LineCount{(int)row}];
        const auto col_start = ByteCount{(row == start.row) ? (int)start.column : 0};
        const auto col_end = (row == end.row)
            ? ByteCount{(int)end.column}
            : line.length();
        if (col_start < col_end and col_end <= line.length())
            result += line.substr(col_start, col_end - col_start);
    }
    return result;
}
```

- [ ] **Step 2: Implement `predicates_match()`**

Add after `parse_query_predicates`:
```cpp
bool predicates_match(const Vector<QueryPredicate>& predicates,
                      const TSQueryMatch& match,
                      const Buffer& buffer)
{
    for (const auto& pred : predicates)
    {
        // Find the captured node for this predicate
        TSNode node = {};
        bool found = false;
        for (uint16_t c = 0; c < match.capture_count; ++c)
        {
            if (match.captures[c].index == pred.capture_id)
            {
                node = match.captures[c].node;
                found = true;
                break;
            }
        }
        if (not found)
            return false;

        String text = node_text_for_predicate(node, buffer);

        switch (pred.type)
        {
        case PredicateType::Eq:
            if (pred.capture_id2)
            {
                // capture-vs-capture: find second capture
                bool found2 = false;
                String text2;
                for (uint16_t c = 0; c < match.capture_count; ++c)
                {
                    if (match.captures[c].index == *pred.capture_id2)
                    {
                        text2 = node_text_for_predicate(match.captures[c].node, buffer);
                        found2 = true;
                        break;
                    }
                }
                if (not found2 or text != text2)
                    return false;
            }
            else if (text != pred.value)
                return false;
            break;

        case PredicateType::NotEq:
            if (pred.capture_id2)
            {
                bool found2 = false;
                String text2;
                for (uint16_t c = 0; c < match.capture_count; ++c)
                {
                    if (match.captures[c].index == *pred.capture_id2)
                    {
                        text2 = node_text_for_predicate(match.captures[c].node, buffer);
                        found2 = true;
                        break;
                    }
                }
                if (not found2)
                    return false;  // missing capture -> conservative fail
                if (text == text2)
                    return false;
            }
            else if (text == pred.value)
                return false;
            break;

        case PredicateType::Match:
            if (pred.regex and not regex_search(text.begin(), text.end(),
                                                text.begin(), text.end(), *pred.regex))
                return false;
            break;

        case PredicateType::NotMatch:
            if (pred.regex and regex_search(text.begin(), text.end(),
                                            text.begin(), text.end(), *pred.regex))
                return false;
            break;

        case PredicateType::AnyOf:
        {
            bool in_set = false;
            for (const auto& v : pred.values)
            {
                if (text == v)
                {
                    in_set = true;
                    break;
                }
            }
            if (not in_set)
                return false;
            break;
        }

        case PredicateType::NotAnyOf:
        {
            for (const auto& v : pred.values)
            {
                if (text == v)
                    return false;
            }
            break;
        }
        }
    }
    return true;
}
```

- [ ] **Step 3: Add required includes to `language_registry.cc`**

Ensure these are at the top of `language_registry.cc`:
```cpp
#include "buffer.hh"
#include "regex.hh"
```

- [ ] **Step 4: Build**

Run: `make -j$(sysctl -n hw.ncpu) 2>&1 | head -30`
Expected: Clean build

- [ ] **Step 5: Commit**

```bash
git add src/language_registry.cc
git commit -m "feat: implement predicates_match() evaluation function"
```

---

### Task 4: Unit tests for parsing and evaluation

**Files:**
- Modify: `src/tree_sitter_tests.cc`

- [ ] **Step 1: Add test for `PredicateType` enum and `QueryPredicate` defaults**

```cpp
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
```

- [ ] **Step 2: Add `#include "regex.hh"` to test file if not present**

- [ ] **Step 3: Build and run tests**

Run: `make -j$(sysctl -n hw.ncpu) 2>&1 | head -30 && ./src/.kak_tests 2>&1 | tail -20`
Expected: All tests pass (including new ones)

- [ ] **Step 4: Commit**

```bash
git add src/tree_sitter_tests.cc
git commit -m "test: add unit tests for QueryPredicate data structures"
```

---

### Task 5: Integrate predicates in `highlighters.cc`

**Files:**
- Modify: `src/highlighters.cc:2510` (locals loop), `src/highlighters.cc:2620-2666` (highlight loop)

- [ ] **Step 1: Add predicate filtering to `build_local_references` locals loop**

At line 2510, change the match loop to:
```cpp
    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match))
    {
        // Filter by predicates
        if (match.pattern_index < (uint32_t)locals_preds.size()
            and not locals_preds[(int)match.pattern_index].empty()
            and not predicates_match(locals_preds[(int)match.pattern_index], match, buffer))
            continue;

        for (uint16_t c = 0; c < match.capture_count; ++c)
        {
```

This requires passing `locals_preds` to `build_local_references`. The function needs the `LanguageConfig*` — check how it's called and thread the predicates through. The caller at `do_highlight` (line 2668) has access to `config` via `LanguageRegistry::instance().get(...)`.

Add parameter to `build_local_references` signature:
```cpp
static HashSet<uint32_t> build_local_references(
    TSQuery* locals_query, TSNode root, const Buffer& buffer,
    uint32_t start_byte, uint32_t end_byte,
    const PatternPredicates& locals_preds)
```

Update the call site in `do_highlight` (~line 2697) to pass `config->locals_predicates()`.

- [ ] **Step 2: Add predicate filtering to `run_highlights` capture loop**

The `run_highlights` method (line 2620) needs buffer access and highlight predicates. Add parameters:
```cpp
void run_highlights(TSQuery* query, TSNode root,
                    const Vector<String>& capture_faces,
                    uint32_t start_byte, uint32_t end_byte,
                    const HashSet<uint32_t>& local_refs,
                    const PatternPredicates& highlight_preds,
                    const Buffer& buffer,
                    HighlightContext& context, DisplayBuffer& display_buffer)
```

In the capture loop at line 2635, add after getting the match:
```cpp
        while (ts_query_cursor_next_capture(m_cursor, &match, &capture_index))
        {
            // Filter by predicates
            if (match.pattern_index < (uint32_t)highlight_preds.size()
                and not highlight_preds[(int)match.pattern_index].empty()
                and not predicates_match(highlight_preds[(int)match.pattern_index], match, buffer))
            {
                ts_query_cursor_remove_match(m_cursor, match.id);
                continue;
            }

            const TSQueryCapture& capture = match.captures[capture_index];
```

Update the call site in `do_highlight` to pass `config->highlight_predicates()` and `buffer`.

- [ ] **Step 3: Update injection layer call sites**

In `do_highlight` at ~line 2733, the injection layer loop also calls `build_local_references` and `run_highlights`. Update these to pass the injection layer's own predicates:

```cpp
        for (auto& layer : syntax_tree.injection_layers())
        {
            // ...existing null checks...

            HashSet<uint32_t> inj_local_refs;
            if (layer.config->locals_query())
                inj_local_refs = build_local_references(layer.config->locals_query(),
                                                        ts_tree_root_node(layer.tree),
                                                        buffer, start_byte, end_byte,
                                                        layer.config->locals_predicates());

            run_highlights(inj_query,
                           ts_tree_root_node(layer.tree),
                           inj_faces,
                           start_byte, end_byte,
                           inj_local_refs,
                           layer.config->highlight_predicates(),
                           buffer,
                           context, display_buffer);
        }
```

- [ ] **Step 4: Build and test**

Run: `make -j$(sysctl -n hw.ncpu) 2>&1 | head -30`
Expected: Clean build

- [ ] **Step 5: Commit**

```bash
git add src/highlighters.cc
git commit -m "feat: filter highlights and locals by query predicates"
```

---

### Task 6: Integrate predicates in `syntax_tree.cc`

**Files:**
- Modify: `src/syntax_tree.cc:349` (injection detection loop)

- [ ] **Step 1: Add predicate filtering to injection detection**

In `detect_injections()` at line 349, the injection predicates need to come from the config. The function already has access to `cfg` and `buffer`.

After the existing `const auto& patterns = cfg->injection_patterns();` (line 346), add:
```cpp
const auto& inj_preds = cfg->injection_predicates();
```

In the match loop at line 349:
```cpp
        while (ts_query_cursor_next_match(cursor, &match))
        {
            // Filter by predicates
            if (match.pattern_index < (uint32_t)inj_preds.size()
                and not inj_preds[(int)match.pattern_index].empty()
                and not predicates_match(inj_preds[(int)match.pattern_index], match, buffer))
                continue;

            TSNode content_node = {};
```

- [ ] **Step 2: Add `#include "buffer.hh"` if not already present in `syntax_tree.cc`**

Check existing includes and add if needed.

- [ ] **Step 3: Build**

Run: `make -j$(sysctl -n hw.ncpu) 2>&1 | head -30`
Expected: Clean build

- [ ] **Step 4: Commit**

```bash
git add src/syntax_tree.cc
git commit -m "feat: filter injection detection by query predicates"
```

---

### Task 7: Integrate predicates in `commands.cc` — text object commands

**Files:**
- Modify: `src/commands.cc` — 7 match/capture loops for text objects, fold, context, indent, inspection

- [ ] **Step 1: Add predicate filtering to `tree_select_cmd` (line 2946)**

The config is available via `syntax_tree.config()`. Add:
```cpp
const auto& to_preds = syntax_tree.config()->textobject_predicates();
```

In the match loop:
```cpp
            while (ts_query_cursor_next_match(qcursor, &match))
            {
                if (match.pattern_index < (uint32_t)to_preds.size()
                    and not to_preds[(int)match.pattern_index].empty()
                    and not predicates_match(to_preds[(int)match.pattern_index], match, buffer))
                    continue;
```

- [ ] **Step 2: Add predicate filtering to `tree_select_next_cmd` (line 3023)**

Same pattern — get `textobject_predicates()`, filter in loop.

- [ ] **Step 3: Add predicate filtering to `tree_select_prev_cmd` (line 3095)**

Same pattern.

- [ ] **Step 4: Add predicate filtering to `tree_select_all_cmd` (line 3442)**

Same pattern.

- [ ] **Step 5: Add predicate filtering to `tree_filter_cmd` (line 3489)**

Same pattern.

- [ ] **Step 6: Add predicate filtering to `tree_fold_cmd` (line 3697)**

Same pattern — uses textobject query.

- [ ] **Step 7: Add predicate filtering to `tree_update_context_cmd` (line 3809)**

Uses textobject query. Same pattern.

- [ ] **Step 8: Add predicate filtering to `tree_indent_cmd` (line 3891)**

Uses indent query, next_capture loop. Get `indent_predicates()`:
```cpp
const auto& ind_preds = syntax_tree.config()->indent_predicates();
```

In the capture loop, use `ts_query_cursor_remove_match` since it's `next_capture`:
```cpp
            while (ts_query_cursor_next_capture(cursor, &match, &capture_index))
            {
                if (match.pattern_index < (uint32_t)ind_preds.size()
                    and not ind_preds[(int)match.pattern_index].empty()
                    and not predicates_match(ind_preds[(int)match.pattern_index], match, buffer))
                {
                    ts_query_cursor_remove_match(cursor, match.id);
                    continue;
                }
```

- [ ] **Step 9: Add predicate filtering to `tree_indent_newline_cmd` (line 4026)**

Same pattern as `tree_indent_cmd` — indent query, next_capture loop.

- [ ] **Step 10: Add predicate filtering to `tree_sitter_highlight_name_cmd` (line 4146)**

Uses highlight query, next_capture loop. Get `highlight_predicates()`:
```cpp
const auto& hl_preds = config->highlight_predicates();
```

With `ts_query_cursor_remove_match` for rejected matches.

- [ ] **Step 11: Build and test**

Run: `make -j$(sysctl -n hw.ncpu) 2>&1 | head -30`
Expected: Clean build

- [ ] **Step 12: Commit**

```bash
git add src/commands.cc
git commit -m "feat: filter all command query loops by predicates"
```

---

### Task 8: Smoke test — verify predicates work end-to-end

**Files:** None (manual testing)

- [ ] **Step 1: Build the editor**

Run: `make -j$(sysctl -n hw.ncpu)`

- [ ] **Step 2: Run unit tests**

Run: `make test 2>&1` or `./src/.kak_tests 2>&1`
Expected: All tests pass

- [ ] **Step 3: Manual smoke test with Rust file**

Open a Rust file (`./tskak src/main.cc` or any `.rs` file). Verify:
- ALL_CAPS identifiers get `@constant` face (from `#match? @constant "^[A-Z][A-Z_0-9]*$"`)
- `_` parameters don't get normal variable highlight (from `#eq?` predicates)
- Keywords like `if`/`else` highlighted correctly where `#any-of?` predicates are used

- [ ] **Step 4: Check debug buffer for predicate errors**

In kakoune: `:buffer *debug*`
Verify no unexpected "invalid regex" warnings.

- [ ] **Step 5: Commit any fixes if needed**

---

### Task 9: Update memory

- [ ] **Step 1: Update project memory with predicate support status**

Update `project_treesitter_kak.md` to move predicates from "What's Next" to "Features Implemented".
