# Tree-Sitter Query Predicate Support

## Problem

761 predicate usages across `runtime/queries/` are ignored. Highlight queries like `((identifier) @constant (#match? @constant "^[A-Z_]+$"))` apply the face to ALL identifiers instead of only uppercase ones. Only `#set!` predicates (for injection config) are currently processed.

## Scope

Core text predicates only: `#eq?`, `#not-eq?`, `#match?`, `#not-match?`, `#any-of?`, `#not-any-of?`. These cover ~95% of predicate usages. Custom predicates (`#not-kind-eq?`, `#same-line?`, `#is-not? local`) deferred to follow-up.

## Design Decisions

- **Parse at load time, evaluate at match time** — follows existing `#set!` pattern in `language_registry.cc`
- **Regex engine** — Kakoune's built-in `Regex` (`regex.hh`), not `std::regex`
- **No new files** — predicate types added to `language_registry.hh`, evaluation as free function in `language_registry.cc`

## Data Structures (`language_registry.hh`)

```cpp
enum class PredicateType { Eq, NotEq, Match, NotMatch, AnyOf, NotAnyOf };

struct QueryPredicate {
    PredicateType type;
    uint32_t capture_id;            // capture to test
    String value;                   // literal string (Eq/NotEq)
    Optional<uint32_t> capture_id2; // second capture (capture-vs-capture Eq)
    Vector<String> values;          // string set (AnyOf)
    Optional<Regex> regex;          // compiled regex (Match/NotMatch)
};

// Indexed by pattern index
using PatternPredicates = Vector<Vector<QueryPredicate>>;
```

`LanguageConfig` gains `PatternPredicates` for each query type: highlights, locals, injections, textobjects, indents.

## Parse Phase (`language_registry.cc`)

At grammar load time, for each compiled query:

1. Iterate `ts_query_predicates_for_pattern(query, pattern_index, &step_count)` for every pattern
2. Split steps by `TSQueryPredicateStepTypeDone` sentinel into individual predicates
3. First step is always a string naming the predicate (`#eq?`, etc.)
4. Parse arguments:
   - `#eq? @cap "str"` → `Eq` with capture_id + value
   - `#eq? @cap @cap2` → `Eq` with capture_id + capture_id2
   - `#not-eq? @cap "str"` → `NotEq` with capture_id + value
   - `#match? @cap "regex"` → `Match` with capture_id + compiled Regex
   - `#not-match? @cap "regex"` → `NotMatch` with capture_id + compiled Regex
   - `#not-eq? @cap @cap2` → `NotEq` with capture_id + capture_id2
   - `#any-of? @cap "a" "b" ...` → `AnyOf` with capture_id + values vector
   - `#not-any-of? @cap "a" "b" ...` → `NotAnyOf` with capture_id + values vector
5. Invalid regex at parse time → log warning, omit predicate from vector entirely
6. Unknown predicates logged and skipped (not an error)
6. Existing `#set!` parsing for injections remains unchanged

Extracted into a reusable function:
```cpp
PatternPredicates parse_query_predicates(const TSQuery* query);
```

Called for each query during `LanguageRegistry::load_language()`.

## Evaluate Phase

Free function in `language_registry.cc` (or `.hh` if inline):

```cpp
bool predicates_match(const Vector<QueryPredicate>& predicates,
                      const TSQueryMatch& match,
                      const Buffer& buffer);
```

