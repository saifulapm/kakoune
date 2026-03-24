# Kakoune First-Class Tree-Sitter Integration

## Overview

Native tree-sitter integration embedded directly in Kakoune's C++ core, providing syntax highlighting, text objects, AST navigation, language injection, indentation, and rainbow brackets — all driven by the parse tree rather than regex patterns.

This is a **fork** of Kakoune. Tree-sitter's C runtime is **vendored** into the repo. Grammars and query files are **bundled** in a `runtime/` directory. Tree-sitter highlighting is the **default** when a grammar is available, with regex fallback via command.

## Architecture

### Core Data Model

Each buffer gets a `SyntaxTree` that owns the tree-sitter parsing state:

```cpp
class SyntaxTree {
    TSParser* m_parser;
    TSTree* m_tree;
    TSLanguage* m_language;
    LanguageConfig* m_config;
    size_t m_timestamp;             // last buffer timestamp we parsed at
    Vector<SyntaxLayer> m_layers;   // injection layers
    HighlightCache m_highlight_cache;
};
```

**Lifecycle:**
- Created when buffer's `filetype` is set and a tree-sitter grammar exists
- Stored in `Buffer`'s `ValueMap` (existing per-buffer key-value store) to avoid polluting `buffer.hh` with tree-sitter includes
- Destroyed on buffer close or filetype change

**Incremental parsing:**
1. On `do_highlight()`, check `buffer.timestamp() != m_timestamp`
2. Get `buffer.changes_since(m_timestamp)`
3. Convert each `Buffer::Change` to `TSInputEdit` (see byte offset tracking below)
4. Call `ts_tree_edit()` then `ts_parser_parse()` with old tree
5. Update `m_timestamp`

**Byte offset tracking:** `Buffer::Change` provides `BufferCoord` (line + column), but `TSInputEdit` requires absolute byte offsets (`start_byte`, `old_end_byte`, `new_end_byte`). `SyntaxTree` maintains a prefix-sum array of line byte lengths, updated incrementally on each change. This gives O(1) `BufferCoord` → byte offset conversion. The `TSPoint` fields (`start_point`, `old_end_point`, `new_end_point`) map directly from `BufferCoord` since both use (row, column).

**Undo/redo:** Works naturally — `changes_since(timestamp)` returns all changes including undo reversals. `ts_tree_edit()` handles both insertions and deletions, so undo produces correct `TSInputEdit` values without special handling.

### Highlighting Pipeline

`TreeSitterHighlighter` implements Kakoune's `Highlighter` interface:

```cpp
struct TreeSitterHighlighter : Highlighter {
    TreeSitterHighlighter(String language_name)
        : Highlighter{HighlightPass::Colorize}
        , m_language{std::move(language_name)} {}

    void do_highlight(HighlightContext context, DisplayBuffer& display_buffer,
                      BufferRange range) override;
private:
    String m_language;
};
```

**Highlighting flow:**
1. Get buffer's `SyntaxTree` (reparse if stale)
2. Create `TSQueryCursor`, execute `highlights.scm` query restricted to visible byte range
3. For each match, map capture name → face name (`@keyword` → `ts_keyword`)
4. Call `highlight_range()` on `DisplayBuffer` to apply faces

**Face naming:** Capture names from `.scm` → `ts_` prefix, dots → underscores. `@constant.builtin` → `ts_constant_builtin`. Default faces map to Kakoune's existing semantic faces.

**Registration:**
```
add-highlighter buffer/tree-sitter tree-sitter rust
```

### Language Configuration & Runtime

**Runtime directory layout:**
```
runtime/
├── grammars/
│   ├── rust.so           # .so on all platforms (dlopen handles it)
│   ├── python.so
│   └── ...
└── queries/
    ├── rust/
    │   ├── highlights.scm
    │   ├── injections.scm
    │   ├── textobjects.scm
    │   └── locals.scm
    ├── python/
    │   └── ...
    └── ...
```

**Language registry:**
```cpp
struct LanguageConfig {
    String name;
    TSLanguage* language;           // loaded from .dylib
    TSQuery* highlight_query;       // compiled from highlights.scm
    TSQuery* injection_query;
    TSQuery* textobject_query;
    TSQuery* locals_query;
    HashMap<String, String> capture_faces;
};

class LanguageRegistry : Singleton<LanguageRegistry> {
    HashMap<String, LanguageConfig> m_languages;
public:
    const LanguageConfig* get(StringView name);  // lazy-loads on first access
};
```

