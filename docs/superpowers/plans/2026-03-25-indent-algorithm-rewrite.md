# Indent Algorithm Rewrite Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rewrite tree-indent commands to use Helix's parent-walk algorithm with scope semantics and @indent.always/@outdent.always captures.

**Architecture:** Single `compute_indent_for_line()` function shared by both commands. Collects indent captures into a HashMap keyed by node ID, walks up parents applying scope-based accumulation, handles line transitions with Helix's three-way branch logic.

**Tech Stack:** C++17, tree-sitter C API, Kakoune's Buffer/String types

**Spec:** `docs/superpowers/specs/2026-03-25-indent-algorithm-rewrite-design.md`

---

### Task 1: Add IndentScope enum and scope storage to LanguageConfig

**Files:**
- Modify: `src/language_registry.hh`
- Modify: `src/language_registry.cc`

- [ ] **Step 1: Add IndentScope enum to language_registry.hh**

After the `PredicateType` enum (line ~29), add:
```cpp
enum class IndentScope { All, Tail };
```

- [ ] **Step 2: Add indent_scopes field and getter to LanguageConfig**

Public section, after `locals_predicates()`:
```cpp
const Vector<Optional<IndentScope>>& indent_scopes() const { return m_indent_scopes; }
```

Private section, after `m_locals_predicates`:
```cpp
Vector<Optional<IndentScope>> m_indent_scopes;  // per-pattern scope override from #set! "scope"
```

Uses `Optional` so we can distinguish "no override" from "explicit Tail". `@outdent` defaults to `All` but `@indent` defaults to `Tail` — the per-capture default is applied at evaluation time, not storage time.

- [ ] **Step 3: Parse `#set! "scope"` for indent queries in language_registry.cc**

In the `load_language()` method, after `config.m_indent_predicates = parse_query_predicates(config.m_indent_query);`, add:

```cpp
// Parse #set! "scope" for indent patterns
if (config.m_indent_query)
{
    uint32_t pattern_count = ts_query_pattern_count(config.m_indent_query);
    config.m_indent_scopes.resize((int)pattern_count);  // default: empty Optional (no override)

    for (uint32_t p = 0; p < pattern_count; ++p)
    {
        uint32_t step_count = 0;
        const TSQueryPredicateStep* steps =
            ts_query_predicates_for_pattern(config.m_indent_query, p, &step_count);

        for (uint32_t s = 0; s < step_count; ++s)
        {
            if (steps[s].type != TSQueryPredicateStepTypeString)
                continue;

            uint32_t pred_len = 0;
            const char* pred_name = ts_query_string_value_for_id(
                config.m_indent_query, steps[s].value_id, &pred_len);
            StringView pred{pred_name, (ByteCount)pred_len};

            if (pred == "set!" and s + 2 < step_count
                and steps[s+1].type == TSQueryPredicateStepTypeString)
            {
                uint32_t key_len = 0;
                const char* key_str = ts_query_string_value_for_id(
                    config.m_indent_query, steps[s+1].value_id, &key_len);
                StringView key{key_str, (ByteCount)key_len};

                if (key == "scope" and s + 2 < step_count
                    and steps[s+2].type == TSQueryPredicateStepTypeString)
                {
                    uint32_t val_len = 0;
                    const char* val_str = ts_query_string_value_for_id(
                        config.m_indent_query, steps[s+2].value_id, &val_len);
                    StringView val{val_str, (ByteCount)val_len};

                    if (val == "all")
                        config.m_indent_scopes[(int)p] = IndentScope::All;
                    else if (val == "tail")
                        config.m_indent_scopes[(int)p] = IndentScope::Tail;

                    s += 2;
                }
            }
        }
    }
}
```

- [ ] **Step 4: Update move constructor and assignment**

Add `m_indent_scopes` to the move constructor initializer list and move assignment body (same pattern as predicate fields).

- [ ] **Step 5: Build**

Run: `make -j$(sysctl -n hw.ncpu) 2>&1 | head -20`
Expected: Clean build

- [ ] **Step 6: Commit**

```bash
git add src/language_registry.hh src/language_registry.cc
git commit -m "feat: add IndentScope enum and #set! scope parsing for indent queries"
```

---

### Task 2: Implement compute_indent_for_line() with parent-walk algorithm

**Files:**
- Modify: `src/commands.cc`

This is the core task. Add the shared indent computation function before `tree_indent_cmd`.

- [ ] **Step 1: Add IndentCaptureType enum and IndentAccum struct**

Add before `tree_indent_cmd` (before line 3905):

