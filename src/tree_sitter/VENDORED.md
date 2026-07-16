# Vendored tree-sitter C runtime

- **Upstream**: https://github.com/tree-sitter/tree-sitter (`lib/src/*` and
  `lib/include/tree_sitter/api.h`, flattened into this directory)
- **Version**: unreleased master snapshot, post-v0.26.x
- **ABI**: `TREE_SITTER_LANGUAGE_VERSION` 15,
  `TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION` 13
- **Upstream commit**: checkout window `08d676b41..0535b0ca` of master,
  ~2026-03-22..28. Verified byte-identical against upstream:
  - `api.h` == `lib/include/tree_sitter/api.h` at
    `08d676b4174251b82059307dd4bc0ac78f27534a`
    ("fix(lib): document invariants that must be upheld for `TSInputEdit`",
    2026-03-23)
  - `lib/src` files == `cf302b07d1cae984068b7eb44a6e44529566a8c9`
    ("fix(query): don't add copies for quantifier steps outside alternations",
    2026-03-04; last commit touching `lib/src` before the checkout window)
- **Vendored here**: commit `e94e71ac9`
  ("vendor: add tree-sitter C runtime and build integration", 2026-03-24)
- **Local deviations**: none in the C sources; `lib/src/wasm/` is omitted
  (harmless — `TREE_SITTER_FEATURE_WASM` is never defined, so `wasm_store.c`
  compiles as a stub).

## Re-vendoring

Copy `lib/src/*` (flattened, keeping the `portable/` and `unicode/` subdirs)
and `lib/include/tree_sitter/api.h` from a tagged upstream release into this
directory, update this file with the exact tag/commit, and rebuild
(`make clean && make`). Known upstream query-cursor fixes after this snapshot
(capture-list pool widening 361f293a, finished-state heap fixes 43dc8ead /
123fb1c1, 2026-06/07 anchor/quantifier fixes) make updating to >= the next
tagged release desirable.