- Grammars are compiled `.dylib`/`.so` files (standard tree-sitter format)
- Query files are Helix-compatible `.scm` — Helix's `runtime/queries/` reusable for most languages. Custom predicates (`#is-not? local`, `#set! local.scope-inherits`) require matching implementation in our query evaluator
- Languages lazy-load on first use
- Runtime directory discovered via: `$KAKOUNE_RUNTIME` → `../share/kak/` relative to binary → compile-time default
- Filetype → language mapping via lookup table (most 1:1, some differ like `c-family` → `c`, `cpp`)

### Language Injection

Multi-language documents (HTML+JS, Markdown+code blocks) use layered parse trees:

```cpp
struct SyntaxLayer {
    TSParser* parser;
    TSTree* tree;
    Language language;
    Vector<TSRange> ranges;
    Optional<LayerId> parent;
};
```

**Flow:**
1. After parsing root layer, execute `injections.scm` query
2. Each match specifies injection language (`#set! injection.language "javascript"`) and content range (`@injection.content`)
3. Create child `SyntaxLayer` with `ts_parser_set_included_ranges()` limiting to injection ranges
4. Parse child layers independently
5. During highlighting, iterate all layers — children override parent in their ranges
6. On edits, re-check injections after reparsing root

**Combined injections:** Multiple disjoint nodes of same language (e.g., multiple `<script>` tags) combined into one layer via `injection.combined`.

**Depth limit:** Max 3 injection levels (configurable).

## Performance Design

Goal: faster than Helix on every axis.

### 1. Parsing — Tighter Incremental Updates
- Only reparse layers whose `TSRange` intersects the edit region (Helix reparses all "touched" layers)
- Convert `Buffer::Change` → `TSInputEdit` directly, no intermediate abstraction

### 2. Highlighting — Cached
```cpp
struct HighlightCache {
    size_t tree_timestamp;
    BufferRange range;
    Vector<CachedHighlight> highlights;
};
```
- Cache highlight results per buffer
- Recompute only when: tree changes OR viewport scrolls outside cached range
- On scroll without edits: extend cache incrementally
- Helix recomputes on every render

### 3. Query Execution — Byte Range Restriction
- `ts_query_cursor_set_byte_range()` limits to visible region ± 50 lines scroll margin
- Combined with caching for compound speedup

### 4. Injection Layers — Lazy Parsing
- Don't parse injection layers until visible
- Helix eagerly parses all injection layers on file open

### 5. Grammar Loading — Lazy with Deferred First Parse
- Grammars lazy-load on first access via `dlopen()` (fast, sub-millisecond for most grammars)
- First full parse may exceed frame budget on large files — in that case, use `ts_parser_set_timeout_micros()` with 8ms limit, schedule parse continuation via Kakoune's `EventManager` `Timer`, and show regex highlighting until tree-sitter parse completes
- On completion, invalidate display buffer to trigger re-highlight with tree-sitter
- Helix blocks on grammar load

### 6. Parse Timeout — Per-Frame Budget
- 8ms parse budget per frame (targeting 120fps)
- If tree-sitter doesn't finish, use stale tree this frame, continue on next idle
- Helix uses 500ms timeout (half-second UI freeze)

### 7. Memory
- Each `SyntaxTree` owns its own `TSParser` (creation cost is negligible ~1KB; sharing is unsafe due to stateful parser internals like included ranges and timeout)
- Pool `TSTree` objects for frequently created/destroyed injection layers (e.g., Markdown with many code blocks)

### Performance Comparison

| Aspect | Helix | This Design |
|--------|-------|-------------|
| Parse trigger | All touched layers | Only layers intersecting edit |
| Highlight compute | Every render | Cached, skip if unchanged |
| Injection parsing | Eager (all layers) | Lazy (visible only) |
| Grammar loading | Blocking | Lazy + deferred first parse |
| Parse timeout | 500ms | 8ms per frame |
| Parser reuse | Thread-local cache | Per-SyntaxTree (safe) |

## Text Objects & AST Navigation

### Text Objects

Query-driven via `textobjects.scm`:
```scheme
(function_item body: (_) @function.inside) @function.around
(struct_item body: (_) @class.inside) @class.around
(parameter (_) @parameter.inside) @parameter.around
```

**New commands:**
```
tree-select <object> <inside|around>
tree-select-next <object>
tree-select-prev <object>
```

**Object mode integration:**
```
<a-i>f  →  tree-select function inside
<a-a>f  →  tree-select function around
<a-i>c  →  tree-select class inside
<a-a>c  →  tree-select class around
<a-i>a  →  tree-select parameter inside
```

**Algorithm:**
1. Get cursor byte position
2. Query `textobjects.scm` for captures containing cursor
3. Select smallest containing node
4. On `count > 1`, expand to next larger match

Multiple selections: each selection independently navigates to its nearest AST node.

### AST Navigation

**New commands:**
```
tree-parent
tree-first-child
tree-next-sibling
tree-prev-sibling
```