```cpp
enum class IndentCaptureType { Indent, IndentAlways, Outdent, OutdentAlways };

struct IndentAccum
{
    int indent = 0;
    int indent_always = 0;
    int outdent = 0;
    int outdent_always = 0;

    void add_capture(IndentCaptureType type)
    {
        switch (type)
        {
        case IndentCaptureType::Indent:
            if (indent_always == 0)
                indent = 1;
            break;
        case IndentCaptureType::IndentAlways:
            indent_always++;
            indent = 0;
            break;
        case IndentCaptureType::Outdent:
            if (outdent_always == 0)
                outdent = 1;
            break;
        case IndentCaptureType::OutdentAlways:
            outdent_always++;
            outdent = 0;
            break;
        }
    }

    void add_line(const IndentAccum& other)
    {
        indent += other.indent;
        indent_always += other.indent_always;
        outdent += other.outdent;
        outdent_always += other.outdent_always;
    }

    int net() const
    {
        return (indent + indent_always) - (outdent + outdent_always);
    }
};
```

- [ ] **Step 2: Add helper functions**

Add after IndentAccum:

```cpp
// Get effective start line, adjusted for new-line insertion
static uint32_t get_node_start_line(TSNode node, Optional<uint32_t> new_line_byte_pos)
{
    uint32_t row = ts_node_start_point(node).row;
    if (new_line_byte_pos and ts_node_start_byte(node) >= *new_line_byte_pos)
        row++;
    return row;
}

// Check if only whitespace appears before node on its line.
// Matches Helix's is_first_in_line (indent.rs:295-306).
static bool is_first_in_line(TSNode node, const Buffer& buffer,
                             Optional<uint32_t> new_line_byte_pos)
{
    uint32_t row = ts_node_start_point(node).row;
    if (row >= (uint32_t)(int)buffer.line_count())
        return true;

    StringView line_content = buffer[LineCount{(int)row}];
    uint32_t node_col = ts_node_start_point(node).column;

    // If new line will be inserted before this node on the same line,
    // only check whitespace from the new-line position to the node.
    uint32_t check_from_col = 0;
    if (new_line_byte_pos)
    {
        uint32_t line_start_col = 0;  // column 0 of this row
        // new_line_byte_pos is relative to buffer; node.column is relative to line
        // If the new line insertion point falls on this line before the node,
        // treat everything before it as "not on this line"
        uint32_t node_start_byte = ts_node_start_byte(node);
        if (node_start_byte >= node_col)  // node_start_byte - node_col = line start byte
        {
            uint32_t line_start_byte = node_start_byte - node_col;
            if (line_start_byte < *new_line_byte_pos
                and *new_line_byte_pos <= node_start_byte)
            {
                check_from_col = *new_line_byte_pos - line_start_byte;
            }
        }
    }

    for (uint32_t col = check_from_col; col < node_col
         and col < (uint32_t)(int)line_content.length(); ++col)
    {
        char c = line_content[(int)col];
        if (c != ' ' and c != '\t')
            return false;
    }
    return true;
}
```

- [ ] **Step 3: Implement compute_indent_for_line()**