For each predicate in the pattern's list:
1. Find the captured node by `capture_id` in `match.captures`
2. Extract node text from buffer using `TSNode` start/end coordinates
3. Evaluate:
   - **Eq**: node text == value (or == other capture's text)
   - **NotEq**: node text != value
   - **Match**: `regex_search(node_text, regex)` (partial match, not full-string)
   - **NotMatch**: `!regex_search(node_text, regex)`
   - **AnyOf**: node text is in values set
   - **NotAnyOf**: node text is NOT in values set
4. ALL predicates must pass (logical AND) — if any fails, return false

Node text extraction uses `BufferCoord` from `TSNode`'s `ts_node_start_point()` / `ts_node_end_point()`, reading from buffer lines. Single-line nodes use `StringView` (no allocation). Multi-line nodes concatenate lines into a `String`. Most predicate targets (identifiers, keywords) are single-line.

Buffer access: `run_highlights` obtains buffer via `context.context.buffer()`. Other call sites already have buffer available.

## Integration Points

### `highlighters.cc` — TreeSitterHighlighter

In the highlight capture loop (`ts_query_cursor_next_capture`):
```cpp
// After getting match, before processing capture:
const auto& preds = lang_config.highlight_predicates[match.pattern_index];
if (!preds.empty() && !predicates_match(preds, match, buffer))
{
    ts_query_cursor_remove_match(m_cursor, match.id);
    continue;
}
```

Note: `ts_query_cursor_remove_match()` is needed when using `next_capture` to tell the cursor the match was rejected.

### `highlighters.cc` — build_local_references

In the locals match loop (`ts_query_cursor_next_match`):
```cpp
if (!predicates_match(locals_preds[match.pattern_index], match, buffer))
    continue;
```

### `syntax_tree.cc` — detect_injections

In the injection match loop:
```cpp
if (!predicates_match(injection_preds[match.pattern_index], match, buffer))
    continue;
```

### `commands.cc` — all match/capture loops

Every `ts_query_cursor_next_match` / `next_capture` loop needs the predicate gate. Specific sites:
- Text object selection (`tree-select`, `tree-select-next/prev`, `tree-select-all`, `tree-filter`)
- Indent queries (`tree-indent`)
- Sticky context (`tree-update-context`)
- Inspection commands (`tree-sitter-highlight-name`)

## Predicate Semantics

| Predicate | Example | Logic |
|-----------|---------|-------|
| `#eq? @cap "str"` | `(#eq? @fn "main")` | node text == "str" |
| `#eq? @cap @cap2` | `(#eq? @a @b)` | node text == other node text |
| `#not-eq? @cap "str"` | `(#not-eq? @_ "_")` | node text != "str" |
| `#not-eq? @cap @cap2` | `(#not-eq? @a @b)` | node text != other node text |
| `#match? @cap "re"` | `(#match? @c "^[A-Z]+$")` | regex search (partial match) |
| `#not-match? @cap "re"` | `(#not-match? @v "^_")` | regex doesn't match |
| `#any-of? @cap "a" "b"` | `(#any-of? @kw "if" "else")` | text in set |
| `#not-any-of? @cap "a" "b"` | `(#not-any-of? @v "x" "y")` | text NOT in set |

## Performance

- Regexes compiled once at load time, reused every frame
- Predicate check is O(predicates × node_text_length) per match — negligible vs query iteration
- Short-circuit: return false on first failing predicate
- No allocation in hot path: node text extracted into stack `StringView` when single-line

## Error Handling

- Invalid regex in `#match?` → log warning at parse time, omit predicate entirely (no runtime branch)
- Unknown predicate name → log at debug level, skip
- Missing capture in match → predicate fails (conservative)
- Capture outside buffer bounds → clamp coordinates (existing safety pattern)

## Files Changed

1. **`src/language_registry.hh`** — `PredicateType`, `QueryPredicate`, `PatternPredicates`; new fields on `LanguageConfig`
2. **`src/language_registry.cc`** — `parse_query_predicates()`, called during grammar loading
3. **`src/highlighters.cc`** — predicate filtering in highlight + locals loops
4. **`src/syntax_tree.cc`** — predicate filtering in injection detection
5. **`src/commands.cc`** — predicate filtering in text object/indent queries

## Testing

- Unit test: parse predicates from a test query, verify correct types/values
- Unit test: `predicates_match()` with mock buffer content
- Unit test: `#eq?` capture-vs-capture (two captures in same match)
- Unit test: `#match?` with single-line and multi-line nodes
- Unit test: `#any-of?` with empty string in the value set
- Unit test: predicate on capture missing from match → returns false
- Unit test: short-circuit — first predicate fails, second never evaluated
- Functional test: highlight query with `#match?` produces correct face only on matching nodes
- Regression: existing highlights/injections/text-objects unchanged for queries without predicates
