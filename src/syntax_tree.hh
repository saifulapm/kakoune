#ifndef syntax_tree_hh_INCLUDED
#define syntax_tree_hh_INCLUDED

#include "tree_sitter.hh"
#include "array_view.hh"
#include "buffer.hh"
#include "event_manager.hh"
#include "string.hh"
#include "utils.hh"
#include "vector.hh"

#include <cstdint>
#include <atomic>
#include <thread>

namespace Kakoune
{

class LanguageConfig;

struct QueryCursorGuard
{
    TSQueryCursor* cursor;
    QueryCursorGuard() : cursor(ts_query_cursor_new()) {}
    ~QueryCursorGuard() { if (cursor) ts_query_cursor_delete(cursor); }
    QueryCursorGuard(const QueryCursorGuard&) = delete;
    QueryCursorGuard& operator=(const QueryCursorGuard&) = delete;
    operator TSQueryCursor*() { return cursor; }
    TSQueryCursor* operator->() { return cursor; }
};

struct InjectionLayer
{
    TSParser* parser = nullptr;
    TSTree* tree = nullptr;
    String language_name;
    const LanguageConfig* config = nullptr;  // not owned
    Vector<TSRange, MemoryDomain::Highlight> ranges;

    // Identity for incremental reparse: the ts_node_id of the host content
    // node that produced this layer (the first one, for combined injections).
    // Survives host reparse iff the subtree was reused, so it's a reliable
    // match key for pooling the layer's tree across edits. Stored as
    // uintptr_t so HashMap can hash it directly.
    uintptr_t content_node_id = 0;

    // Combined injection (#set! injection.combined): all content nodes of this
    // language in the host layer form ONE logical document for this single
    // layer. Because it's a per-(host-layer, language) singleton, its old tree
    // can be reused for incremental reparse even when the host content node id
    // changes (which it does on every edit inside the injected region) — the
    // node-id pool would otherwise miss and force a full reparse every
    // keystroke. Mirrors Helix's combined-injection Slab reuse.
    bool combined = false;

    InjectionLayer() = default;
    ~InjectionLayer();
    InjectionLayer(InjectionLayer&&) noexcept;
    InjectionLayer& operator=(InjectionLayer&&) noexcept;
    InjectionLayer(const InjectionLayer&) = delete;
    InjectionLayer& operator=(const InjectionLayer&) = delete;
};

struct LineByteIndex
{
    void rebuild(const Buffer& buffer);
    // Adopt a precomputed line→start-byte table (the worker already built
    // this from its snapshot; avoids re-touching the live Buffer).
    void rebuild_from_offsets(const Vector<uint32_t>& offsets);
    uint32_t byte_offset(BufferCoord coord) const;

private:
    Vector<uint32_t, MemoryDomain::Highlight> m_line_start_bytes;
};

struct DeferParse {};  // tag: build an empty SyntaxTree, no initial parse

class SyntaxTree
{
public:
    SyntaxTree(const Buffer& buffer, const LanguageConfig* config);
    // Empty tree: parser is created but nothing is parsed. The host tree is
    // supplied later via adopt_tree() (used with AsyncSyntaxTree so the first
    // parse also happens on the worker, not synchronously in this ctor).
    SyntaxTree(DeferParse, const LanguageConfig* config);
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

    ConstArrayView<InjectionLayer> injection_layers() const { return m_injection_layers; }
    void detect_injections(const Buffer& buffer);
    const LanguageConfig* config() const;  // re-resolves from registry each call
    const String& language_name() const { return m_language_name; }

    // Adopt a host tree parsed elsewhere (by the background worker) plus its
    // byte index, as of buffer timestamp `ts`. Takes ownership of `tree`.
    // Used by AsyncSyntaxTree: the worker does the expensive host parse, the
    // main thread installs the result here and runs the (cheap, cached,
    // main-thread-only) injection detection through the normal path.
    void adopt_tree(TSTree* tree, LineByteIndex&& byte_index, size_t ts);

private:
    void full_parse(const Buffer& buffer);

    TSParser* m_parser = nullptr;
    TSTree* m_tree = nullptr;
    TSLanguage* m_language = nullptr;
    TSQuery* m_highlight_query = nullptr;  // not owned — owned by LanguageConfig
    String m_language_name;                // used to re-resolve config from registry
    LineByteIndex m_byte_index;
    size_t m_timestamp = 0;
    size_t m_injection_timestamp = 0;
    Vector<InjectionLayer, MemoryDomain::Highlight> m_injection_layers;
    // Edits applied to m_tree since last detect_injections call; replayed onto
    // each pooled injection layer tree so they stay in the post-edit byte
    // coordinate space before incremental reparse.
    Vector<TSInputEdit, MemoryDomain::Highlight> m_pending_edits;
};

void create_syntax_tree(const Buffer& buffer, const LanguageConfig* config);
void create_syntax_tree_deferred(const Buffer& buffer, const LanguageConfig* config);
SyntaxTree& get_syntax_tree(const Buffer& buffer);
void remove_syntax_tree(const Buffer& buffer);
bool has_syntax_tree(const Buffer& buffer);

// ---------------------------------------------------------------------------
// Background (off-render-thread) parsing.
//
// kakoune is single-threaded: highlighting runs synchronously inside the
// redraw path, so a slow tree-sitter parse blocks the UI. The grammar fix
// makes the common case fast, but a pathologically large file or a slow
// grammar would still stall keystrokes. AsyncSyntaxTree removes that risk:
// do_highlight always uses the latest *completed* tree immediately (possibly
// one keystroke stale) and never blocks. When the buffer changes a worker
// thread parses a private snapshot of the buffer text; on completion it wakes
// the event loop through a self-pipe and the main thread swaps the fresh tree
// in and requests a redraw.
//
// Thread-safety contract:
//  - The worker only ever touches data it solely owns (a deep-copied text
//    snapshot, its own TSParser/TSTree). It calls no kakoune singletons and
//    never reads the live Buffer (StringData refcounts are non-atomic).
//  - Everything else (FDWatcher, redraw, Buffer access, the consumable tree)
//    runs on the main thread only.
// ---------------------------------------------------------------------------

// An immutable, worker-owned copy of buffer text plus the line→byte offset
// table the tree-sitter input reader needs. Built on the main thread.
struct BufferSnapshot
{
    Vector<String> lines;
    Vector<uint32_t> line_start_bytes;  // size == lines.size()
    size_t timestamp = 0;