```cpp
static int compute_indent_for_line(const SyntaxTree& syntax_tree,
                                   const Buffer& buffer,
                                   LineCount target_line,
                                   bool new_line)
{
    auto* config = syntax_tree.config();
    if (not config or not config->indent_query())
        return 0;

    TSQuery* query = config->indent_query();
    const auto& ind_preds = config->indent_predicates();
    const auto& ind_scopes = config->indent_scopes();

    // Find capture indices
    uint32_t indent_cap = find_capture_index(query, "indent");
    uint32_t indent_always_cap = find_capture_index(query, "indent.always");
    uint32_t outdent_cap = find_capture_index(query, "outdent");
    uint32_t outdent_always_cap = find_capture_index(query, "outdent.always");

    // Compute byte position and new_line_byte_pos
    auto& byte_index = syntax_tree.byte_index();
    uint32_t byte_pos = byte_index.byte_offset({target_line, ByteCount{0}});
    Optional<uint32_t> new_line_byte_pos;
    if (new_line)
        new_line_byte_pos = byte_pos;

    // Step 1: Find deepest node at position
    TSNode root = ts_tree_root_node(syntax_tree.tree());
    TSNode node = ts_node_descendant_for_byte_range(root, byte_pos, byte_pos);
    if (ts_node_is_null(node))
        return 0;

    // Step 2: Collect indent captures into HashMap keyed by node ID
    struct IndentCapture
    {
        IndentCaptureType type;
        IndentScope scope;
    };
    HashMap<uintptr_t, Vector<IndentCapture>> indent_captures;

    QueryCursorGuard cursor;
    ts_query_cursor_set_match_limit(cursor, 256);
    ts_query_cursor_exec(cursor, query, root);

    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match))
    {
        // Filter by predicates
        if (match.pattern_index < (uint32_t)ind_preds.size()
            and not ind_preds[(int)match.pattern_index].empty()
            and not predicates_match(ind_preds[(int)match.pattern_index], match, buffer))
            continue;

        for (uint16_t c = 0; c < match.capture_count; ++c)
        {
            auto& cap = match.captures[c];
            IndentCaptureType cap_type;
            IndentScope default_scope;

            if (cap.index == indent_cap)
            {
                cap_type = IndentCaptureType::Indent;
                default_scope = IndentScope::Tail;
            }
            else if (cap.index == indent_always_cap)
            {
                cap_type = IndentCaptureType::IndentAlways;
                default_scope = IndentScope::Tail;
            }
            else if (cap.index == outdent_cap)
            {
                cap_type = IndentCaptureType::Outdent;
                default_scope = IndentScope::All;
            }
            else if (cap.index == outdent_always_cap)
            {
                cap_type = IndentCaptureType::OutdentAlways;
                default_scope = IndentScope::All;
            }
            else
                continue;  // unknown capture, skip

            // Determine scope: pattern override (from #set! "scope") or capture-type default
            IndentScope scope = default_scope;
            if (match.pattern_index < (uint32_t)ind_scopes.size()
                and ind_scopes[(int)match.pattern_index])
            {
                scope = *ind_scopes[(int)match.pattern_index];
            }

            uintptr_t node_id = (uintptr_t)cap.node.id;
            indent_captures[node_id].push_back({cap_type, scope});
        }
    }

    // Step 3: Walk up parents
    uint32_t target = (uint32_t)(int)target_line;
    if (new_line)
        target++;  // new_line adjusts target

    IndentAccum result;
    IndentAccum indent_for_line;
    IndentAccum indent_for_line_below;

    while (true)
    {
        bool is_first = is_first_in_line(node, buffer, new_line_byte_pos);

        // Apply indent captures for this node
        auto it = indent_captures.find((uintptr_t)node.id);
        if (it != indent_captures.end())
        {
            for (auto& def : it->value)
            {
                switch (def.scope)
                {
                case IndentScope::All:
                    if (is_first)
                        indent_for_line.add_capture(def.type);
                    else
                        indent_for_line_below.add_capture(def.type);
                    break;
                case IndentScope::Tail:
                    indent_for_line_below.add_capture(def.type);
                    break;
                }
            }
            indent_captures.remove(it->key);
        }

        TSNode parent = ts_node_parent(node);
        if (ts_node_is_null(parent))
        {
            // Root reached — finalize.
            // Intentionally use raw row (no new_line adjustment), matching Helix indent.rs:987
            uint32_t node_start_line = ts_node_start_point(node).row;
            if (node_start_line < (uint32_t)(int)target_line
                or (new_line and node_start_line == (uint32_t)(int)target_line
                    and ts_node_start_byte(node) < byte_pos))
            {
                result.add_line(indent_for_line_below);
            }
            result.add_line(indent_for_line);
            break;
        }

        uint32_t node_line = get_node_start_line(node, new_line_byte_pos);
        uint32_t parent_line = get_node_start_line(parent, new_line_byte_pos);

        if (node_line != parent_line)
        {
            // (a) Only add indent_for_line_below if node is before target
            uint32_t effective_target = (uint32_t)(int)target_line + (new_line ? 1 : 0);
            if (node_line < effective_target)
                result.add_line(indent_for_line_below);

            // (b) Adjacent lines: shift down
            if (node_line == parent_line + 1)
            {
                indent_for_line_below = indent_for_line;
            }
            else
            {
                // (c) Gap: flush indent_for_line too
                result.add_line(indent_for_line);
                indent_for_line_below = {};
            }
            indent_for_line = {};
        }

        node = parent;
    }

    return result.net();
}
```

- [ ] **Step 4: Build to verify compilation**

Run: `make -j$(sysctl -n hw.ncpu) 2>&1 | head -20`
Expected: Clean build (function is static, no callers yet)

- [ ] **Step 5: Commit**

```bash
git add src/commands.cc
git commit -m "feat: implement compute_indent_for_line with parent-walk algorithm"
```

---

### Task 3: Rewrite tree_indent_cmd to use compute_indent_for_line

**Files:**
- Modify: `src/commands.cc:3905-4011`

- [ ] **Step 1: Replace the command body**

Replace the lambda body of `tree_indent_cmd` (lines 3913-4010) with:

