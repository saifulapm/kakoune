# Indent Algorithm Rewrite

## Problem

Current indent implementation uses a flat scan algorithm — iterate all matches, apply +1/-1 deltas per line. This doesn't support Helix's scope semantics (`all` vs `tail`), capture priority (`@indent.always` overrides `@indent`), or the parent-walk that determines which line each indent applies to. Result: incorrect indentation for multi-line constructs, assignments, and nested blocks.

## Scope

Rewrite `tree_indent_cmd` and `tree_indent_newline_cmd` to use Helix's parent-walk algorithm with `@indent`, `@indent.always`, `@outdent`, `@outdent.always` captures and `#set! "scope" "all"|"tail"` property. Defer `@extend`/`@extend.prevent-once` and `@align`/`@anchor` to follow-up.

## Core Function

```cpp
int compute_indent_for_line(const SyntaxTree& syntax_tree,
                            const Buffer& buffer,
                            LineCount line,
                            bool new_line);
```

Returns target indent level (in indent-width units). Both commands call this.

## Algorithm: Parent-Walk

Matches Helix's `treesitter_indent_for_pos()` (helix-core/src/indent.rs:911-998).

### Step 1: Find deepest node

Find the innermost tree-sitter node at the byte position of `line`. If `new_line` is true, record `new_line_byte_pos` (the byte offset where the newline will be inserted). This adjusts all line-number calculations: any node starting at or after `new_line_byte_pos` has its effective line incremented by 1 (it will be pushed down by the new line).

Helper functions:
- `get_node_start_line(node, new_line_byte_pos)` — returns `ts_node_start_point(node).row`, but if `new_line_byte_pos` is set and `node.start_byte >= new_line_byte_pos`, returns `row + 1`
- `get_node_end_line(node, new_line_byte_pos)` — same adjustment for end point
- `is_first_in_line(node, buffer, new_line_byte_pos)` — true if all bytes between line start and `node.start_byte` are whitespace (with new-line adjustment)

### Step 2: Collect indent captures

Run the indent query on the tree. For each match (filtered by `predicates_match()`), extract the capture type (`@indent`, `@indent.always`, `@outdent`, `@outdent.always`) and store in a `HashMap<node_id, Vector<{capture_type, scope}>>`.

The scope for each capture is determined by:
1. Check if the pattern has a `#set! "scope"` override → use that
2. Otherwise use the capture type's default scope (Indent/IndentAlways → Tail, Outdent/OutdentAlways → All)

### Step 3: Walk up parents

Three accumulators: `result`, `indent_for_line`, `indent_for_line_below` (all `IndentAccum`).

Starting from the deepest node, walk up to the root. At each node:

1. Remove this node's captures from the HashMap (if any)
2. Determine if node is first-in-line via `is_first_in_line()`
3. Apply each capture based on scope:
   - **Scope::Tail**: `indent_for_line_below.add_capture(type)`
   - **Scope::All**: if first-in-line → `indent_for_line.add_capture(type)`, else → `indent_for_line_below.add_capture(type)`
4. Get parent. If no parent (root reached), go to Step 4.
5. Compare `node_line` vs `parent_line` (using `get_node_start_line` with new-line adjustment):
   - **Same line**: continue to parent
   - **Different line** — three-way branch (matching Helix indent.rs:967-981):
     - **(a)** If `node_line < target_line`: flush `indent_for_line_below` into `result` via `result.add_line(indent_for_line_below)`
     - **(b)** If `node_line == parent_line + 1` (adjacent): shift `indent_for_line` down → `indent_for_line_below = indent_for_line`
     - **(c)** If gap > 1 line: flush `indent_for_line` into `result` via `result.add_line(indent_for_line)`; reset `indent_for_line_below`
     - In both (b) and (c): reset `indent_for_line`
6. Continue with parent

### Step 4: Finalize (at root)

Apply the same conditional guard as Helix (indent.rs:984-994):

```
node_start_line = get_node_start_line(root_node, new_line_byte_pos)
if node_start_line < target_line
   OR (new_line AND node_start_line == target_line AND node.start_byte < byte_pos):
    result.add_line(indent_for_line_below)
result.add_line(indent_for_line)
return result.net()
```

The root does NOT unconditionally add `indent_for_line_below` — only if the root starts before the target line.

## Indentation Accumulator

