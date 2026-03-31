# Memory

## Project Context
- Stack: C++2b (Kakoune editor fork with tree-sitter integration)
- Build: `make -j$(sysctl -n hw.ncpu)` (simple Makefile, no cmake)
- Grammars: loaded from Helix runtime via dlopen (.so files)
- Queries: loaded from Helix runtime (queries/{lang}/*.scm)
- Key patterns: LanguageConfig holds query pointers, LanguageRegistry does lazy loading

## Principles
- Personal fork — no need for backward compat or community consensus
- Prefer using Helix ecosystem (grammars, queries) over maintaining own

## Decisions
| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-03-31 | Use helix runtime for grammars/queries | Avoid maintaining own grammar build system; helix keeps these up to date |
| 2026-03-31 | Remove fold, context, symbols, grammar mgmt features | User doesn't use these; reduces maintenance burden |
| 2026-03-31 | HELIX_RUNTIME env → /opt/homebrew/opt/helix/libexec/runtime/ fallback | Works OOTB with homebrew, overridable via env var |

## Open Items
- [ ] Build blocked by root-owned .o files — user needs `sudo chown -R $(whoami) src/*.opt.o src/.*.opt.d`
- [ ] runtime/queries/ directory (1201 .scm files) in kakoune repo is now unused — can be deleted

## Last Session
- Date: 2026-03-31
- Task: Remove 4 tree-sitter features + switch to helix grammar loading
- Status: Complete (pending full build verification)
- Key files: src/language_registry.{hh,cc}, src/commands.cc, src/main.cc, rc/tools/tree-sitter.kak, rc/tools/tree-sitter.asciidoc
- Notes: Syntax check passes. Full build needs .o file ownership fix first.
