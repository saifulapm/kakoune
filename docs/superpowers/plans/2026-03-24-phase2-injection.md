# Phase 2: Language Injection — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Support multi-language documents (HTML+JS/CSS, Markdown+code blocks) by parsing injection layers with separate grammars and compositing highlights.

**Architecture:** Extend `SyntaxTree` with a `Vector<InjectionLayer>`. After parsing root, run `injections.scm` query to detect embedded languages. Create child parsers with `ts_parser_set_included_ranges()`. Highlighter iterates all layers. `LanguageConfig` extended to load/compile `injections.scm`.

---

## File Changes

| File | Change |
|------|--------|
| `src/language_registry.hh` | Add `m_injection_query` to `LanguageConfig`, add injection predicate parsing |
| `src/language_registry.cc` | Load `injections.scm`, extract `#set! injection.language` predicates |
| `src/syntax_tree.hh` | Add `InjectionLayer` struct, extend `SyntaxTree` with layer management |
| `src/syntax_tree.cc` | Implement injection detection, layer parsing, multi-layer update |
| `src/highlighters.cc` | Update `TreeSitterHighlighter::do_highlight` to iterate all layers |
| `src/tree_sitter_tests.cc` | Add tests for injection predicate extraction |
| `test/highlight/tree-sitter/injection/` | Functional test with HTML grammar |

---

## Task 1: Extend LanguageConfig to Load injections.scm

**Files:** `src/language_registry.hh`, `src/language_registry.cc`

Add to `LanguageConfig`:
- `TSQuery* m_injection_query` — compiled injections.scm
- `Vector<InjectionPattern> m_injection_patterns` — per-pattern metadata extracted from predicates

The `InjectionPattern` struct stores what `#set!` predicates specify per query pattern:
```cpp
struct InjectionPattern
{
    String language;           // from #set! injection.language "..."
    bool combined = false;     // from #set! injection.combined
    bool include_children = false; // from #set! injection.include-children
};
```

In `load_language()`, after loading highlights.scm, also load injections.scm. Use `ts_query_predicates_for_pattern()` to extract `#set!` predicates for each pattern. Build the `InjectionPattern` vector.

Key tree-sitter API for predicate extraction:
```c
const TSQueryPredicateStep* ts_query_predicates_for_pattern(
    const TSQuery* query, uint32_t pattern_index, uint32_t* step_count);
// Steps are: TSQueryPredicateStepTypeString or TSQueryPredicateStepTypeCapture
// For #set! injection.language "rust": 3 steps — "set!", "injection.language", "rust"
```

---

## Task 2: Add InjectionLayer to SyntaxTree

**Files:** `src/syntax_tree.hh`, `src/syntax_tree.cc`

Add:
```cpp
struct InjectionLayer
{
    TSParser* parser = nullptr;
    TSTree* tree = nullptr;
    String language_name;
    const LanguageConfig* config = nullptr;  // not owned
    Vector<TSRange, MemoryDomain::Highlight> ranges;

    ~InjectionLayer();
    InjectionLayer(InjectionLayer&&) noexcept;
    InjectionLayer& operator=(InjectionLayer&&) noexcept;
    // ... delete copy
};
```

Extend `SyntaxTree`:
```cpp
class SyntaxTree
{
    // ... existing members ...
    Vector<InjectionLayer, MemoryDomain::Highlight> m_injection_layers;

public:
    ConstArrayView<InjectionLayer> injection_layers() const;
    void detect_injections(const Buffer& buffer);
};
```

`detect_injections()` algorithm:
1. Get root layer's `LanguageConfig` → its `injection_query()`
2. Execute injection query against root tree
3. For each match, extract `@injection.content` capture (the range to inject)
4. Determine language: check `InjectionPattern` for this pattern's `#set! injection.language`, OR get text from `@injection.language` capture node
5. Look up language in `LanguageRegistry`
6. If found, create/update `InjectionLayer`:
   - Set `ts_parser_set_included_ranges()` with the injection ranges
   - Parse with the injection language's grammar
7. Handle `combined` flag: multiple matches with same language+combined → merge ranges into one layer

---

## Task 3: Update Highlighter for Multi-Layer

**Files:** `src/highlighters.cc`

Change `do_highlight()` to:
1. Highlight root layer (existing code)
2. Call `syntax_tree.detect_injections(buffer)`
3. For each `InjectionLayer`:
   - Execute that layer's `config->highlight_query()` against its tree
   - Apply faces using `highlight_range()` (child layers override parent — applied after parent)

---

## Task 4: Build HTML Grammar and Add Test

**Files:** `runtime/grammars/`, `test/highlight/tree-sitter/injection/`

- Compile HTML, JavaScript, CSS grammars as .so files
- Create functional test: HTML file with `<script>` and `<style>` tags
- Verify buffer content is unchanged after highlighting with injections active

---

## Task 5: Final Verification

- Clean build (release + debug)
- All existing tests pass
- Injection test passes
- Manual smoke test with HTML file
