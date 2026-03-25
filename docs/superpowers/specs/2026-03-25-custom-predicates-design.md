# Custom Predicates (Phase 2)

## Problem

Custom predicates `#not-kind-eq?`, `#same-line?`, `#not-same-line?`, `#one-line?`, `#not-one-line?` are logged as unknown and skipped. These are used in 40+ indent query patterns across Rust, C, Python, Go, etc. Without them, indent queries produce incorrect results.

## Scope

Five custom predicates that operate on node metadata (no buffer text needed). `#is-not? local` deferred to indent algorithm rewrite.

## Design

### Extended `PredicateType` enum

```cpp
enum class PredicateType {
    Eq, NotEq, Match, NotMatch, AnyOf, NotAnyOf,
    NotKindEq, SameLine, NotSameLine, OneLine, NotOneLine
};
```

### Predicate Semantics

| Predicate | Fields Used | Evaluation |
|-----------|-------------|------------|
| `#not-kind-eq? @cap "kind"` | `capture_id` + `value` | `ts_node_type(node) == value` → fail |
| `#same-line? @cap1 @cap2` | `capture_id` + `capture_id2` | `start_point.row` differs → fail |
| `#not-same-line? @cap1 @cap2` | `capture_id` + `capture_id2` | `start_point.row` same → fail |
| `#one-line? @cap` | `capture_id` | `start_point.row != end_point.row` → fail |
| `#not-one-line? @cap` | `capture_id` | `start_point.row == end_point.row` → fail |

### Parse Changes (`parse_single_predicate`)

Add three new branches after existing `#any-of?` handling:

- `"not-kind-eq?"` with count >= 3: capture + string → `NotKindEq`
- `"same-line?" / "not-same-line?"` with count >= 3: two captures → `SameLine`/`NotSameLine`
- `"one-line?" / "not-one-line?"` with count >= 2: one capture → `OneLine`/`NotOneLine`

### Evaluate Changes (`predicates_match`)

Add five new cases to the switch. None need buffer text — all use `TSNode` API directly:

- **NotKindEq**: `StringView{ts_node_type(node)} == pred.value` → return false
- **SameLine**: find second capture node, compare `ts_node_start_point().row`
- **NotSameLine**: same but fail if rows ARE equal
- **OneLine**: `ts_node_start_point(node).row == ts_node_end_point(node).row` → pass; else fail
- **NotOneLine**: inverse of OneLine

For SameLine/NotSameLine, missing second capture → conservative fail (same as Eq/NotEq capture-vs-capture).

### Files Changed

1. **`src/language_registry.hh`** — extend `PredicateType` enum (one line change)
2. **`src/language_registry.cc`** — extend `parse_single_predicate()` (3 new branches) and `predicates_match()` (5 new switch cases)
3. **No other files** — all loop sites already call `predicates_match()`

### Testing

- Unit test: construct each new predicate type, verify fields
- Verify `not-same-line?` no longer appears in debug buffer for Rust
- Verify Rust indent queries with `#not-same-line?` produce correct indentation
