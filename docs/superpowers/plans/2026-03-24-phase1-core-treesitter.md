# Phase 1: Core Tree-Sitter Integration — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Embed the tree-sitter C runtime into a fork of Kakoune, enabling incremental syntax highlighting via `.scm` query files for any language with a tree-sitter grammar.

**Architecture:** Vendor tree-sitter's C11 runtime into `src/tree_sitter/`. Add `SyntaxTree` (per-buffer parse state with incremental updates), `LanguageRegistry` (lazy grammar/query loading), and `TreeSitterHighlighter` (lives inside `highlighters.cc` alongside existing highlighters for access to `highlight_range()`). Buffer changes convert to `TSInputEdit` via a prefix-sum line-byte index. Highlight results are cached per buffer.

**Tech Stack:** C++20 (Kakoune's standard), tree-sitter C11 runtime, `.scm` query files (Helix-compatible)

**Spec:** `docs/superpowers/specs/2026-03-24-kakoune-treesitter-design.md`

**Reference codebases:**
- Kakoune source: `/Users/saiful/Sites/rust/kakoune/` — the fork base
- Helix: `/Users/saiful/Sites/rust/helix/` — reference for tree-sitter integration patterns
- tree-house: `/Users/saiful/Sites/rust/tree-house/` — reference for query/highlight algorithms

---

## Kakoune C++ Style Guide (for implementers)

Follow these conventions exactly — code must be indistinguishable from native Kakoune:

- **Naming:** `m_` prefix for members, `snake_case` functions, `PascalCase` types, `do_` prefix for virtual overrides
- **Headers:** `#ifndef foo_hh_INCLUDED` / `#define foo_hh_INCLUDED` guards. Own header first, then project headers, then system headers
- **Strings:** Use `StringView` for parameters (non-owning), `String` for owned data. Use `format()` for formatting
- **Memory:** `UniquePtr<T>` (not `std::unique_ptr`), `make_unique_ptr<T>()`. Explicit `MemoryDomain` for containers
- **Errors:** Throw `runtime_error(format("message {}", var))`. Use `kak_assert()` for debug invariants
- **Per-buffer state:** Use `BufferSideCache<T>` pattern with `get_free_value_id()` and `buffer.values()[]`
- **Highlighter registration:** Static `HighlighterDesc` + factory function, registered in `register_highlighters()`
- **Command registration:** Register directly via `cm.register_command()` in `register_commands()` (the `CommandDesc` struct is local to `commands.cc` — either define commands there or use `cm.register_command()` directly)
- **Tests:** Unit tests via `UnitTest test_name{[]() { kak_assert(...); }};` (debug builds only). Functional tests in `test/<category>/<name>/` with `cmd`, `in`, `out`, `rc` files
- **Build:** New `.cc` files in `src/` auto-discovered by Makefile's `find` rule. No manual file list
- **RAII for C resources:** Use Kakoune's `on_scope_end` or a simple RAII wrapper for `TSQueryCursor`, `TSParser`, `TSTree`

---

## File Structure

### New Files

| File | Responsibility |
|------|---------------|
| `src/tree_sitter/` | Vendored tree-sitter C runtime. Compiled as C11 objects |
| `src/tree_sitter.hh` | `extern "C"` wrapper to include tree-sitter API from C++ |
| `src/syntax_tree.hh` | `SyntaxTree`, `LineByteIndex` declarations |
| `src/syntax_tree.cc` | Incremental parsing, `Buffer::Change` → `TSInputEdit`, `TSInput` read callback, per-buffer storage |
| `src/language_registry.hh` | `LanguageConfig`, `LanguageRegistry` singleton |
| `src/language_registry.cc` | `dlopen()` grammar loading, `.scm` reading, face mapping |
| `src/tree_sitter_tests.cc` | Unit tests for byte index, input edit conversion |
| `rc/tree-sitter.kak` | Default `ts_*` face definitions, auto-enable hook |
| `runtime/queries/` | `.scm` query files per language (from Helix) |
| `runtime/grammars/` | Compiled grammar `.so` files (at least C for testing) |
| `test/highlight/tree-sitter/` | Functional tests |

### Modified Files

| File | Change |
|------|--------|
| `Makefile` | Add C compilation rule for `src/tree_sitter/*.c`, add `-ldl` (Linux), add `CC` variable |
| `src/highlighters.cc` | Add `TreeSitterHighlighter` class and register it (lives here for `highlight_range()` access) |
| `src/commands.cc` | Add `tree-sitter-enable`/`tree-sitter-disable` commands in `register_commands()` |
| `src/main.cc` | Instantiate `LanguageRegistry` singleton |

---

## Task 1: Fork Kakoune and Set Up Build

**Files:**
- Copy: entire `/Users/saiful/Sites/rust/kakoune/` into working directory

- [ ] **Step 1: Copy Kakoune source into the treesitter.kak repo**

```bash
cd /Users/saiful/Sites/rust/treesitter.kak
cp -r /Users/saiful/Sites/rust/kakoune/src .
cp -r /Users/saiful/Sites/rust/kakoune/rc .
cp -r /Users/saiful/Sites/rust/kakoune/test .
cp -r /Users/saiful/Sites/rust/kakoune/colors .
cp -r /Users/saiful/Sites/rust/kakoune/doc .
cp /Users/saiful/Sites/rust/kakoune/Makefile .
```

- [ ] **Step 2: Verify the build works**

```bash
make -j$(sysctl -n hw.ncpu)
```

Expected: Builds `src/kak` successfully.

- [ ] **Step 3: Verify tests pass**

```bash
make test
```

Expected: All existing tests pass.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "fork: import kakoune source as base for tree-sitter integration"
```

---

## Task 2: Vendor Tree-Sitter C Runtime

**Files:**
- Create: `src/tree_sitter/` directory with vendored C files
- Create: `src/tree_sitter.hh`
- Modify: `Makefile`

- [ ] **Step 1: Download and vendor tree-sitter runtime**

```bash
cd /Users/saiful/Sites/rust/treesitter.kak
git clone --depth 1 https://github.com/tree-sitter/tree-sitter.git /tmp/tree-sitter-vendor
mkdir -p src/tree_sitter

# Copy lib/src (runtime implementation) and lib/include (public API)
cp /tmp/tree-sitter-vendor/lib/src/*.c src/tree_sitter/
cp /tmp/tree-sitter-vendor/lib/src/*.h src/tree_sitter/
mkdir -p src/tree_sitter/api
cp /tmp/tree-sitter-vendor/lib/include/tree_sitter/*.h src/tree_sitter/api/

rm -rf /tmp/tree-sitter-vendor
```

- [ ] **Step 2: Create `src/tree_sitter.hh` — C++ wrapper header**

```cpp
#ifndef tree_sitter_hh_INCLUDED
#define tree_sitter_hh_INCLUDED

extern "C" {
#include "tree_sitter/api/api.h"
}

#endif // tree_sitter_hh_INCLUDED
```

- [ ] **Step 3: Update Makefile for C compilation**

Add these changes to the Makefile:

After the `CXX = c++` line, add:
```makefile
CC = cc
```

After the `objects = ...` line, add:
```makefile
ts_sources = $(shell find src/tree_sitter -maxdepth 1 -type f -name '*.c')
ts_objects = $(ts_sources:.c=$(tag).o)
```

Add `.c` to the `.SUFFIXES` line:
```makefile
.SUFFIXES: $(tag).o .cc .c
```

Add C compilation rule (after the `.cc$(tag).o:` rule):
```makefile
.c$(tag).o:
	$(CC) -std=c11 $(CXXFLAGS-debug-$(debug)) -Isrc/tree_sitter -MD -MP -MF $(*D)/.$(*F)$(tag).d -c -o $@ $<
```

Modify the link rule to include `$(ts_objects)`:
```makefile
src/kak$(tagbin): src/.version$(tag).o $(objects) $(ts_objects)
	$(CXX) $(KAK_LDFLAGS) $(KAK_CXXFLAGS) $(objects) $(ts_objects) src/.version$(tag).o $(KAK_LIBS) -o $@
```

Add `-ldl` on Linux (after existing LIBS lines):
```makefile
LIBS-os-Linux += -ldl
```

- [ ] **Step 4: Verify tree-sitter compiles and links**

```bash
make clean && make -j$(sysctl -n hw.ncpu)
```

Expected: Builds successfully with tree-sitter objects linked in.

- [ ] **Step 5: Commit**

```bash
git add src/tree_sitter/ src/tree_sitter.hh Makefile
git commit -m "vendor: add tree-sitter C runtime and build integration"
```

---

## Task 3: Implement SyntaxTree — Core Parse State

**Files:**
- Create: `src/syntax_tree.hh`
- Create: `src/syntax_tree.cc`
- Create: `src/tree_sitter_tests.cc`

- [ ] **Step 1: Write `src/syntax_tree.hh`**

```cpp
#ifndef syntax_tree_hh_INCLUDED
#define syntax_tree_hh_INCLUDED

#include "tree_sitter.hh"
#include "buffer.hh"
#include "utils.hh"
#include "vector.hh"

namespace Kakoune
{

struct LineByteIndex
{
    void rebuild(const Buffer& buffer);
    uint32_t byte_offset(BufferCoord coord) const;

private:
    // Prefix-sum: m_line_start_bytes[i] = sum of byte lengths of lines 0..i-1
    Vector<uint32_t, MemoryDomain::Highlight> m_line_start_bytes;
};

TSInputEdit make_ts_input_edit(const LineByteIndex& index,
                               const Buffer::Change& change);

class SyntaxTree
{
public:
    SyntaxTree(const Buffer& buffer, TSLanguage* language,
               TSQuery* highlight_query);
    ~SyntaxTree();

    SyntaxTree(SyntaxTree&& other) noexcept;
    SyntaxTree& operator=(SyntaxTree&&) noexcept;
    SyntaxTree(const SyntaxTree&) = delete;
    SyntaxTree& operator=(const SyntaxTree&) = delete;

    void update(const Buffer& buffer);

    TSTree* tree() const { return m_tree; }
    TSQuery* highlight_query() const { return m_highlight_query; }
    const LineByteIndex& byte_index() const { return m_byte_index; }
    bool is_valid() const { return m_tree != nullptr; }
    size_t timestamp() const { return m_timestamp; }

private:
    void full_parse(const Buffer& buffer);

    TSParser* m_parser = nullptr;
    TSTree* m_tree = nullptr;
    TSLanguage* m_language = nullptr;
    TSQuery* m_highlight_query = nullptr;  // not owned — owned by LanguageConfig
    LineByteIndex m_byte_index;
    size_t m_timestamp = 0;
};

void create_syntax_tree(const Buffer& buffer, TSLanguage* language,
                        TSQuery* highlight_query);
SyntaxTree& get_syntax_tree(const Buffer& buffer);
void remove_syntax_tree(const Buffer& buffer);
bool has_syntax_tree(const Buffer& buffer);

} // namespace Kakoune

#endif // syntax_tree_hh_INCLUDED
```

- [ ] **Step 2: Write `src/syntax_tree.cc`**

Key implementation details:
- `LineByteIndex::rebuild(buffer)` iterates buffer lines to build prefix-sum
- `make_ts_input_edit` converts `Buffer::Change` using the byte index. For Insert: `old_end == start` (nothing removed). For Erase: `new_end == start` (nothing added)
- `SyntaxTree::update()` must rebuild the byte index **before** processing each change (using the buffer state at that change's timestamp). Since `changes_since()` returns changes relative to the buffer state at the time they occurred, and tree-sitter's `ts_tree_edit` expects coordinates in the current tree state, we process all edits against the tree first, then do a single incremental reparse
- `TSInput::read` callback returns pointers into buffer line storage (safe — single-threaded, buffer not modified during parse)
- Per-buffer storage uses static `ValueId` from `get_free_value_id()`
- `SyntaxTree` needs move constructor/assignment for `Value` storage

```cpp
#include "syntax_tree.hh"
#include "exception.hh"

namespace Kakoune
{

// --- LineByteIndex ---

void LineByteIndex::rebuild(const Buffer& buffer)
{
    auto count = (int)buffer.line_count();
    m_line_start_bytes.resize(count);
    uint32_t offset = 0;
    for (int i = 0; i < count; ++i)
    {
        m_line_start_bytes[i] = offset;
        offset += (uint32_t)buffer[LineCount{i}].length();
    }
}

uint32_t LineByteIndex::byte_offset(BufferCoord coord) const
{
    if ((int)coord.line >= (int)m_line_start_bytes.size())
        return m_line_start_bytes.empty() ? 0 : m_line_start_bytes.back();
    return m_line_start_bytes[(int)coord.line] + (uint32_t)coord.column;
}

// --- TSInputEdit conversion ---

TSInputEdit make_ts_input_edit(const LineByteIndex& index,
                               const Buffer::Change& change)
{
    TSInputEdit edit{};
    edit.start_byte = index.byte_offset(change.begin);
    edit.start_point = {(uint32_t)change.begin.line,
                        (uint32_t)change.begin.column};

    if (change.type == Buffer::Change::Insert)
    {
        edit.old_end_byte = edit.start_byte;
        edit.old_end_point = edit.start_point;
        edit.new_end_byte = index.byte_offset(change.end);
        edit.new_end_point = {(uint32_t)change.end.line,
                              (uint32_t)change.end.column};
    }
    else // Erase
    {
        edit.old_end_byte = index.byte_offset(change.end);
        edit.old_end_point = {(uint32_t)change.end.line,
                              (uint32_t)change.end.column};
        edit.new_end_byte = edit.start_byte;
        edit.new_end_point = edit.start_point;
    }

    return edit;
}

// --- TSInput read callback ---

static const char* ts_input_read(void* payload, uint32_t byte_offset,
                                  TSPoint position, uint32_t* bytes_read)
{
    auto* buffer = static_cast<const Buffer*>(payload);
    auto line = LineCount{(int)position.row};

    if (line >= buffer->line_count())
    {
        *bytes_read = 0;
        return "";
    }

    StringView content = (*buffer)[line];
    auto col = ByteCount{(int)position.column};

    if (col >= content.length())
    {
        *bytes_read = 0;
        return "";
    }

    *bytes_read = (uint32_t)(content.length() - col);
    return content.begin() + (int)col;
}

// --- SyntaxTree ---

SyntaxTree::SyntaxTree(const Buffer& buffer, TSLanguage* language,
                       TSQuery* highlight_query)
    : m_language{language}
    , m_highlight_query{highlight_query}
{
    m_parser = ts_parser_new();
    if (not m_parser)
        throw runtime_error("failed to create tree-sitter parser");

    if (not ts_parser_set_language(m_parser, language))
    {
        ts_parser_delete(m_parser);
        m_parser = nullptr;
        throw runtime_error("failed to set tree-sitter language");
    }

    ts_parser_set_timeout_micros(m_parser, 8000);
    full_parse(buffer);
}

SyntaxTree::~SyntaxTree()
{
    if (m_tree)
        ts_tree_delete(m_tree);
    if (m_parser)
        ts_parser_delete(m_parser);
}

SyntaxTree::SyntaxTree(SyntaxTree&& other) noexcept
    : m_parser{other.m_parser}
    , m_tree{other.m_tree}
    , m_language{other.m_language}
    , m_highlight_query{other.m_highlight_query}
    , m_byte_index{std::move(other.m_byte_index)}
    , m_timestamp{other.m_timestamp}
{
    other.m_parser = nullptr;
    other.m_tree = nullptr;
}

SyntaxTree& SyntaxTree::operator=(SyntaxTree&& other) noexcept
{
    if (this != &other)
    {
        if (m_tree) ts_tree_delete(m_tree);
        if (m_parser) ts_parser_delete(m_parser);

        m_parser = other.m_parser;
        m_tree = other.m_tree;
        m_language = other.m_language;
        m_highlight_query = other.m_highlight_query;
        m_byte_index = std::move(other.m_byte_index);
        m_timestamp = other.m_timestamp;

        other.m_parser = nullptr;
        other.m_tree = nullptr;
    }
    return *this;
}

void SyntaxTree::full_parse(const Buffer& buffer)
{
    m_byte_index.rebuild(buffer);

    TSInput input{};
    input.payload = const_cast<Buffer*>(&buffer);
    input.read = ts_input_read;
    input.encoding = TSInputEncodingUTF8;

    TSTree* new_tree = ts_parser_parse(m_parser, nullptr, input);
    if (new_tree)
    {
        if (m_tree)
            ts_tree_delete(m_tree);
        m_tree = new_tree;
    }

    m_timestamp = buffer.timestamp();
}

void SyntaxTree::update(const Buffer& buffer)
{
    if (buffer.timestamp() == m_timestamp)
        return;

    if (not m_tree)
    {
        full_parse(buffer);
        return;
    }

    // Apply all edits to the tree.
    // Buffer::Change coordinates are relative to the buffer state
    // *at the time of that change*. tree-sitter's ts_tree_edit expects
    // coordinates relative to the current tree state, which gets updated
    // after each edit. Both track the same sequence of transformations,
    // so applying them in order is correct.
    //
    // We build the byte index from the state BEFORE changes, then
    // update it after each change to stay consistent.
    auto changes = buffer.changes_since(m_timestamp);

    for (auto& change : changes)
    {
        auto edit = make_ts_input_edit(m_byte_index, change);
        ts_tree_edit(m_tree, &edit);

        // After applying each edit to the tree, rebuild byte index
        // to match the tree's new coordinate space.
        // This is correct because changes_since returns changes
        // in chronological order, and each change's coords are
        // relative to the state after all previous changes.
        m_byte_index.rebuild(buffer);
    }

    // Incremental reparse
    TSInput input{};
    input.payload = const_cast<Buffer*>(&buffer);
    input.read = ts_input_read;
    input.encoding = TSInputEncodingUTF8;

    TSTree* new_tree = ts_parser_parse(m_parser, m_tree, input);
    if (new_tree)
    {
        ts_tree_delete(m_tree);
        m_tree = new_tree;
    }

    m_timestamp = buffer.timestamp();
}

// --- Per-buffer storage ---

static const ValueId syntax_tree_id = get_free_value_id();

void create_syntax_tree(const Buffer& buffer, TSLanguage* language,
                        TSQuery* highlight_query)
{
    buffer.values()[syntax_tree_id] = Value(SyntaxTree{buffer, language, highlight_query});
}

SyntaxTree& get_syntax_tree(const Buffer& buffer)
{
    return buffer.values()[syntax_tree_id].as<SyntaxTree>();
}

void remove_syntax_tree(const Buffer& buffer)
{
    buffer.values().erase(syntax_tree_id);
}

bool has_syntax_tree(const Buffer& buffer)
{
    auto it = buffer.values().find(syntax_tree_id);
    return it != buffer.values().end();
}

} // namespace Kakoune
```

- [ ] **Step 3: Write unit tests in `src/tree_sitter_tests.cc`**

```cpp
#include "syntax_tree.hh"
#include "unit_tests.hh"

namespace Kakoune
{

UnitTest test_make_ts_input_edit_insert{[]()
{
    // Simulate buffer "abc\ndef\n" — line 0: 4 bytes, line 1: 4 bytes
    LineByteIndex index;
    // We can't call rebuild without a Buffer in unit tests,
    // so test the TSInputEdit logic directly via known byte offsets.
    // This test validates the struct field assignments are correct.

    Buffer::Change insert_change{Buffer::Change::Insert, {1, 2}, {1, 4}};

    // For Insert: old_end == start, new_end == change.end
    // start at line 1, col 2
    // We verify the TSPoint mapping is correct
    auto edit = make_ts_input_edit(index, insert_change);

    kak_assert(edit.start_point.row == 1);
    kak_assert(edit.start_point.column == 2);
    kak_assert(edit.old_end_point.row == 1);
    kak_assert(edit.old_end_point.column == 2);
    kak_assert(edit.new_end_point.row == 1);
    kak_assert(edit.new_end_point.column == 4);
}};

UnitTest test_make_ts_input_edit_erase{[]()
{
    Buffer::Change erase_change{Buffer::Change::Erase, {0, 3}, {0, 5}};

    auto edit = make_ts_input_edit(LineByteIndex{}, erase_change);

    kak_assert(edit.start_point.row == 0);
    kak_assert(edit.start_point.column == 3);
    kak_assert(edit.old_end_point.row == 0);
    kak_assert(edit.old_end_point.column == 5);
    kak_assert(edit.new_end_point.row == 0);
    kak_assert(edit.new_end_point.column == 3);
}};

UnitTest test_capture_face_name{[]()
{
    kak_assert(capture_to_face_name("keyword") == "ts_keyword");
    kak_assert(capture_to_face_name("constant.builtin") == "ts_constant_builtin");
    kak_assert(capture_to_face_name("punctuation.bracket") == "ts_punctuation_bracket");
}};

} // namespace Kakoune
```

Note: `capture_to_face_name` is declared in `language_registry.hh` (Task 4). This file will not compile until Task 4 is complete. The implementer should either stub the function in Task 3 or move this test to after Task 4.

- [ ] **Step 4: Verify it compiles**

```bash
make debug=yes -j$(sysctl -n hw.ncpu)
```

Expected: Compiles. Unit tests for `make_ts_input_edit` run at startup.

- [ ] **Step 5: Commit**

```bash
git add src/syntax_tree.hh src/syntax_tree.cc src/tree_sitter_tests.cc
git commit -m "feat: add SyntaxTree with incremental parsing and byte offset tracking"
```

---

## Task 4: Implement LanguageRegistry

**Files:**
- Create: `src/language_registry.hh`
- Create: `src/language_registry.cc`
- Modify: `src/main.cc` — instantiate singleton

- [ ] **Step 1: Write `src/language_registry.hh`**

```cpp
#ifndef language_registry_hh_INCLUDED
#define language_registry_hh_INCLUDED

#include "tree_sitter.hh"
#include "hash_map.hh"
#include "string.hh"
#include "utils.hh"
#include "vector.hh"

namespace Kakoune
{

String capture_to_face_name(StringView capture_name);

struct LanguageConfig
{
    String name;
    TSLanguage* language = nullptr;
    TSQuery* highlight_query = nullptr;
    Vector<String, MemoryDomain::Highlight> capture_faces; // index → face name
    void* grammar_handle = nullptr; // dlopen handle

    ~LanguageConfig();
    LanguageConfig() = default;
    LanguageConfig(LanguageConfig&&) noexcept;
    LanguageConfig& operator=(LanguageConfig&&) noexcept;
    LanguageConfig(const LanguageConfig&) = delete;
    LanguageConfig& operator=(const LanguageConfig&) = delete;
};

class LanguageRegistry : public Singleton<LanguageRegistry>
{
public:
    LanguageRegistry(String runtime_dir);
    ~LanguageRegistry();

    const LanguageConfig* get(StringView language_name);
    StringView runtime_dir() const { return m_runtime_dir; }

    static StringView filetype_to_language(StringView filetype);

private:
    LanguageConfig* load_language(StringView name);
    String read_query_file(StringView language, StringView query_name) const;

    HashMap<String, LanguageConfig, MemoryDomain::Highlight> m_languages;
    String m_runtime_dir;
};

} // namespace Kakoune

#endif // language_registry_hh_INCLUDED
```

- [ ] **Step 2: Write `src/language_registry.cc`**

Key implementation:
- `capture_to_face_name()`: replace dots with underscores, prefix `ts_`
- `LanguageConfig` destructor: `ts_query_delete()`, `dlclose()`
- `LanguageConfig` move: transfer ownership, null out source
- `filetype_to_language()`: mapping table for known mismatches (`sh` → `bash`, `cpp` → `cpp`, etc.)
- `load_language()`: `dlopen()` grammar `.so`, `dlsym()` for `tree_sitter_<name>()` entry point, `ts_query_new()` to compile highlights.scm, build capture→face mapping
- All errors logged to `*debug*` buffer via `write_to_debug_buffer()`, return `nullptr`
- Platform handling: on macOS, try `.dylib` then `.so`; on Linux, `.so` only

```cpp
#include "language_registry.hh"
#include "exception.hh"
#include "file.hh"
#include "debug.hh"

#include <dlfcn.h>

namespace Kakoune
{

// (Full implementation as shown in original plan Task 4, with these fixes:)
// - Constructor takes runtime_dir parameter
// - Platform-aware grammar extension: try .so first, then .dylib on macOS
// - Proper error handling with write_to_debug_buffer

LanguageRegistry::LanguageRegistry(String runtime_dir)
    : m_runtime_dir{std::move(runtime_dir)}
{}

// ... (rest of implementation same as original plan Task 4)

} // namespace Kakoune
```

- [ ] **Step 3: Instantiate singleton in `src/main.cc`**

Find the block in `main.cc` where other singletons are created (search for `HighlighterRegistry` or `CommandManager`). Add near them:

```cpp
#include "language_registry.hh"
```

And in the `main()` function, after `HighlighterRegistry` but before `register_highlighters()`:

```cpp
String runtime_dir = runtime_directory(); // or equivalent path discovery
LanguageRegistry lang_registry{std::move(runtime_dir)};
```

Note: Check how Kakoune discovers its runtime directory (look for `runtime_directory()` or `KAKOUNE_RUNTIME` env var usage in `main.cc`) and use the same mechanism.

- [ ] **Step 4: Verify it compiles and unit tests pass**

```bash
make debug=yes -j$(sysctl -n hw.ncpu)
```

Expected: Compiles. `test_capture_face_name` unit test now passes.

- [ ] **Step 5: Commit**

```bash
git add src/language_registry.hh src/language_registry.cc src/main.cc
git commit -m "feat: add LanguageRegistry with grammar loading and query compilation"
```

---

## Task 5: Implement TreeSitterHighlighter (inside highlighters.cc)

**Files:**
- Modify: `src/highlighters.cc`

The highlighter lives inside `highlighters.cc` to access the file-local `highlight_range()` template function. This follows the same pattern as `RangesHighlighter`, `RegexHighlighter`, etc.

- [ ] **Step 1: Add includes at top of `highlighters.cc`**

```cpp
#include "syntax_tree.hh"
#include "language_registry.hh"
```

- [ ] **Step 2: Add TreeSitterHighlighter class before `register_highlighters()`**

```cpp
// --- Tree-sitter syntax highlighter ---

const HighlighterDesc tree_sitter_hl_desc = {
    "Use tree-sitter for syntax highlighting of the current buffer.\n"
    "Automatically detects language from buffer filetype.\n"
    "Requires a compiled grammar and highlights.scm for the filetype.",
    {}
};

struct TreeSitterHighlighter : Highlighter
{
    TreeSitterHighlighter()
        : Highlighter{HighlightPass::Colorize} {}

    static UniquePtr<Highlighter> create(HighlighterParameters params,
                                          Highlighter*)
    {
        if (params.size() != 0)
            throw runtime_error("tree-sitter highlighter takes no parameters");
        return make_unique_ptr<TreeSitterHighlighter>();
    }

private:
    void do_highlight(HighlightContext context, DisplayBuffer& display_buffer,
                      BufferRange range) override
    {
        auto& buffer = context.context.buffer();

        // Lazy-create syntax tree on first highlight
        if (not has_syntax_tree(buffer))
        {
            if (not LanguageRegistry::has_instance())
                return;

            auto filetype = context.context.options()["filetype"].get<String>();
            if (filetype.empty())
                return;

            auto language = LanguageRegistry::filetype_to_language(filetype);
            auto* config = LanguageRegistry::instance().get(language);
            if (not config)
                return;

            try
            {
                create_syntax_tree(buffer, config->language,
                                   config->highlight_query);
            }
            catch (runtime_error& err)
            {
                write_to_debug_buffer(format("tree-sitter: {}", err.what()));
                return;
            }
        }

        auto& syntax_tree = get_syntax_tree(buffer);
        syntax_tree.update(buffer);

        if (not syntax_tree.is_valid())
            return;

        auto& faces = context.context.faces();
        auto* query = syntax_tree.highlight_query();
        auto* tree = syntax_tree.tree();
        auto& byte_index = syntax_tree.byte_index();

        // Restrict query to visible byte range
        uint32_t start_byte = byte_index.byte_offset(range.begin);
        uint32_t end_byte = byte_index.byte_offset(range.end);

        // RAII cursor
        TSQueryCursor* cursor = ts_query_cursor_new();
        auto cleanup = on_scope_end([&]{ ts_query_cursor_delete(cursor); });

        ts_query_cursor_set_byte_range(cursor, start_byte, end_byte);
        ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));

        // Get capture → face mapping
        auto filetype = context.context.options()["filetype"].get<String>();
        auto lang_name = LanguageRegistry::filetype_to_language(filetype);
        auto* config = LanguageRegistry::instance().get(lang_name);
        if (not config)
            return;

        TSQueryMatch match;
        uint32_t capture_index;

        while (ts_query_cursor_next_capture(cursor, &match, &capture_index))
        {
            auto& capture = match.captures[capture_index];
            TSNode node = capture.node;

            TSPoint start_pt = ts_node_start_point(node);
            TSPoint end_pt = ts_node_end_point(node);

            BufferCoord begin_coord{LineCount{(int)start_pt.row},
                                    ByteCount{(int)start_pt.column}};
            BufferCoord end_coord{LineCount{(int)end_pt.row},
                                  ByteCount{(int)end_pt.column}};

            if ((int)capture.index >= (int)config->capture_faces.size())
                continue;

            auto& face_name = config->capture_faces[(int)capture.index];

            try
            {
                Face face = faces[face_name];
                highlight_range(display_buffer, begin_coord, end_coord, false,
                               [&](DisplayAtom& atom) {
                                   atom.face = merge_faces(atom.face, face);
                               });
            }
            catch (runtime_error&) {}  // face not defined — skip
        }
    }
};
```

- [ ] **Step 3: Register in `register_highlighters()`**

At the end of the `register_highlighters()` function, add:

```cpp
registry.insert({
    "tree-sitter",
    {TreeSitterHighlighter::create, &tree_sitter_hl_desc}
});
```

- [ ] **Step 4: Verify it compiles**

```bash
make debug=yes -j$(sysctl -n hw.ncpu)
```

Expected: Compiles successfully.

- [ ] **Step 5: Commit**

```bash
git add src/highlighters.cc
git commit -m "feat: add TreeSitterHighlighter with query execution and face mapping"
```

---

## Task 6: Add Commands

**Files:**
- Modify: `src/commands.cc`

Commands are registered directly via `cm.register_command()` inside `register_commands()`, since `CommandDesc` is local to this file.

- [ ] **Step 1: Add includes at top of `commands.cc`**

```cpp
#include "syntax_tree.hh"
#include "language_registry.hh"
```

- [ ] **Step 2: Add commands inside `register_commands()`**

At the end of `register_commands()`, add:

```cpp
cm.register_command("tree-sitter-enable",
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto filetype = context.options()["filetype"].get<String>();
        if (filetype.empty())
            throw runtime_error("buffer has no filetype set");

        if (not LanguageRegistry::has_instance())
            throw runtime_error("tree-sitter language registry not initialized");

        auto language = LanguageRegistry::filetype_to_language(filetype);
        auto* config = LanguageRegistry::instance().get(language);
        if (not config)
            throw runtime_error(format("no tree-sitter grammar for '{}'", language));

        if (not has_syntax_tree(buffer))
            create_syntax_tree(buffer, config->language, config->highlight_query);
    },
    "tree-sitter-enable: enable tree-sitter for the current buffer",
    {{}, ParameterDesc::Flags::None, 0, 0});

cm.register_command("tree-sitter-disable",
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        if (has_syntax_tree(buffer))
            remove_syntax_tree(buffer);
    },
    "tree-sitter-disable: disable tree-sitter for the current buffer",
    {{}, ParameterDesc::Flags::None, 0, 0});
```

- [ ] **Step 3: Verify it compiles**

```bash
make debug=yes -j$(sysctl -n hw.ncpu)
```

Expected: Compiles successfully.

- [ ] **Step 4: Commit**

```bash
git add src/commands.cc
git commit -m "feat: add tree-sitter-enable and tree-sitter-disable commands"
```

---

## Task 7: Add Default Faces and Auto-Enable Hook

**Files:**
- Create: `rc/tree-sitter.kak`

- [ ] **Step 1: Create `rc/tree-sitter.kak`**

Contains face definitions mapping `ts_*` → Kakoune semantic faces, plus a `WinSetOption` hook to auto-enable tree-sitter when filetype is set.

```kak
# Tree-sitter face definitions
set-face global ts_attribute              attribute
set-face global ts_comment                comment
set-face global ts_comment_block          comment
set-face global ts_comment_line           comment
set-face global ts_constant               value
set-face global ts_constant_builtin       builtin
set-face global ts_constructor            function
set-face global ts_function               function
set-face global ts_function_builtin       builtin
set-face global ts_function_macro         function
set-face global ts_function_method        function
set-face global ts_keyword                keyword
set-face global ts_keyword_control        keyword
set-face global ts_keyword_control_conditional keyword
set-face global ts_keyword_control_import keyword
set-face global ts_keyword_control_repeat keyword
set-face global ts_keyword_control_return keyword
set-face global ts_keyword_directive      meta
set-face global ts_keyword_function       keyword
set-face global ts_keyword_operator       operator
set-face global ts_keyword_storage        keyword
set-face global ts_keyword_storage_modifier keyword
set-face global ts_keyword_storage_type   type
set-face global ts_label                  meta
set-face global ts_namespace              module
set-face global ts_operator               operator
set-face global ts_property               variable
set-face global ts_punctuation            default
set-face global ts_punctuation_bracket    default
set-face global ts_punctuation_delimiter  default
set-face global ts_special                meta
set-face global ts_string                 string
set-face global ts_string_escape          value
set-face global ts_string_regexp          meta
set-face global ts_string_special         meta
set-face global ts_tag                    keyword
set-face global ts_type                   type
set-face global ts_type_builtin           builtin
set-face global ts_type_enum_variant      value
set-face global ts_variable               variable
set-face global ts_variable_builtin       builtin
set-face global ts_variable_other_member  variable
set-face global ts_variable_parameter     variable

# Auto-enable tree-sitter highlighting
hook -group tree-sitter-auto global WinSetOption filetype=.+ %{
    try %{
        add-highlighter window/tree-sitter tree-sitter
    }
}

hook -group tree-sitter-auto global WinSetOption filetype= %{
    try %{ remove-highlighter window/tree-sitter }
}
```

- [ ] **Step 2: Commit**

```bash
git add rc/tree-sitter.kak
git commit -m "feat: add default tree-sitter faces and auto-enable hook"
```

---

## Task 8: Add Runtime Queries and Build a Test Grammar

**Files:**
- Create: `runtime/queries/` (from Helix)
- Create: `runtime/grammars/` (compile at least C grammar)

- [ ] **Step 1: Copy query files from Helix**

```bash
cd /Users/saiful/Sites/rust/treesitter.kak
mkdir -p runtime/queries

for lang in rust c cpp python javascript typescript bash go json toml \
            html css markdown ruby lua yaml zig java kotlin swift; do
    if [ -d "/Users/saiful/Sites/rust/helix/runtime/queries/$lang" ]; then
        cp -r "/Users/saiful/Sites/rust/helix/runtime/queries/$lang" runtime/queries/
    fi
done
```

- [ ] **Step 2: Build at least the C grammar for testing**

The C grammar is the simplest (no custom scanner). This is required so end-to-end testing works.

```bash
cd /tmp
git clone --depth 1 https://github.com/tree-sitter/tree-sitter-c.git
cd tree-sitter-c

# Compile as shared library
cc -shared -o c.so -fPIC -Isrc src/parser.c

mkdir -p /Users/saiful/Sites/rust/treesitter.kak/runtime/grammars
cp c.so /Users/saiful/Sites/rust/treesitter.kak/runtime/grammars/

cd /tmp && rm -rf tree-sitter-c
```

- [ ] **Step 3: Verify grammar loads**

```bash
# Quick check that dlopen will find the entry point
nm -gU /Users/saiful/Sites/rust/treesitter.kak/runtime/grammars/c.so | grep tree_sitter_c
```

Expected: Shows `tree_sitter_c` symbol.

- [ ] **Step 4: Commit**

```bash
cd /Users/saiful/Sites/rust/treesitter.kak
git add runtime/queries/ runtime/grammars/
git commit -m "feat: add tree-sitter query files and compiled C grammar"
```

---

## Task 9: Functional Tests

**Files:**
- Create: `test/highlight/tree-sitter/`

- [ ] **Step 1: Create basic highlighting test**

```bash
mkdir -p test/highlight/tree-sitter/basic
```

Create `test/highlight/tree-sitter/basic/in`:
```c
int main() {
    return 0;
}
```

Create `test/highlight/tree-sitter/basic/cmd`:
```
nop
```

Create `test/highlight/tree-sitter/basic/rc`:
```kak
set-option buffer filetype c
add-highlighter buffer/tree-sitter tree-sitter
```

Create `test/highlight/tree-sitter/basic/out` (same as `in` — highlighting doesn't change text):
```c
int main() {
    return 0;
}
```

- [ ] **Step 2: Create test for tree-sitter-enable/disable commands**

```bash
mkdir -p test/highlight/tree-sitter/enable-disable
```

Create `test/highlight/tree-sitter/enable-disable/in`:
```c
int x = 42;
```

Create `test/highlight/tree-sitter/enable-disable/rc`:
```kak
set-option buffer filetype c
```

Create `test/highlight/tree-sitter/enable-disable/cmd`:
```
tree-sitter-enable
tree-sitter-disable
```

Create `test/highlight/tree-sitter/enable-disable/out`:
```c
int x = 42;
```

- [ ] **Step 3: Create incremental update test**

```bash
mkdir -p test/highlight/tree-sitter/incremental
```

Create `test/highlight/tree-sitter/incremental/in`:
```c
int hello() { return 0; }
```

Create `test/highlight/tree-sitter/incremental/rc`:
```kak
set-option buffer filetype c
add-highlighter buffer/tree-sitter tree-sitter
```

Create `test/highlight/tree-sitter/incremental/cmd`:
```
execute-keys %{/hello<ret>cworld<esc>}
```

Create `test/highlight/tree-sitter/incremental/out`:
```c
int world() { return 0; }
```

- [ ] **Step 4: Set runtime dir for tests**

Create `test/highlight/tree-sitter/basic/env` (and copy to other test dirs):
```bash
export KAKOUNE_RUNTIME="$PWD"
```

Copy to other test dirs:
```bash
cp test/highlight/tree-sitter/basic/env test/highlight/tree-sitter/enable-disable/env
cp test/highlight/tree-sitter/basic/env test/highlight/tree-sitter/incremental/env
```

- [ ] **Step 5: Run tests**

```bash
cd /Users/saiful/Sites/rust/treesitter.kak
test/run test/highlight/tree-sitter/
```

Expected: All tests pass.

- [ ] **Step 6: Commit**

```bash
git add test/highlight/tree-sitter/
git commit -m "test: add functional tests for tree-sitter highlighting"
```

---

## Task 10: Final Verification

- [ ] **Step 1: Clean build**

```bash
make clean && make -j$(sysctl -n hw.ncpu)
```

Expected: Builds successfully.

- [ ] **Step 2: Debug build with unit tests**

```bash
make clean && make debug=yes -j$(sysctl -n hw.ncpu)
```

Expected: Compiles, unit tests pass.

- [ ] **Step 3: Full test suite**

```bash
make test
```

Expected: All existing Kakoune tests pass + tree-sitter tests pass.

- [ ] **Step 4: Manual smoke test**

```bash
echo '#include <stdio.h>
int main() {
    printf("hello world\n");
    return 0;
}' > /tmp/test.c

KAKOUNE_RUNTIME=$PWD src/kak /tmp/test.c
```

In Kakoune, verify:
- `int`, `return` colored as keywords
- `"hello world\n"` colored as string
- `main`, `printf` colored as functions
- Run `:tree-sitter-disable` — highlighting reverts
- Run `:tree-sitter-enable` — highlighting returns

- [ ] **Step 5: Final commit**

```bash
git add -A
git commit -m "feat: phase 1 complete — core tree-sitter integration with highlighting"
```

---

## Dependency Graph

```
Task 1 (Fork)
  └→ Task 2 (Vendor tree-sitter)
       └→ Task 3 (SyntaxTree)
            └→ Task 4 (LanguageRegistry)
                 └→ Task 5 (TreeSitterHighlighter in highlighters.cc)
                      └→ Task 6 (Commands in commands.cc)
                           ├→ Task 7 (Faces & Hook)
                           ├→ Task 8 (Queries + C Grammar)
                           └→ Task 9 (Tests)
                                └→ Task 10 (Final Verification)
```

---

## Notes for Implementers

### Key Kakoune APIs:

- `src/value.hh` — `ValueMap`, `ValueId`, `get_free_value_id()` for per-buffer storage
- `src/highlighters.cc:49-87` — `highlight_range()` template (file-local, not in any header)
- `src/highlighters.cc:155-168` — `BufferSideCache<T>` pattern
- `src/word_db.cc:13-20` — example of per-buffer `ValueId` usage
- `src/scope.hh` — `on_scope_end()` RAII helper
- `src/buffer.hh:181-212` — buffer content access
- `src/changes.hh` — `update_ranges()` and change tracking
- `src/debug.hh` — `write_to_debug_buffer()`

### Key tree-sitter APIs:

- `ts_parser_new()`, `ts_parser_set_language()`, `ts_parser_parse()` — parser lifecycle
- `ts_parser_set_timeout_micros()` — per-parse timeout
- `ts_tree_edit()` — incremental edit
- `ts_query_new()`, `ts_query_cursor_new()`, `ts_query_cursor_exec()` — query system
- `ts_query_cursor_next_capture()` — iterate highlights
- `ts_query_cursor_set_byte_range()` — restrict to visible region
- `ts_query_cursor_delete()` — free cursor (use RAII wrapper)
- `ts_query_capture_name_for_id()` — get capture name by index
- `ts_node_start_point()`, `ts_node_end_point()` — node coordinates

### Grammar compilation:

```bash
# Compile any tree-sitter grammar as a .so:
cd tree-sitter-<lang>
cc -shared -o <lang>.so -fPIC -Isrc src/parser.c [src/scanner.c]
cp <lang>.so /path/to/runtime/grammars/
```