**User mode:**
```kak
declare-user-mode tree
map global tree p ': tree-parent<ret>'
map global tree c ': tree-first-child<ret>'
map global tree n ': tree-next-sibling<ret>'
map global tree N ': tree-prev-sibling<ret>'
```

## Indentation

AST-based indentation via `indents.scm`:
```scheme
(function_item body: (_) @indent)
(if_expression consequence: (_) @indent)
"}" @outdent
")" @outdent
```

**New command:** `tree-indent` — reindent selection based on AST.

**Auto-indent hook:** Hooks into `InsertChar` (same hook chain as existing regex indent). On newline:
1. Find AST node at cursor
2. Walk up tree, count `@indent` ancestors minus `@outdent` nodes
3. Apply indent level (respecting `tabstop`/`indentwidth`)

Replaces regex-based `increase_indent_pattern`/`decrease_indent_pattern`.

## Rainbow Brackets

Via `rainbow.scm`:
```scheme
["(" ")" "[" "]" "{" "}"] @rainbow
```

Compute nesting depth by maintaining a depth counter stack during the linear highlight query pass — increment on opening bracket capture, decrement on closing. Map `depth % N` to faces `ts_rainbow_1` through `ts_rainbow_6`.

Enabled via: `add-highlighter buffer/rainbow tree-sitter-rainbow`

## Local Variable Scoping

`locals.scm` enables scope-aware highlighting where local variables get distinct faces from globals/keywords.

**Deferred to Phase 2+:** `locals.scm` files will be loaded alongside other queries but not processed in Phase 1. Scope tracking (building a scope tree during highlight traversal, resolving definitions vs references, scope inheritance) adds significant complexity. Initial highlighting works without it — locals support will be added alongside injection support since both require multi-pass query processing.

## Error Handling

- **Grammar load failure** (`dlopen()` fails): log to `*debug*` buffer, fall back to regex highlighting for that language
- **Query compilation error** (malformed `.scm`): report error on first use via `*debug*` buffer, disable tree-sitter for that language
- **Parse failure** (`ts_parser_parse()` returns NULL from timeout): preserve last good tree, schedule retry on next idle via `EventManager` `Timer`
- **Missing injection grammar** (e.g., `injections.scm` references a language we don't have): skip that injection layer silently, highlight parent layer only
- **Invalid UTF-8**: Kakoune buffers are always valid UTF-8 (enforced at buffer load), so this cannot occur

## Fallback Strategy

- Tree-sitter is default when grammar exists for the filetype
- `tree-sitter-disable` command per buffer switches back to regex highlighting
- Languages without a grammar use existing regex highlighters unchanged
- Background grammar loading shows regex highlighting until tree-sitter ready

## Phased Implementation

| Phase | Feature | Description |
|-------|---------|-------------|
| 1 | Core | Vendor tree-sitter, `SyntaxTree`, incremental parsing, `TreeSitterHighlighter`, `LanguageRegistry`, face mapping, runtime directory |
| 2 | Injection | `SyntaxLayer`, `injections.scm` processing, multi-layer highlighting, combined injections |
| 3 | Text Objects | `textobjects.scm` queries, `tree-select` commands, object mode integration |
| 4 | Navigation | `tree-parent/child/sibling` commands, user mode |
| 5 | Polish | `indents.scm` auto-indent, rainbow brackets, performance tuning |

## Files Changed/Added

**New files:**
- `src/syntax_tree.hh` / `src/syntax_tree.cc` — `SyntaxTree`, `SyntaxLayer`, incremental parsing
- `src/tree_sitter_highlighter.hh` / `src/tree_sitter_highlighter.cc` — `TreeSitterHighlighter`
- `src/language_registry.hh` / `src/language_registry.cc` — `LanguageRegistry`, `LanguageConfig`, grammar loading
- `src/tree_sitter_commands.cc` — text object, navigation, indentation commands
- `src/tree_sitter/` — vendored tree-sitter C runtime (~15 files)
- `runtime/grammars/` — compiled grammar `.so` files
- `runtime/queries/` — `.scm` query files per language (from Helix)
- `rc/tree-sitter.kak` — default key mappings, user modes, face definitions

**Modified files:**
- `src/buffer.hh` — no direct changes; `SyntaxTree` stored via existing `Buffer::values()` `ValueMap`
- `src/highlighters.cc` — register `tree-sitter` and `tree-sitter-rainbow` highlighter types in `register_highlighters()`
- `src/commands.cc` — register tree-sitter commands
- `Makefile` / build system — compile vendored tree-sitter, link grammars
- `rc/filetype/*.kak` — add tree-sitter highlighter alongside existing regex (with fallback logic)