```cpp
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto* config = syntax_tree.config();
        if (not config or not config->indent_query())
            throw runtime_error("no indent query for this buffer's language");

        ColumnCount tabstop = context.options()["tabstop"].get<int>();
        ColumnCount indent_width = context.options()["indentwidth"].get<int>();
        if (indent_width == 0)
            indent_width = tabstop;

        // Collect the set of lines to reindent (deduplicated, sorted)
        auto& selections = context.selections();
        Vector<LineCount> lines_to_indent;
        LineCount last_line = -1;

        for (auto& sel : selections)
        {
            for (auto line = std::max(last_line + 1, sel.min().line);
                 line <= sel.max().line; ++line)
                lines_to_indent.push_back(line);
            last_line = sel.max().line;
        }

        // Apply edits bottom-to-top
        ScopedEdition edition(context);
        ScopedSelectionEdition selection_edition{context};

        for (int i = (int)lines_to_indent.size() - 1; i >= 0; --i)
        {
            auto line = lines_to_indent[i];
            int level = compute_indent_for_line(syntax_tree, buffer, line, false);
            int target_spaces = std::max(0, level) * (int)indent_width;
            String indent_str(' ', CharCount{target_spaces});

            auto content = buffer[line];
            ByteCount ws_end = 0;
            while (ws_end < content.length() and
                   (content[(int)ws_end] == ' ' or content[(int)ws_end] == '\t'))
                ws_end++;

            buffer.replace(line, {line, ws_end}, indent_str);
        }
    }
```

- [ ] **Step 2: Build and test**

Run: `make -j$(sysctl -n hw.ncpu) 2>&1 | head -20`

- [ ] **Step 3: Commit**

```bash
git add src/commands.cc
git commit -m "feat: rewrite tree-indent to use parent-walk algorithm"
```

---

### Task 4: Rewrite tree_indent_newline_cmd to use compute_indent_for_line

**Files:**
- Modify: `src/commands.cc:4013-4132`

- [ ] **Step 1: Replace the command body**

Replace the lambda body of `tree_indent_newline_cmd` with:

```cpp
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto* config = syntax_tree.config();
        if (not config or not config->indent_query())
            throw runtime_error("no indent query for this buffer's language");

        auto cursor = context.selections().main().cursor();
        auto line = cursor.line;

        if (line == 0)
            return;

        ColumnCount indent_width = context.options()["indentwidth"].get<int>();
        ColumnCount tabstop = context.options()["tabstop"].get<int>();
        if (indent_width == 0)
            indent_width = tabstop;

        int level = compute_indent_for_line(syntax_tree, buffer, line, true);
        int target_spaces = std::max(0, level) * (int)indent_width;
        String indent_str(' ', CharCount{target_spaces});

        // Replace current line's leading whitespace
        auto cur_content = buffer[line];
        ByteCount cur_ws_end = 0;
        while (cur_ws_end < cur_content.length() and
               (cur_content[(int)cur_ws_end] == ' ' or cur_content[(int)cur_ws_end] == '\t'))
            cur_ws_end++;

        ScopedEdition edition(context);
        ScopedSelectionEdition selection_edition{context};
        buffer.replace({line, ByteCount{0}}, {line, cur_ws_end}, indent_str);
    }
```

- [ ] **Step 2: Build and test**

Run: `make -j$(sysctl -n hw.ncpu) 2>&1 | head -20`

- [ ] **Step 3: Commit**

```bash
git add src/commands.cc
git commit -m "feat: rewrite tree-indent-newline to use parent-walk algorithm"
```

---

### Task 5: Smoke test

**Files:** None (manual testing)

- [ ] **Step 1: Build**

Run: `make -j$(sysctl -n hw.ncpu)`

- [ ] **Step 2: Run unit tests**

Run: `./src/kak -ui dummy -e 'quit' 2>&1; echo "Exit: $?"`
Expected: Exit: 0

- [ ] **Step 3: Test with Rust file**

Open `./tskak test_predicates.rs`. Select all (`%`), run `:tree-indent`. Verify:
- Function bodies are indented one level
- Closing braces are at the correct level
- Match arms are indented inside match blocks
- Nested blocks indent correctly

- [ ] **Step 4: Test auto-indent**

Open a new Rust file, type code with Enter keypresses. Verify auto-indent produces correct levels for:
- After `fn main() {` → indent
- After `if condition {` → indent
- Typing `}` → outdent
- Multi-line assignments

- [ ] **Step 5: Check debug buffer**

`:buffer *debug*` — verify no crashes or unexpected errors.

- [ ] **Step 6: Commit fixes if needed**