    void build(const Buffer& buffer);
    uint32_t total_bytes() const
    {
        return line_start_bytes.empty() ? 0
            : line_start_bytes.back()
              + (uint32_t)(int)lines.back().length();
    }
};

// Result the worker produces and the main thread consumes.
//
// The worker parses only the HOST tree — that is the part that was
// pathologically slow (2.3s → ms). Injection detection stays on the main
// thread (it needs LanguageRegistry and Buffer text for language/predicate
// resolution, both main-thread-only) and is already well-cached, so moving
// only the host parse off-thread removes the stall without duplicating the
// injection machinery or risking races in it.
//
// Threading contract: only the tree-sitter *parse* runs off-thread. The
// per-edit buffer snapshot is still copied synchronously on the main thread
// (cheap next to a parse, but not free), and ~AsyncSyntaxTree joins the
// worker (bounded by the parse budget). do_highlight never blocks on the
// parse itself.
struct ParsedSyntax
{
    TSTree* tree = nullptr;
    LineByteIndex byte_index;
    size_t timestamp = 0;

    ParsedSyntax() = default;
    ~ParsedSyntax();
    ParsedSyntax(ParsedSyntax&&) noexcept;
    ParsedSyntax& operator=(ParsedSyntax&&) noexcept;
    ParsedSyntax(const ParsedSyntax&) = delete;
    ParsedSyntax& operator=(const ParsedSyntax&) = delete;
    bool is_valid() const { return tree != nullptr; }
};

// Threshold-gated background parsing.
//
// Spawning a worker thread + pipe + FDWatcher per buffer is wasteful when the
// parse is fast (the common case: small files, simple grammars). So this
// starts in SYNCHRONOUS mode — poll() parses on the main thread and times it.
// Only when a parse exceeds m_async_threshold does the buffer switch to
// BACKGROUND mode (lazily creating the pipe/FDWatcher/worker), after which it
// never blocks the render thread again. Mirrors Helix: parse inline; only the
// genuinely slow files pay the async machinery.
class AsyncSyntaxTree
{
public:
    AsyncSyntaxTree(const Buffer& buffer, const LanguageConfig* config);
    ~AsyncSyntaxTree();

    AsyncSyntaxTree(const AsyncSyntaxTree&) = delete;
    AsyncSyntaxTree& operator=(const AsyncSyntaxTree&) = delete;

    // Main thread. In sync mode: parse now (timed). In background mode: kick a
    // worker if the buffer advanced and none is in flight. Never blocks on a
    // background parse.
    void poll(const Buffer& buffer);

    const ParsedSyntax& current() const { return m_current; }
    bool is_valid() const { return m_current.is_valid(); }

private:
    void parse_sync(const Buffer& buffer);   // main thread; may flip to bg
    void enter_background_mode();             // lazily set up pipe/watcher
    void on_worker_done();                    // main thread, from FDWatcher
    void start_job(const Buffer& buffer);     // background mode only

    const LanguageConfig* m_config;
    String m_language_name;
    const Buffer* m_buffer;       // stable: AsyncSyntaxTree lives in its values()

    ParsedSyntax m_current;       // main-thread only
    bool m_background_mode = false;

    // Background-mode-only state (unset until enter_background_mode()).
    std::thread m_worker;
    std::atomic<bool> m_job_running{false};
    int m_wake_pipe[2] = {-1, -1};
    UniquePtr<FDWatcher> m_watcher;
    BufferSnapshot m_pending_snapshot;   // main writes pre-launch, worker reads
    ParsedSyntax m_worker_result;        // worker writes, main reads post-join
    size_t m_dispatched_timestamp = (size_t)-1;
    bool m_last_parse_timed_out = false;
};

void create_async_syntax_tree(const Buffer& buffer, const LanguageConfig* config);
AsyncSyntaxTree& get_async_syntax_tree(const Buffer& buffer);
bool has_async_syntax_tree(const Buffer& buffer);
void remove_async_syntax_tree(const Buffer& buffer);

// Ensure the buffer has a deferred SyntaxTree and an AsyncSyntaxTree, poll the
// background parser, and install the latest completed host tree into the
// SyntaxTree (when it advanced). Returns the ready SyntaxTree, or nullptr if
// the first parse is still in flight or setup failed. This is the single
// entry point highlighters use; tree ownership stays out of the render layer.
SyntaxTree* sync_async_host_tree(const Buffer& buffer, const LanguageConfig* config);

} // namespace Kakoune

#endif // syntax_tree_hh_INCLUDED