```cpp
struct IndentAccum
{
    int indent = 0;         // regular indent (capped at 1)
    int indent_always = 0;  // always-indent (stacks)
    int outdent = 0;        // regular outdent (capped at 1)
    int outdent_always = 0; // always-outdent (stacks)

    void add_capture(IndentCaptureType type);
    void add_line(const IndentAccum& other);  // accumulate from another line
    int net() const;
};
```

### Cross-line accumulation (matching Helix indent.rs:475-488)

`add_line()` merges another accumulator's fields into this one by addition:
```cpp
void add_line(const IndentAccum& other) {
    indent += other.indent;
    indent_always += other.indent_always;
    outdent += other.outdent;
    outdent_always += other.outdent_always;
}
```

This preserves the structured fields so outdents from inner lines can cancel indents from outer lines. The final `net()` is called only once on the `result` accumulator at the very end.

### Capture semantics (matching Helix indent.rs:494-522)

| Capture Added | Effect |
|---------------|--------|
| `@indent` | if `indent_always == 0`: set `indent = 1` |
| `@indent.always` | `indent_always++`, set `indent = 0` |
| `@outdent` | if `outdent_always == 0`: set `outdent = 1` |
| `@outdent.always` | `outdent_always++`, set `outdent = 0` |

### Net indent

```cpp
int net() const {
    return (indent + indent_always) - (outdent + outdent_always);
}
```

## IndentScope Enum and Storage

```cpp
enum class IndentScope { All, Tail };
```

### Default scopes (matching Helix indent.rs:578-585)

| Capture | Default |
|---------|---------|
| `@indent` | Tail |
| `@indent.always` | Tail |
| `@outdent` | All |
| `@outdent.always` | All |

### `#set! "scope"` parsing

Parsed at grammar load time in `language_registry.cc`, alongside existing `#set!` injection parsing. Stored as `Vector<IndentScope>` indexed by pattern index on `LanguageConfig`.

Only applies to indent queries. If `#set! "scope" "all"` appears on a pattern, ALL captures in that pattern use scope All. Same for "tail".

## Indent Capture Type

```cpp
enum class IndentCaptureType { Indent, IndentAlways, Outdent, OutdentAlways };
```

Capture names looked up at query time:
- `"indent"` → `IndentCaptureType::Indent`
- `"indent.always"` → `IndentCaptureType::IndentAlways`
- `"outdent"` → `IndentCaptureType::Outdent`
- `"outdent.always"` → `IndentCaptureType::OutdentAlways`

Unknown captures are silently skipped.

## Command Rewrites

### `tree_indent_cmd` (reindent selection)

For each selected line:
1. Call `compute_indent_for_line(syntax_tree, buffer, line, false)`
2. Set the line's indentation to `max(0, result) * indent_width`
3. Apply bottom-to-top to avoid coordinate shifting (existing pattern)

### `tree_indent_newline_cmd` (auto-indent on Enter)

1. Call `compute_indent_for_line(syntax_tree, buffer, new_line, true)`
2. Set indentation to `max(0, result) * indent_width`

Both commands share the same algorithm — only the `new_line` flag differs.

## Files Changed

1. **`src/language_registry.hh`** — add `IndentScope` enum, `IndentCaptureType` enum, `indent_scopes()` getter, `m_indent_scopes` field on `LanguageConfig`
2. **`src/language_registry.cc`** — parse `#set! "scope" "all"|"tail"` for indent queries during `load_language()`; update move ctor/assignment
3. **`src/commands.cc`** — add `IndentAccum` struct, `compute_indent_for_line()` function; rewrite `tree_indent_cmd` and `tree_indent_newline_cmd` to call it

## Testing

- Rust: multi-line assignment with `(#not-same-line? ...)` + `(#set! "scope" "all")` indents inner lines correctly
- Python: nested if/for blocks produce correct indent levels
- Closing braces/brackets outdent to parent level
- Single-line constructs (e.g., `if (x) return;`) don't indent the next line
- `@indent.always` overrides multiple `@indent` captures on same node
- `@outdent` with scope All applies on the outdent node's own line
- Line-transition: nodes spanning multiple lines with gaps (parent_line and node_line differ by >1)
- Root-node conditional guard: root starting after target line doesn't contribute indent_for_line_below
- `new_line=true`: nodes at the insertion boundary get correct virtual line numbers
- Regression: existing indent behavior preserved for languages without scope annotations
