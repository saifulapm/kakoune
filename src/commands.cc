#include "commands.hh"

#include "alias_registry.hh"
#include "buffer.hh"
#include "buffer_manager.hh"
#include "buffer_utils.hh"
#include "client.hh"
#include "client_manager.hh"
#include "command_manager.hh"
#include "completion.hh"
#include "context.hh"
#include "debug.hh"
#include "event_manager.hh"
#include "face_registry.hh"
#include "file.hh"
#include "hash_map.hh"
#include "hook_manager.hh"
#include "highlighter.hh"
#include "highlighters.hh"
#include "input_handler.hh"
#include "insert_completer.hh"
#include "keymap_manager.hh"
#include "normal.hh"
#include "option_manager.hh"
#include "option_types.hh"
#include "parameters_parser.hh"
#include "profile.hh"
#include "ranges.hh"
#include "ranked_match.hh"
#include "regex.hh"
#include "register_manager.hh"
#include "remote.hh"
#include "shell_manager.hh"
#include "string.hh"
#include "language_registry.hh"
#include "syntax_tree.hh"
#include "user_interface.hh"
#include "window.hh"

#include <utility>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#if defined(__GLIBC__) || defined(__CYGWIN__)
#include <malloc.h>
#endif

namespace Kakoune
{

extern const char* version;

struct LocalScope : Scope
{
    LocalScope(Context& context)
        : Scope(context.scope()), m_context{context}
    {
        m_context.m_local_scopes.push_back(this);
    }

    ~LocalScope()
    {
        kak_assert(not m_context.m_local_scopes.empty() and m_context.m_local_scopes.back() == this);
        m_context.m_local_scopes.pop_back();
    }

private:
    Context& m_context;
};

namespace
{

Buffer* open_fifo(StringView name, StringView filename, Buffer::Flags flags, bool scroll)
{
    int fd = open(parse_filename(filename).c_str(), O_RDONLY | O_NONBLOCK);
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    if (fd < 0)
       throw runtime_error(format("unable to open '{}'", filename));

    return create_fifo_buffer(name.str(), fd, flags, scroll ? AutoScroll::Yes : AutoScroll::No);
}

template<typename... Completers> struct PerArgumentCommandCompleter;

template<> struct PerArgumentCommandCompleter<>
{
    Completions operator()(const Context&, CommandParameters,
                           size_t, ByteCount) const { return {}; }
};

template<typename Completer, typename... Rest>
struct PerArgumentCommandCompleter<Completer, Rest...> : PerArgumentCommandCompleter<Rest...>
{
    template<typename C, typename... R>
        requires (not std::is_base_of_v<PerArgumentCommandCompleter<>, std::remove_reference_t<C>>)
    PerArgumentCommandCompleter(C&& completer, R&&... rest)
      : PerArgumentCommandCompleter<Rest...>(std::forward<R>(rest)...),
        m_completer(std::forward<C>(completer)) {}

    Completions operator()(const Context& context,
                           CommandParameters params, size_t token_to_complete,
                           ByteCount pos_in_token)
    {
        if (token_to_complete == 0)
        {
            const String& arg = token_to_complete < params.size() ?
                                params[token_to_complete] : String();
            return m_completer(context, arg, pos_in_token);
        }
        return PerArgumentCommandCompleter<Rest...>::operator()(
            context, params.subrange(1),
            token_to_complete-1, pos_in_token);
    }

    Completer m_completer;
};

template<typename... Completers>
PerArgumentCommandCompleter<std::decay_t<Completers>...>
make_completer(Completers&&... completers)
{
    return {std::forward<Completers>(completers)...};
}

template<typename Completer>
auto add_flags(Completer completer, Completions::Flags completions_flags)
{
    return [completer=std::move(completer), completions_flags]
           (const Context& context, StringView prefix, ByteCount cursor_pos) {
        Completions res = completer(context, prefix, cursor_pos);
        res.flags |= completions_flags;
        return res;
    };
}

template<typename Completer>
auto menu(Completer completer)
{
    return add_flags(std::move(completer), Completions::Flags::Menu);
}

template<bool menu>
auto filename_completer = make_completer(
    [](const Context& context, StringView prefix, ByteCount cursor_pos)
    { return Completions{ 0_byte, cursor_pos,
                          complete_filename(prefix,
                                            context.options()["ignored_files"].get<Regex>(),
                                            cursor_pos, FilenameFlags::Expand),
                                            menu ? Completions::Flags::Menu : Completions::Flags::None}; });

template<bool menu>
auto filename_arg_completer =
    [](const Context& context, StringView prefix, ByteCount cursor_pos) -> Completions
    { return { 0_byte, cursor_pos,
               complete_filename(prefix,
                                 context.options()["ignored_files"].get<Regex>(),
                                 cursor_pos, FilenameFlags::OnlyDirectories),
               menu ? Completions::Flags::Menu : Completions::Flags::None }; };

auto client_arg_completer =
    [](const Context& context, StringView prefix, ByteCount cursor_pos) -> Completions
    { return { 0_byte, cursor_pos,
               ClientManager::instance().complete_client_name(prefix, cursor_pos),
               Completions::Flags::Menu }; };

auto arg_completer = [](auto candidates) -> PromptCompleter {
    return [=](const Context& context, StringView prefix, ByteCount cursor_pos) -> Completions {
        return Completions{ 0_byte, cursor_pos, complete(prefix, cursor_pos, candidates), Completions::Flags::Menu };
    };
};

template<bool ignore_current = false>
static Completions complete_buffer_name(const Context& context, StringView prefix, ByteCount cursor_pos)
{
    struct RankedMatchAndBuffer : RankedMatch
    {
        RankedMatchAndBuffer(RankedMatch  m, const Buffer* b)
            : RankedMatch{std::move(m)}, buffer{b} {}

        using RankedMatch::operator==;
        using RankedMatch::operator<;

        const Buffer* buffer;
    };

    StringView query = prefix.substr(0, cursor_pos);
    Vector<RankedMatchAndBuffer> filename_matches;
    Vector<RankedMatchAndBuffer> matches;
    for (const auto& buffer : BufferManager::instance())
    {
        if (ignore_current and buffer.get() == &context.buffer())
            continue;

        StringView bufname = buffer->display_name();
        if (buffer->flags() & Buffer::Flags::File)
        {
            if (RankedMatch match{split_path(bufname).second, query})
            {
                filename_matches.emplace_back(match, buffer.get());
                continue;
            }
        }
        if (RankedMatch match{bufname, query})
            matches.emplace_back(match, buffer.get());
    }
    std::sort(filename_matches.begin(), filename_matches.end());
    std::sort(matches.begin(), matches.end());

    CandidateList res;
    for (auto& match : filename_matches)
        res.push_back(match.buffer->display_name());
    for (auto& match : matches)
        res.push_back(match.buffer->display_name());

    return { 0, cursor_pos, res };
}

template<typename Func>
auto make_single_word_completer(Func&& func)
{
    return make_completer(
        [func = std::move(func)](const Context& context,
               StringView prefix, ByteCount cursor_pos) -> Completions {
            auto candidate = { func(context) };
            return { 0_byte, cursor_pos, complete(prefix, cursor_pos, candidate) }; });
}

const ParameterDesc no_params{ {}, ParameterDesc::Flags::None, 0, 0 };
const ParameterDesc single_param{ {}, ParameterDesc::Flags::None, 1, 1 };
const ParameterDesc single_optional_param{ {}, ParameterDesc::Flags::None, 0, 1 };
const ParameterDesc double_params{ {}, ParameterDesc::Flags::None, 2, 2 };

static Completions complete_scope(const Context&,
                                  StringView prefix, ByteCount cursor_pos)
{
   static constexpr StringView scopes[] = { "global", "buffer", "window", "local"};
   return { 0_byte, cursor_pos, complete(prefix, cursor_pos, scopes) };
}

static Completions complete_scope_including_current(const Context&,
                                  StringView prefix, ByteCount cursor_pos)
{
   static constexpr StringView scopes[] = { "global", "buffer", "window", "local", "current" };
   return { 0_byte, cursor_pos, complete(prefix, cursor_pos, scopes) };
}

static Completions complete_scope_no_global(const Context&,
                                            StringView prefix, ByteCount cursor_pos)
{
   static constexpr StringView scopes[] = { "buffer", "window", "local", "current" };
   return { 0_byte, cursor_pos, complete(prefix, cursor_pos, scopes) };
}


static Completions complete_command_name(const Context& context,
                                         StringView prefix, ByteCount cursor_pos)
{
   return CommandManager::instance().complete_command_name(
       context, prefix.substr(0, cursor_pos));
}

struct AsyncShellScript
{
    AsyncShellScript(String shell_script,
                     Completions::Flags flags = Completions::Flags::None)
      : m_shell_script{std::move(shell_script)}, m_flags(flags) {}

    AsyncShellScript(const AsyncShellScript& other) : m_shell_script{other.m_shell_script}, m_flags(other.m_flags) {}
    AsyncShellScript& operator=(const AsyncShellScript& other) {  m_shell_script = other.m_shell_script; m_flags = other.m_flags; return *this; }

protected:
    void spawn_script(const Context& context, const ShellContext& shell_context, auto&& handle_line)
    {
        m_handle_line = handle_line;
        m_running_script.emplace(ShellManager::instance().spawn(m_shell_script, context, false, shell_context));
        m_watcher.emplace((int)m_running_script->out, FdEvents::Read, EventMode::Urgent,
                          [this, &input_handler=context.input_handler()](auto&&... args) { read_stdout(input_handler); });
    }

    void read_stdout(InputHandler& input_handler)
    {
        char buffer[2048];
        bool closed = false;
        int fd = (int)m_running_script->out;
        while (fd_readable(fd))
        {
            int size = read(fd, buffer, sizeof(buffer));
            if (size == 0)
            {
                closed = true;
                break;
            }
            m_stdout_buffer.insert(m_stdout_buffer.end(), buffer, buffer + size);
        }
        auto end = closed ? m_stdout_buffer.end() : find(m_stdout_buffer | reverse(), '\n').base();
        for (auto c : ArrayView(m_stdout_buffer.begin(), end) | split<StringView>('\n')
                                                              | filter([](auto s) { return not s.empty(); }))
            m_handle_line(c);

        m_stdout_buffer.erase(m_stdout_buffer.begin(), end);

        input_handler.refresh_ifn();
        if (closed)
        {
            m_running_script.reset();
            m_watcher.reset();
            m_handle_line = {};
        }
    }

    String m_shell_script;
    Optional<Shell> m_running_script;
    Optional<FDWatcher> m_watcher;
    Vector<char, MemoryDomain::Completion> m_stdout_buffer;
    Function<void (StringView)> m_handle_line;
    Completions::Flags m_flags;
};

struct ShellScriptCompleter : AsyncShellScript
{
    using AsyncShellScript::AsyncShellScript;

    Completions operator()(const Context& context,
                           CommandParameters params, size_t token_to_complete,
                           ByteCount pos_in_token)
    {
        CandidateList candidates;
        if (m_last_token != token_to_complete or pos_in_token != m_last_pos_in_token)
        {
            ShellContext shell_context{
                params,
                { { "token_to_complete", to_string(token_to_complete) },
                  { "pos_in_token",      to_string(pos_in_token) } }
            };
            spawn_script(context, shell_context, [this](StringView line) { m_candidates.push_back(line.str()); });

            candidates = std::move(m_candidates); // avoid completion menu flicker by keeping the previous result visible
            m_candidates.clear();
            m_last_token = token_to_complete;
            m_last_pos_in_token = pos_in_token;
        }
        else
            candidates = m_candidates;

        return {0_byte, pos_in_token, std::move(candidates), m_flags};
    }

private:
    CandidateList m_candidates;
    int m_last_token = -1;
    ByteCount m_last_pos_in_token = -1;
};

struct ShellCandidatesCompleter : AsyncShellScript
{
    using AsyncShellScript::AsyncShellScript;

    Completions operator()(const Context& context,
                           CommandParameters params, size_t token_to_complete,
                           ByteCount pos_in_token)
    {
        if (m_last_token != token_to_complete)
        {
            ShellContext shell_context{
                params,
                { { "token_to_complete", to_string(token_to_complete) } }
            };
            spawn_script(context, shell_context, [this](StringView line) { m_candidates.emplace_back(line.str(), used_letters(line)); });
            m_candidates.clear();
            m_last_token = token_to_complete;
        }
        return rank_candidates(params[token_to_complete].substr(0, pos_in_token));
    }

private:
    Completions rank_candidates(StringView query)
    {
        UsedLetters query_letters = used_letters(query);
        Vector<RankedMatch> matches;
        for (auto&& [i, candidate] : m_candidates | enumerate())
        {
            if (RankedMatch m{candidate.first, candidate.second, query, query_letters})
            {
                m.set_input_sequence_number(i);
                matches.push_back(m);
            }
        }

        constexpr size_t max_count = 100;
        CandidateList res;
        // Gather best max_count matches
        for_n_best(matches, max_count, [](auto& lhs, auto& rhs) { return rhs < lhs; }, [&] (const RankedMatch& m) {
            if (not res.empty() and res.back() == m.candidate())
                return false;
            res.push_back(m.candidate().str());
            return true;
        });

        return Completions{0_byte, query.length(), std::move(res), m_flags};
    }

    Vector<std::pair<String, UsedLetters>, MemoryDomain::Completion> m_candidates;
    int m_last_token = -1;
};

template<typename Completer>
struct PromptCompleterAdapter
{
    PromptCompleterAdapter(Completer completer) : m_completer{std::move(completer)} {}

    operator PromptCompleter() &&
    {
        if (not m_completer)
            return {};
        return [completer=std::move(m_completer)](const Context& context,
                                                  StringView prefix, ByteCount cursor_pos) {
            return completer(context, {String{String::NoCopy{}, prefix}}, 0, cursor_pos);
        };
    }

private:
    Completer m_completer;
};

Scope* get_scope_ifp(StringView scope, const Context& context)
{
    if (prefix_match("global", scope))
        return &GlobalScope::instance();
    else if (prefix_match("buffer", scope))
        return &context.buffer();
    else if (prefix_match("window", scope))
        return &context.window();
    else if (prefix_match("local", scope))
        return context.local_scope();
    else if (prefix_match(scope, "buffer="))
        return &BufferManager::instance().get_buffer(scope.substr(7_byte));
    return nullptr;
}

Scope& get_scope(StringView scope, const Context& context)
{
    if (auto s = get_scope_ifp(scope, context))
        return *s;
    throw runtime_error(format("no such scope: '{}'", scope));
}

struct CommandDesc
{
    const char* name;
    const char* alias;
    const char* docstring;
    ParameterDesc params;
    CommandFlags flags;
    CommandHelper helper;
    CommandCompleter completer;
    void (*func)(const ParametersParser&, Context&, const ShellContext&);
};

template<bool force_reload>
void edit(const ParametersParser& parser, Context& context, const ShellContext&)
{
    const bool scratch = (bool)parser.get_switch("scratch");

    if (parser.positional_count() == 0 and not force_reload and not scratch)
        throw wrong_argument_count();

    const bool no_hooks = context.hooks_disabled();
    const auto flags = (no_hooks ? Buffer::Flags::NoHooks : Buffer::Flags::None) |
       (parser.get_switch("debug") ? Buffer::Flags::Debug : Buffer::Flags::None);

    auto& buffer_manager = BufferManager::instance();
    const auto& name = parser.positional_count() > 0 ?
        parser[0] : (scratch ? generate_buffer_name("*scratch-{}*") : context.buffer().name());

    Buffer* buffer = buffer_manager.get_buffer_ifp(name);
    if (scratch)
    {
        if (parser.get_switch("readonly") or parser.get_switch("fifo") or parser.get_switch("scroll"))
            throw runtime_error("scratch is not compatible with readonly, fifo or scroll");

        if (buffer == nullptr or force_reload)
        {
            if (buffer != nullptr and force_reload)
                buffer_manager.delete_buffer(*buffer);
            buffer = create_buffer_from_string(name, flags, {});
        }
        else if (buffer->flags() & Buffer::Flags::File)
            throw runtime_error(format("buffer '{}' exists but is not a scratch buffer", name));
    }
    else if (force_reload and buffer and buffer->flags() & Buffer::Flags::File)
    {
        reload_file_buffer(*buffer);
    }
    else
    {
        if (auto fifo = parser.get_switch("fifo"))
            buffer = open_fifo(name, *fifo, flags, (bool)parser.get_switch("scroll"));
        else if (not buffer)
        {
            buffer = parser.get_switch("existing") ? open_file_buffer(name, flags)
                                                   : open_or_create_file_buffer(name, flags);
            if (buffer->flags() & Buffer::Flags::New)
                context.print_status({format("new file '{}'", name), context.faces()["StatusLine"]});
        }

        buffer->flags() &= ~Buffer::Flags::NoHooks;
        if (parser.get_switch("readonly"))
        {
            buffer->flags() |= Buffer::Flags::ReadOnly;
            buffer->options().get_local_option("readonly").set(true);
        }
    }

    Buffer* current_buffer = context.has_buffer() ? &context.buffer() : nullptr;

    const size_t param_count = parser.positional_count();
    if (current_buffer and (buffer != current_buffer or param_count > 1))
        context.push_jump();

    if (buffer != current_buffer)
        context.change_buffer(*buffer);
    buffer = &context.buffer(); // change_buffer hooks might change the buffer again

    if (parser.get_switch("fifo") and not parser.get_switch("scroll"))
        context.selections_write_only() = { *buffer, Selection{} };
    else if (param_count > 1 and not parser[1].empty())
    {
        int line = std::max(0, str_to_int(parser[1]) - 1);
        int column = param_count > 2 and not parser[2].empty() ?
                     std::max(0, str_to_int(parser[2]) - 1) : 0;

        auto& buffer = context.buffer();
        context.selections_write_only() = { buffer, buffer.clamp({ line,  column }) };
        if (context.has_window())
            context.window().center_line(context.selections().main().cursor().line);
    }
}

ParameterDesc edit_params{
    { { "existing", { {}, "fail if the file does not exist, do not open a new file" } },
      { "scratch",  { {}, "create a scratch buffer, not linked to a file" } },
      { "debug",    { {}, "create buffer as debug output" } },
      { "fifo",     { {filename_arg_completer<true>},  "create a buffer reading its content from a named fifo" } },
      { "readonly", { {}, "create a buffer in readonly mode" } },
      { "scroll",   { {}, "place the initial cursor so that the fifo will scroll to show new data" } } },
      ParameterDesc::Flags::None, 0, 3
};
const CommandDesc edit_cmd = {
    "edit",
    "e",
    "edit [<switches>] <filename> [<line> [<column>]]: open the given filename in a buffer",
    edit_params,
    CommandFlags::None,
    CommandHelper{},
    filename_completer<false>,
    edit<false>
};

const CommandDesc force_edit_cmd = {
    "edit!",
    "e!",
    "edit! [<switches>] <filename> [<line> [<column>]]: open the given filename in a buffer, "
    "force reload if needed",
    edit_params,
    CommandFlags::None,
    CommandHelper{},
    filename_completer<false>,
    edit<true>
};

const ParameterDesc write_params = {
    {
        { "sync", { {}, "force the synchronization of the file onto the filesystem" } },
        { "method", { {arg_completer(Array{"replace", "overwrite"})}, "explicit writemethod (replace|overwrite)" } },
        { "force", { {}, "Allow overwriting existing file with explicit filename" } }
    },
    ParameterDesc::Flags::None, 0, 1
};

const ParameterDesc write_params_except_force = {
    {
        { "sync", { {}, "force the synchronization of the file onto the filesystem" } },
        { "method", { {arg_completer(Array{"replace", "overwrite"})}, "explicit writemethod (replace|overwrite)" } },
    },
    ParameterDesc::Flags::None, 0, 1
};

auto parse_write_method(StringView str)
{
    constexpr auto desc = enum_desc(Meta::Type<WriteMethod>{});
    auto it = find_if(desc, [str](const EnumDesc<WriteMethod>& d) { return d.name == str; });
    if (it == desc.end())
        throw runtime_error(format("invalid writemethod '{}'", str));
    return it->value;
}

void do_write_buffer(Context& context, Optional<String> filename, WriteFlags flags, Optional<WriteMethod> write_method = {})
{
    Buffer& buffer = context.buffer();
    const bool is_file = (bool)(buffer.flags() & Buffer::Flags::File);

    if (not filename and !is_file)
        throw runtime_error("cannot write a non file buffer without a filename");

    const bool is_readonly = (bool)(context.buffer().flags() & Buffer::Flags::ReadOnly);
    // if the buffer is in read-only mode and we try to save it directly
    // or we try to write to it indirectly using e.g. a symlink, throw an error
    if (is_file and is_readonly and
        (not filename or real_path(*filename) == buffer.filename()))
        throw runtime_error("cannot overwrite the buffer when in readonly mode");

    auto effective_filename = filename ? parse_filename(*filename) : buffer.filename();
    if (filename and not (flags & WriteFlags::Force) and
        real_path(effective_filename) != buffer.filename() and
        regular_file_exists(effective_filename))
        throw runtime_error("cannot overwrite existing file without -force");

    auto method = write_method.value_or_compute([&] { return context.options()["writemethod"].get<WriteMethod>(); });

    context.hooks().run_hook(Hook::BufWritePre, effective_filename, context);
    BusyIndicator busy_indicator{context, [&](std::chrono::seconds elapsed) {
        return DisplayLine{format("waiting while writing buffer '{}' ({}s)", buffer.filename(), elapsed.count()),
                           context.faces()["Information"]};
    }};
    write_buffer_to_file(buffer, effective_filename, method, flags);
    context.hooks().run_hook(Hook::BufWritePost, effective_filename, context);
}

template<bool force = false>
void write_buffer(const ParametersParser& parser, Context& context, const ShellContext&)
{
    return do_write_buffer(context,
                           parser.positional_count() > 0 ? parser[0] : Optional<String>{},
                           (parser.get_switch("sync") ? WriteFlags::Sync : WriteFlags::None) |
                           (parser.get_switch("force") or force ? WriteFlags::Force : WriteFlags::None),
                           parser.get_switch("method").map(parse_write_method));
}

const CommandDesc write_cmd = {
    "write",
    "w",
    "write [<switches>] [<filename>]: write the current buffer to its file "
    "or to <filename> if specified",
    write_params,
    CommandFlags::None,
    CommandHelper{},
    filename_completer<false>,
    write_buffer,
};

const CommandDesc force_write_cmd = {
    "write!",
    "w!",
    "write! [<switches>] [<filename>]: write the current buffer to its file "
    "or to <filename> if specified, even when the file is write protected",
    write_params_except_force,
    CommandFlags::None,
    CommandHelper{},
    filename_completer<false>,
    write_buffer<true>,
};

void write_all_buffers(const Context& context, bool sync = false, Optional<WriteMethod> write_method = {})
{
    // Copy buffer list because hooks might be creating/deleting buffers
    Vector<SafePtr<Buffer>> buffers;
    for (auto& buffer : BufferManager::instance())
        buffers.emplace_back(buffer.get());

    for (auto& buffer : buffers)
    {
        if ((buffer->flags() & Buffer::Flags::File) and
            ((buffer->flags() & Buffer::Flags::New) or
             buffer->is_modified())
            and !(buffer->flags() & Buffer::Flags::ReadOnly))
        {
            auto method = write_method.value_or_compute([&] { return context.options()["writemethod"].get<WriteMethod>(); });
            auto flags = sync ? WriteFlags::Sync : WriteFlags::None;
            buffer->run_hook_in_own_context(Hook::BufWritePre, buffer->name(), context.name());
            BusyIndicator busy_indicator{context, [&](std::chrono::seconds elapsed) {
                return DisplayLine{format("waiting while writing buffer ({}s)", elapsed.count()),
                                   context.faces()["Information"]};
            }};
            write_buffer_to_file(*buffer, buffer->name(), method, flags);
            buffer->run_hook_in_own_context(Hook::BufWritePost, buffer->name(), context.name());
        }
    }
}

const CommandDesc write_all_cmd = {
    "write-all",
    "wa",
    "write-all [<switches>]: write all changed buffers that are associated to a file",
    ParameterDesc{
        write_params_except_force.switches,
        ParameterDesc::Flags::None, 0, 0
    },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&){
        write_all_buffers(context,
                          (bool)parser.get_switch("sync"),
                          parser.get_switch("method").map(parse_write_method));
    }
};

static void ensure_all_buffers_are_saved(Context& context)
{
    auto is_modified = [](const UniquePtr<Buffer>& buf) {
        return (buf->flags() & Buffer::Flags::File) and buf->is_modified();
    };

    auto it = find_if(BufferManager::instance(), is_modified);
    const auto end = BufferManager::instance().end();
    if (it == end)
        return;

    if (not context.buffer().is_modified())
    {
        context.push_jump();
        context.change_buffer(**it);
    }

    String message = format("{} modified buffers remaining: [",
                            std::count_if(it, end, is_modified));
    while (it != end)
    {
        message += (*it)->display_name();
        it = std::find_if(it+1, end, is_modified);
        message += (it != end) ? ", " : "]";
    }
    throw runtime_error(message);
}

template<bool force>
void kill(const ParametersParser& parser, Context& context, const ShellContext&)
{
    auto& client_manager = ClientManager::instance();

    if (not force)
        ensure_all_buffers_are_saved(context);

    const int status = parser.positional_count() > 0 ? str_to_int(parser[0]) : 0;
    while (not client_manager.empty())
        client_manager.remove_client(**client_manager.begin(), true, status);

    throw kill_session{status};
}

const CommandDesc kill_cmd = {
    "kill",
    nullptr,
    "kill [<exit status>]: terminate the current session, the server and all clients connected. "
    "An optional integer parameter can set the server and client processes exit status",
    { {}, ParameterDesc::Flags::SwitchesAsPositional, 0, 1 },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    kill<false>
};


const CommandDesc force_kill_cmd = {
    "kill!",
    nullptr,
    "kill! [<exit status>]: force the termination of the current session, the server and all clients connected. "
    "An optional integer parameter can set the server and client processes exit status",
    { {}, ParameterDesc::Flags::SwitchesAsPositional, 0, 1 },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    kill<true>
};

const CommandDesc daemonize_session_cmd = {
    "daemonize-session",
    nullptr,
    "daemonize-session: set the session server not to quit on last client exit",
    { {} },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context&, const ShellContext&) { Server::instance().daemonize(); }
};

template<bool force>
void quit(const ParametersParser& parser, Context& context, const ShellContext&)
{
    if (not force and ClientManager::instance().count() == 1 and not Server::instance().is_daemon())
        ensure_all_buffers_are_saved(context);

    const int status = parser.positional_count() > 0 ? str_to_int(parser[0]) : 0;
    ClientManager::instance().remove_client(context.client(), true, status);
}

const CommandDesc quit_cmd = {
    "quit",
    "q",
    "quit [<exit status>]: quit current client, and the kakoune session if the client is the last "
    "(if not running in daemon mode). "
    "An optional integer parameter can set the client exit status",
    { {}, ParameterDesc::Flags::SwitchesAsPositional, 0, 1 },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    quit<false>
};

const CommandDesc force_quit_cmd = {
    "quit!",
    "q!",
    "quit! [<exit status>]: quit current client, and the kakoune session if the client is the last "
    "(if not running in daemon mode). Force quit even if the client is the "
    "last and some buffers are not saved. "
    "An optional integer parameter can set the client exit status",
    { {}, ParameterDesc::Flags::SwitchesAsPositional, 0, 1 },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    quit<true>
};

template<bool force>
void write_quit(const ParametersParser& parser, Context& context,
                const ShellContext& shell_context)
{
    do_write_buffer(context, {},
                    parser.get_switch("sync") ? WriteFlags::Sync : WriteFlags::None,
                    parser.get_switch("method").map(parse_write_method));
    quit<force>(parser, context, shell_context);
}

const CommandDesc write_quit_cmd = {
    "write-quit",
    "wq",
    "write-quit [<switches>] [<exit status>]: write current buffer and quit current client. "
    "An optional integer parameter can set the client exit status",
    write_params_except_force,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    write_quit<false>
};

const CommandDesc force_write_quit_cmd = {
    "write-quit!",
    "wq!",
    "write-quit! [<switches>] [<exit status>] write: current buffer and quit current client, even if other buffers are not saved. "
    "An optional integer parameter can set the client exit status",
    write_params_except_force,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    write_quit<true>
};

const CommandDesc write_all_quit_cmd = {
    "write-all-quit",
    "waq",
    "write-all-quit [<switches>] [<exit status>]: write all buffers associated to a file and quit current client. "
    "An optional integer parameter can set the client exit status.",
    write_params_except_force,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext& shell_context)
    {
        write_all_buffers(context,
                          (bool)parser.get_switch("sync"),
                          parser.get_switch("method").map(parse_write_method));
        quit<false>(parser, context, shell_context);
    }
};

const CommandDesc buffer_cmd = {
    "buffer",
    "b",
    "buffer <name>: set buffer to edit in current client",
    {
        { { "matching", { {}, "treat the argument as a regex" } } },
        ParameterDesc::Flags::None, 1, 1
    },
    CommandFlags::None,
    CommandHelper{},
    make_completer(menu(complete_buffer_name<true>)),
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        Buffer& buffer = parser.get_switch("matching") ? BufferManager::instance().get_buffer_matching(
                                                             [re=Regex{parser[0]}](Buffer& buffer) {
                                                                auto name = buffer.name();
                                                                return regex_match(name.begin(), name.end(), re);
                                                             })
                                                       : BufferManager::instance().get_buffer(parser[0]);
        if (&buffer != &context.buffer())
        {
            context.push_jump();
            context.change_buffer(buffer);
        }
    }
};

template<bool next>
void cycle_buffer(const ParametersParser& parser, Context& context, const ShellContext&)
{
    Buffer* oldbuf = &context.buffer();
    auto it = find_if(BufferManager::instance(),
                      [oldbuf](const UniquePtr<Buffer>& lhs)
                      { return lhs.get() == oldbuf; });
    kak_assert(it != BufferManager::instance().end());

    Buffer* newbuf = nullptr;
    auto cycle = [&] {
        if (not next)
        {
            if (it == BufferManager::instance().begin())
                it = BufferManager::instance().end();
            --it;
        }
        else
        {
            if (++it == BufferManager::instance().end())
                it = BufferManager::instance().begin();
        }
        newbuf = it->get();
    };
    cycle();
    while (newbuf != oldbuf and newbuf->flags() & Buffer::Flags::Debug)
        cycle();

    if (newbuf != oldbuf)
    {
        context.push_jump();
        context.change_buffer(*newbuf);
    }
}

const CommandDesc buffer_next_cmd = {
    "buffer-next",
    "bn",
    "buffer-next: move to the next buffer in the list",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    cycle_buffer<true>
};

const CommandDesc buffer_previous_cmd = {
    "buffer-previous",
    "bp",
    "buffer-previous: move to the previous buffer in the list",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    cycle_buffer<false>
};

template<bool force>
void delete_buffer(const ParametersParser& parser, Context& context, const ShellContext&)
{
    BufferManager& manager = BufferManager::instance();
    Buffer& buffer = parser.positional_count() == 0 ? context.buffer() : manager.get_buffer(parser[0]);
    if (not force and (buffer.flags() & Buffer::Flags::File) and buffer.is_modified())
        throw runtime_error(format("buffer '{}' is modified", buffer.display_name()));

    manager.delete_buffer(buffer);
    context.forget_buffer(buffer);
}

const CommandDesc delete_buffer_cmd = {
    "delete-buffer",
    "db",
    "delete-buffer [name]: delete current buffer or the buffer named <name> if given",
    single_optional_param,
    CommandFlags::None,
    CommandHelper{},
    make_completer(menu(complete_buffer_name<false>)),
    delete_buffer<false>
};

const CommandDesc force_delete_buffer_cmd = {
    "delete-buffer!",
    "db!",
    "delete-buffer! [name]: delete current buffer or the buffer named <name> if "
    "given, even if the buffer is unsaved",
    single_optional_param,
    CommandFlags::None,
    CommandHelper{},
    make_completer(menu(complete_buffer_name<false>)),
    delete_buffer<true>
};

const CommandDesc rename_buffer_cmd = {
    "rename-buffer",
    nullptr,
    "rename-buffer <name>: change current buffer name",
    ParameterDesc{
        {
            { "scratch",  { {}, "convert a file buffer to a scratch buffer" } },
            { "file",  { {}, "convert a scratch buffer to a file buffer" } }
        },
        ParameterDesc::Flags::None, 1, 1
    },
    CommandFlags::None,
    CommandHelper{},
    filename_completer<false>,
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        if (parser.get_switch("scratch") and parser.get_switch("file"))
            throw runtime_error("scratch and file are incompatible switches");

        auto& buffer = context.buffer();
        if (parser.get_switch("scratch"))
            buffer.flags() &= ~(Buffer::Flags::File | Buffer::Flags::New);
        if (parser.get_switch("file"))
            buffer.flags() |= Buffer::Flags::File;

        const bool is_file = (buffer.flags() & Buffer::Flags::File);

        if (not buffer.set_name(is_file ? parse_filename(parser[0]) : parser[0]))
            throw runtime_error(format("unable to change buffer name to '{}': a buffer with this name already exists", parser[0]));
    }
};

static constexpr auto highlighter_scopes = { "global/", "buffer/", "window/", "shared/" };

template<bool add>
Completions highlighter_cmd_completer(
    const Context& context, CommandParameters params,
    size_t token_to_complete, ByteCount pos_in_token)
{
    if (token_to_complete == 0)
    {

        StringView path = params[0];
        auto sep_it = find(path, '/');
        if (sep_it == path.end())
           return { 0_byte, pos_in_token, complete(path, pos_in_token, highlighter_scopes),
                    Completions::Flags::Menu };

        StringView scope{path.begin(), sep_it};
        HighlighterGroup* root = nullptr;
        if (scope == "shared")
            root = &SharedHighlighters::instance();
        else if (auto* s = get_scope_ifp(scope, context))
            root = &s->highlighters().group();
        else
            return {};

        auto offset = scope.length() + 1;
        return offset_pos(root->complete_child(StringView{sep_it+1, path.end()}, pos_in_token - offset, add), offset);
    }
    else if (add and token_to_complete == 1)
    {
        StringView name = params[1];
        return { 0_byte, name.length(), complete(name, pos_in_token, HighlighterRegistry::instance() | transform(&HighlighterRegistry::Item::key)),
                 Completions::Flags::Menu };
    }
    else
        return {};
}

static std::pair<HighlighterGroup&, Highlighter&> get_highlighter(const Context& context, StringView path)
{
    if (not path.empty() and path.back() == '/')
        path = path.substr(0_byte, path.length() - 1);

    auto sep_it = find(path, '/');
    if (path.starts_with("buffer="))
    {
        auto& buffer_manager = BufferManager::instance();
        while (sep_it != path.end() and not buffer_manager.get_buffer_ifp({path.begin()+7, sep_it}))
            sep_it = std::find(sep_it+1, path.end(), '/');
    }

    StringView scope{path.begin(), sep_it};
    auto* root = (scope == "shared") ? static_cast<HighlighterGroup*>(&SharedHighlighters::instance())
                                     : static_cast<HighlighterGroup*>(&get_scope(scope, context).highlighters().group());
    if (sep_it != path.end())
        return {*root, root->get_child(StringView{sep_it+1, path.end()})};
    return {*root, *root};
}

static void redraw_relevant_clients(Context& context, HighlighterGroup& root)
{
    for (auto&& client : ClientManager::instance())
    {
        if (&root == &SharedHighlighters::instance() or
            &root == &GlobalScope::instance().highlighters().group() or
            &root == &client->context().buffer().highlighters().group() or
            &root == &client->context().window().highlighters().group())
            client->force_redraw();
    }
}

const CommandDesc arrange_buffers_cmd = {
    "arrange-buffers",
    nullptr,
    "arrange-buffers <buffer>...: reorder the buffers in the buffers list\n"
    "    the named buffers will be moved to the front of the buffer list, in the order given\n"
    "    buffers that do not appear in the parameters will remain at the end of the list, keeping their current order",
    ParameterDesc{{}, ParameterDesc::Flags::None, 1},
    CommandFlags::None,
    CommandHelper{},
    [](const Context& context, CommandParameters params, size_t, ByteCount cursor_pos)
    {
        return menu(complete_buffer_name<false>)(context, params.back(), cursor_pos);
    },
    [](const ParametersParser& parser, Context&, const ShellContext&)
    {
        BufferManager::instance().arrange_buffers(parser.positionals_from(0));
    }
};

const CommandDesc add_highlighter_cmd = {
    "add-highlighter",
    "addhl",
    "add-highlighter [-override] <path>/<name> <type> <type params>...: add a highlighter to the group identified by <path>\n"
    "    <path> is a '/' delimited path or the parent highlighter, starting with either\n"
    "   'global', 'buffer', 'window' or 'shared', if <name> is empty, it will be autogenerated",
    ParameterDesc{
        { { "override", { {}, "replace existing highlighter with same path if it exists" } }, },
        ParameterDesc::Flags::SwitchesOnlyAtStart, 2
    },
    CommandFlags::None,
    [](const Context& context, CommandParameters params) -> String
    {
        if (params.size() > 1)
        {
            HighlighterRegistry& registry = HighlighterRegistry::instance();
            auto it = registry.find(params[1]);
            if (it != registry.end())
            {
                auto docstring = it->value.description->docstring;
                auto desc_params = generate_switches_doc(it->value.description->params.switches);

                if (desc_params.empty())
                    return format("{}:\n{}", params[1], indent(docstring));
                else
                {
                    auto desc_indent = Vector<String>{docstring, "Switches:", indent(desc_params)}
                                           | transform([](auto& s) { return indent(s); });
                    return format("{}:\n{}", params[1], join(desc_indent, "\n"));
                }
            }
        }
        return "";
    },
    highlighter_cmd_completer<true>,
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        HighlighterRegistry& registry = HighlighterRegistry::instance();

        auto begin = parser.begin();
        StringView path = *begin++;
        StringView type = *begin++;
        Vector<String> highlighter_params;
        for (; begin != parser.end(); ++begin)
            highlighter_params.push_back(*begin);

        auto it = registry.find(type);
        if (it == registry.end())
            throw runtime_error(format("no such highlighter type: '{}'", type));

        auto slash = find(path | reverse(), '/');
        if (slash == path.rend())
            throw runtime_error("no parent in path");

        auto auto_name = [](ConstArrayView<String> params) {
            return join(params | transform([](StringView s) { return replace(s, "/", "<slash>"); }), "_");
        };

        String name{slash.base(), path.end()};
        auto [root, parent] = get_highlighter(context, {path.begin(), slash.base() - 1});
        parent.add_child(name.empty() ? auto_name(parser.positionals_from(1)) : std::move(name),
                         it->value.factory(highlighter_params, &parent), (bool)parser.get_switch("override"));

        redraw_relevant_clients(context, root);
    }
};

const CommandDesc remove_highlighter_cmd = {
    "remove-highlighter",
    "rmhl",
    "remove-highlighter <path>: remove highlighter identified by <path>",
    single_param,
    CommandFlags::None,
    CommandHelper{},
    highlighter_cmd_completer<false>,
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        StringView path = parser[0];
        if (not path.empty() and path.back() == '/') // ignore trailing /
            path = path.substr(0_byte, path.length() - 1_byte);

        auto rev_path = path | reverse();
        auto sep_it = find(rev_path, '/');
        if (sep_it == rev_path.end())
            return;
        auto [root, parent] = get_highlighter(context, {path.begin(), sep_it.base()});
        parent.remove_child({sep_it.base(), path.end()});
        redraw_relevant_clients(context, root);
    }
};

static Completions complete_hooks(const Context&, StringView prefix, ByteCount cursor_pos)
{
    return { 0_byte, cursor_pos, complete(prefix, cursor_pos, enum_desc(Meta::Type<Hook>{}) | transform(&EnumDesc<Hook>::name)) };
}

const CommandDesc add_hook_cmd = {
    "hook",
    nullptr,
    "hook [<switches>] <scope> <hook_name> <filter> <command>: add <command> in <scope> "
    "to be executed on hook <hook_name> when its parameter matches the <filter> regex\n"
    "<scope> can be:\n"
    "  * global: hook is executed for any buffer or window\n"
    "  * buffer: hook is executed only for the current buffer\n"
    "            (and any window for that buffer)\n"
    "  * window: hook is executed only for the current window\n",
    ParameterDesc{
        { { "group", { ArgCompleter{}, "set hook group, see remove-hooks" } },
          { "always", { {}, "run hook even if hooks are disabled" } },
          { "once", { {}, "run the hook only once" } } },
        ParameterDesc::Flags::None, 4, 4
    },
    CommandFlags::None,
    CommandHelper{},
    make_completer(menu(complete_scope), menu(complete_hooks), complete_nothing, CommandManager::Completer{}),
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        auto descs = enum_desc(Meta::Type<Hook>{});
        auto it = find_if(descs, [&](const EnumDesc<Hook>& desc) { return desc.name == parser[1]; });
        if (it == descs.end())
            throw runtime_error{format("no such hook: '{}'", parser[1])};

        Regex regex{parser[2], RegexCompileFlags::Optimize};
        const String& command = parser[3];
        auto group = parser.get_switch("group").value_or(StringView{});

        if (any_of(group, [](char c) { return not is_word(c, { '-' }); }) or
            (not group.empty() and not is_word(group[0])))
            throw runtime_error{format("invalid group name '{}'", group)};

        const auto flags = (parser.get_switch("always") ? HookFlags::Always : HookFlags::None) |
                           (parser.get_switch("once")   ? HookFlags::Once   : HookFlags::None);
        get_scope(parser[0], context).hooks().add_hook(it->value, group.str(), flags,
                                                       std::move(regex), command, context);
    }
};

const CommandDesc remove_hook_cmd = {
    "remove-hooks",
    "rmhooks",
    "remove-hooks <scope> <group>: remove all hooks whose group matches the regex <group>",
    double_params,
    CommandFlags::None,
    CommandHelper{},
    [](const Context& context,
       CommandParameters params, size_t token_to_complete,
       ByteCount pos_in_token) -> Completions
    {
        if (token_to_complete == 0)
            return menu(complete_scope)(context, params[0], pos_in_token);
        else if (token_to_complete == 1)
        {
            if (auto scope = get_scope_ifp(params[0], context))
                return { 0_byte, params[0].length(),
                         scope->hooks().complete_hook_group(params[1], pos_in_token),
                         Completions::Flags::Menu };
        }
        return {};
    },
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        get_scope(parser[0], context).hooks().remove_hooks(Regex{parser[1]});
    }
};

const CommandDesc trigger_user_hook_cmd = {
    "trigger-user-hook",
    nullptr,
    "trigger-user-hook <param>: run 'User' hook with <param> as filter string",
    single_param,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        context.hooks().run_hook(Hook::User, parser[0], context);
    }
};

Vector<String> params_to_shell(const ParametersParser& parser)
{
    Vector<String> vars;
    for (size_t i = 0; i < parser.positional_count(); ++i)
        vars.push_back(parser[i]);
    return vars;
}

Completions complete_completer_type(const Context&, StringView prefix, ByteCount cursor_pos)
{
   static constexpr StringView completers[] = {"file", "client", "buffer", "shell-script", "shell-script-candidates", "command", "shell"};
   return { 0_byte, cursor_pos, complete(prefix, cursor_pos, completers) };
}


CommandCompleter make_command_completer(StringView type, StringView param, Completions::Flags completions_flags)
{
    if (type == "file")
    {
        return [=](const Context& context, CommandParameters params,
                   size_t token_to_complete, ByteCount pos_in_token) {
             const String& prefix = params[token_to_complete];
             const auto& ignored_files = context.options()["ignored_files"].get<Regex>();
             return Completions{0_byte, pos_in_token,
                                complete_filename(prefix, ignored_files,
                                                  pos_in_token, FilenameFlags::Expand),
                                completions_flags};
        };
    }
    else if (type == "client")
    {
        return [=](const Context& context, CommandParameters params,
                   size_t token_to_complete, ByteCount pos_in_token)
        {
             const String& prefix = params[token_to_complete];
             auto& cm = ClientManager::instance();
             return Completions{0_byte, pos_in_token,
                                cm.complete_client_name(prefix, pos_in_token),
                                completions_flags};
        };
    }
    else if (type == "buffer")
    {
        return [=](const Context& context, CommandParameters params,
                   size_t token_to_complete, ByteCount pos_in_token)
        {
             return add_flags(complete_buffer_name<false>, completions_flags)(
                 context, params[token_to_complete], pos_in_token);
        };
    }
    else if (type == "shell-script")
    {
        if (param.empty())
            throw runtime_error("shell-script requires a shell script parameter");

        return ShellScriptCompleter{param.str(), completions_flags};
    }
    else if (type == "shell-script-candidates")
    {
        if (param.empty())
            throw runtime_error("shell-script-candidates requires a shell script parameter");

        return ShellCandidatesCompleter{param.str(), completions_flags};
    }
    else if (type == "command")
        return CommandManager::NestedCompleter{};
    else if (type == "shell")
    {
        return [=](const Context& context, CommandParameters params,
                   size_t token_to_complete, ByteCount pos_in_token)
        {
            return add_flags(shell_complete, completions_flags)(
                context, params[token_to_complete], pos_in_token);
        };
    }
    else
        throw runtime_error(format("invalid command completion type '{}'", type));
}

static CommandCompleter parse_completion_switch(const ParametersParser& parser, Completions::Flags completions_flags) {
    for (StringView completion_switch : {"file-completion", "client-completion", "buffer-completion",
                                         "shell-script-completion", "shell-script-candidates",
                                         "command-completion", "shell-completion"})
    {
        if (auto param = parser.get_switch(completion_switch))
        {
            constexpr StringView suffix = "-completion";
            if (completion_switch.ends_with(suffix))
                completion_switch = completion_switch.substr(0, completion_switch.length() - suffix.length());
            return make_command_completer(completion_switch, *param, completions_flags);
        }
    }
    return {};
}

void define_command(const ParametersParser& parser, Context& context, const ShellContext&)
{
    const String& cmd_name = parser[0];
    auto& cm = CommandManager::instance();

    if (not all_of(cmd_name, is_identifier))
        throw runtime_error(format("invalid command name: '{}'", cmd_name));

    if (cm.command_defined(cmd_name) and not parser.get_switch("override"))
        throw runtime_error(format("command '{}' already defined", cmd_name));

    CommandFlags flags = CommandFlags::None;
    if (parser.get_switch("hidden"))
        flags = CommandFlags::Hidden;

    const bool menu = (bool)parser.get_switch("menu");
    const Completions::Flags completions_flags = menu ?
        Completions::Flags::Menu : Completions::Flags::None;

    const String& commands = parser[1];
    CommandFunc cmd;
    ParameterDesc desc;
    if (auto params = parser.get_switch("params"))
    {
        size_t min = 0, max = -1;
        StringView counts = *params;
        static const Regex re{R"((\d+)?..(\d+)?)"};
        MatchResults<const char*> res;
        if (regex_match(counts.begin(), counts.end(), res, re))
        {
            if (res[1].matched)
                min = (size_t)str_to_int({res[1].first, res[1].second});
            if (res[2].matched)
                max = (size_t)str_to_int({res[2].first, res[2].second});
        }
        else
            min = max = (size_t)str_to_int(counts);

        desc = ParameterDesc{ {}, ParameterDesc::Flags::SwitchesAsPositional, min, max };
        cmd = [=](const ParametersParser& parser, Context& context, const ShellContext& sc) {
            LocalScope local_scope{context};
            CommandManager::instance().execute(commands, context,
                                               { params_to_shell(parser), sc.env_vars });
        };
    }
    else
    {
        desc = ParameterDesc{ {}, ParameterDesc::Flags::SwitchesAsPositional, 0, 0 };
        cmd = [=](const ParametersParser& parser, Context& context, const ShellContext& sc) {
            LocalScope local_scope{context};
            CommandManager::instance().execute(commands, context, { {}, sc.env_vars });
        };
    }

    CommandCompleter completer = parse_completion_switch(parser, completions_flags);
    if (menu and not completer)
        throw runtime_error("menu switch requires a completion switch");
    auto docstring = trim_indent(parser.get_switch("docstring").value_or(StringView{}));

    cm.register_command(cmd_name, cmd, docstring, desc, flags, CommandHelper{}, std::move(completer));
}

const CommandDesc define_command_cmd = {
    "define-command",
    "def",
    "define-command [<switches>] <name> <cmds>: define a command <name> executing <cmds>",
    ParameterDesc{
        { { "params",                   { ArgCompleter{},  "take parameters, accessible to each shell escape as $0..$N\n"
                                                 "parameter should take the form <count> or <min>..<max> (both omittable)" } },
          { "override",                 { {}, "allow overriding an existing command" } },
          { "hidden",                   { {}, "do not display the command in completion candidates" } },
          { "docstring",                { ArgCompleter{},  "define the documentation string for command" } },
          { "menu",                     { {}, "treat completions as the only valid inputs" } },
          { "file-completion",          { {}, "complete parameters using filename completion" } },
          { "client-completion",        { {}, "complete parameters using client name completion" } },
          { "buffer-completion",        { {}, "complete parameters using buffer name completion" } },
          { "command-completion",       { {}, "complete parameters using kakoune command completion" } },
          { "shell-completion",         { {}, "complete parameters using shell command completion" } },
          { "shell-script-completion",  { ArgCompleter{},  "complete parameters using the given shell-script" } },
          { "shell-script-candidates",  { ArgCompleter{},  "get the parameter candidates using the given shell-script" } } },
        ParameterDesc::Flags::None,
        2, 2
    },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    define_command
};

static Completions complete_alias_name(const Context& context, StringView prefix, ByteCount cursor_pos)
{
   return { 0_byte, cursor_pos, complete(prefix, cursor_pos,
                                           context.aliases().flatten_aliases()
                                         | transform(&HashItem<String, String>::key))};
}

const CommandDesc alias_cmd = {
    "alias",
    nullptr,
    "alias <scope> <alias> <command>: alias <alias> to <command> in <scope>",
    ParameterDesc{{}, ParameterDesc::Flags::None, 3, 3},
    CommandFlags::None,
    CommandHelper{},
    make_completer(menu(complete_scope), complete_alias_name, complete_command_name),
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        if (not CommandManager::instance().command_defined(parser[2]))
            throw runtime_error(format("no such command: '{}'", parser[2]));

        AliasRegistry& aliases = get_scope(parser[0], context).aliases();
        aliases.add_alias(parser[1], parser[2]);
    }
};

const CommandDesc unalias_cmd = {
    "unalias",
    nullptr,
    "unalias <scope> <alias> [<expected>]: remove <alias> from <scope>\n"
    "If <expected> is specified, remove <alias> only if its value is <expected>",
    ParameterDesc{{}, ParameterDesc::Flags::None, 2, 3},
    CommandFlags::None,
    CommandHelper{},
    make_completer(menu(complete_scope), complete_alias_name, complete_command_name),
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        AliasRegistry& aliases = get_scope(parser[0], context).aliases();
        if (parser.positional_count() == 3 and
            aliases[parser[1]] != parser[2])
            return;
        aliases.remove_alias(parser[1]);
    }
};

const CommandDesc complete_command_cmd = {
    "complete-command",
    "compl",
    "complete-command [<switches>] <name> <type> [<param>]\n"
    "define command completion",
    ParameterDesc{
        { { "menu",                     { {}, "treat completions as the only valid inputs" } }, },
        ParameterDesc::Flags::None, 2, 3},
    CommandFlags::None,
    CommandHelper{},
    make_completer(complete_command_name, complete_completer_type),
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        const Completions::Flags flags = parser.get_switch("menu") ? Completions::Flags::Menu : Completions::Flags::None;
        CommandCompleter completer = make_command_completer(parser[1], parser.positional_count() >= 3 ? parser[2] : StringView{}, flags);
        CommandManager::instance().set_command_completer(parser[0], std::move(completer));
    }
};

const CommandDesc echo_cmd = {
    "echo",
    nullptr,
    "echo <params>...: display given parameters in the status line",
    ParameterDesc{
        { { "markup", { {}, "parse markup" } },
          { "quoting", { {arg_completer(Array{"raw", "kakoune", "shell"})}, "quote each argument separately using the given style (raw|kakoune|shell)" } },
          { "end-of-line", { {}, "add trailing end-of-line" } },
          { "to-file", { {filename_arg_completer<false>}, "echo contents to given filename" } },
          { "to-shell-script", { ArgCompleter{}, "pipe contents to given shell script" } },
          { "debug", { {}, "write to debug buffer instead of status line" } } },
        ParameterDesc::Flags::SwitchesOnlyAtStart
    },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext& shell_context)
    {
        String message;
        if (auto quoting = parser.get_switch("quoting"))
            message = join(parser | transform(quoter(option_from_string(Meta::Type<Quoting>{}, *quoting))),
                           ' ', false);
        else
            message = join(parser, ' ', false);

        if (parser.get_switch("end-of-line"))
            message.push_back('\n');

        if (auto filename = parser.get_switch("to-file"))
        {
            BusyIndicator busy_indicator{context, [&](std::chrono::seconds elapsed) {
                return DisplayLine{format("waiting while writing to '{}' ({}s)", *filename, elapsed.count()),
                                   context.faces()["Information"]};
            }};
            write_to_file(*filename, message);
        }
        else if (auto command = parser.get_switch("to-shell-script"))
            ShellManager::instance().eval(*command, context, message, ShellManager::Flags::None, shell_context);
        else if (parser.get_switch("debug"))
            write_to_debug_buffer(message);
        else if (parser.get_switch("markup"))
            context.print_status(parse_display_line(message, context.faces()));
        else
            context.print_status({message, context.faces()["StatusLine"]});
    }
};

KeymapMode parse_keymap_mode(StringView str, const KeymapManager::UserModeList& user_modes)
{
    if (prefix_match("normal", str)) return KeymapMode::Normal;
    if (prefix_match("insert", str)) return KeymapMode::Insert;
    if (prefix_match("menu", str))   return KeymapMode::Menu;
    if (prefix_match("prompt", str)) return KeymapMode::Prompt;
    if (prefix_match("goto", str))   return KeymapMode::Goto;
    if (prefix_match("view", str))   return KeymapMode::View;
    if (prefix_match("user", str))   return KeymapMode::User;
    if (prefix_match("object", str)) return KeymapMode::Object;

    auto it = find(user_modes, str);
    if (it == user_modes.end())
        throw runtime_error(format("no such keymap mode: '{}'", str));

    char offset = static_cast<char>(KeymapMode::FirstUserMode);
    return (KeymapMode)(std::distance(user_modes.begin(), it) + offset);
}

static constexpr auto modes = make_array<StringView>({ "normal", "insert", "menu", "prompt", "goto", "view", "user", "object" });

const CommandDesc debug_cmd = {
    "debug",
    nullptr,
    "debug <command>: write some debug information to the *debug* buffer",
    ParameterDesc{{}, ParameterDesc::Flags::SwitchesOnlyAtStart, 1},
    CommandFlags::None,
    CommandHelper{},
    make_completer(
        [](const Context& context, StringView prefix, ByteCount cursor_pos) -> Completions {
               auto c = {"info", "buffers", "options", "memory", "shared-strings",
                         "profile-hash-maps", "faces", "mappings", "regex", "registers"};
               return { 0_byte, cursor_pos, complete(prefix, cursor_pos, c), Completions::Flags::Menu };
    }),
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        if (parser[0] == "info")
        {
            write_to_debug_buffer(format("version: {}", version));
            write_to_debug_buffer(format("pid: {}", getpid()));
            write_to_debug_buffer(format("session: {}", Server::instance().session()));
            #ifdef KAK_DEBUG
            write_to_debug_buffer("build: debug");
            #else
            write_to_debug_buffer("build: release");
            #endif
        }
        else if (parser[0] == "buffers")
        {
            write_to_debug_buffer("Buffers:");
            for (auto& buffer : BufferManager::instance())
                write_to_debug_buffer(buffer->debug_description());
        }
        else if (parser[0] == "options")
        {
            write_to_debug_buffer("Options:");
            for (auto& option : context.options().flatten_options())
                write_to_debug_buffer(format(" * {} \"{}\"{}: {}", option->name(), option->docstring(),
                                             option->flags() & OptionFlags::Hidden ? " (hidden)" : "",
                                             option->get_as_string(Quoting::Kakoune)));
        }
        else if (parser[0] == "memory")
        {
            size_t total = 0;
            write_to_debug_buffer("Memory usage:");
            const ColumnCount column_size = 17;
            write_to_debug_buffer(format("{:17} │{:17} │{:17} │{:17} ",
                                         "domain",
                                         "bytes",
                                         "active allocs",
                                         "total allocs"));
            write_to_debug_buffer(format("{0}┼{0}┼{0}┼{0}", String(Codepoint{0x2500}, column_size + 1)));

            for (int domain = 0; domain < (int)MemoryDomain::Count; ++domain)
            {
                auto& stats = memory_stats[domain];
                total += stats.allocated_bytes;
                write_to_debug_buffer(format("{:17} │{:17} │{:17} │{:17} ",
                                             domain_name((MemoryDomain)domain),
                                             grouped(stats.allocated_bytes),
                                             grouped(stats.allocation_count),
                                             grouped(stats.total_allocation_count)));
            }
            write_to_debug_buffer({});
            write_to_debug_buffer(format("  Total: {}", grouped(total)));
            #if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 33))
            write_to_debug_buffer(format("  Malloced: {}", grouped(mallinfo2().uordblks)));
            #elif defined(__GLIBC__) || defined(__CYGWIN__)
            write_to_debug_buffer(format("  Malloced: {}", grouped(mallinfo().uordblks)));
            #endif
        }
        else if (parser[0] == "shared-strings")
        {
            StringRegistry::instance().debug_stats();
        }
        else if (parser[0] == "profile-hash-maps")
        {
            profile_hash_maps();
        }
        else if (parser[0] == "faces")
        {
            write_to_debug_buffer("Faces:");
            for (auto& face : context.faces().flatten_faces())
                write_to_debug_buffer(format(" * {}: {}", face.key, face.value.face));
        }
        else if (parser[0] == "mappings")
        {
            auto& keymaps = context.keymaps();
            auto user_modes = keymaps.user_modes();
            write_to_debug_buffer("Mappings:");
            for (auto mode : concatenated(modes, user_modes))
            {
                KeymapMode m = parse_keymap_mode(mode, user_modes);
                for (auto& key : keymaps.get_mapped_keys(m)) {
                    KeyList kl = keymaps.get_mapping_keys(key, m);
                    String mapping;
                    for (const auto& k : kl)
                        mapping += to_string(k);
                    write_to_debug_buffer(format(" * {} {}: '{}' {}",
                                          mode, key, mapping, keymaps.get_mapping_docstring(key, m)));
                }
            }
        }
        else if (parser[0] == "regex")
        {
            if (parser.positional_count() != 2)
                throw runtime_error("expected a regex");

            write_to_debug_buffer(format(" * {}:\n{}",
                                  parser[1], dump_regex(compile_regex(parser[1], RegexCompileFlags::Optimize))));
        }
        else if (parser[0] == "registers")
        {
            write_to_debug_buffer("Register info:");
            for (auto&& [name, reg] : RegisterManager::instance())
            {
                auto content = reg->get(context);

                if (content.size() == 1 and content[0] == "")
                    continue;

                write_to_debug_buffer(format(" * {} = {}\n", name,
                    join(content | transform(quote), "\n     = ")));
            }
        }
        else
            throw runtime_error(format("no such debug command: '{}'", parser[0]));
    }
};

const CommandDesc source_cmd = {
    "source",
    nullptr,
    "source <filename> <params>...: execute commands contained in <filename>\n"
    "parameters are available in the sourced script as %arg{0}, %arg{1}, …",
    ParameterDesc{ {}, ParameterDesc::Flags::None, 1, (size_t)-1 },
    CommandFlags::None,
    CommandHelper{},
    filename_completer<true>,
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        ProfileScope profile{context.options()["debug"].get<DebugFlags>(), [&](std::chrono::microseconds duration) {
            write_to_debug_buffer(format("sourcing '{}' took {} us", parser[0], (size_t)duration.count()));
        }};

        String path = real_path(parse_filename(parser[0]));
        MappedFile file_content{path};
        try
        {
            auto params = parser | skip(1) | gather<Vector<String>>();
            CommandManager::instance().execute(file_content, context,
                                               {params, {{"source", path}}});
        }
        catch (Kakoune::runtime_error& err)
        {
            write_to_debug_buffer(format("{}:{}", parser[0], err.what()));
            throw;
        }
    }
};

static String option_doc_helper(const Context& context, CommandParameters params)
{
    const bool is_switch = params.size() > 1 and (params[0] == "-add" or params[0] == "-remove");
    if (params.size() < 2 + (is_switch ? 1 : 0))
        return "";

    auto desc = GlobalScope::instance().option_registry().option_desc(params[1 + (is_switch ? 1 : 0)]);
    if (not desc or desc->docstring().empty())
        return "";

    return format("{}:\n{}", desc->name(), indent(desc->docstring()));
}

static OptionManager& get_options(StringView scope, const Context& context, StringView option_name)
{
    if (scope == "current")
        return context.options()[option_name].manager();
    return get_scope(scope, context).options();
}

const CommandDesc set_option_cmd = {
    "set-option",
    "set",
    "set-option [<switches>] <scope> <name> <value>: set option <name> in <scope> to <value>\n"
    "<scope> can be global, buffer, window, or current which refers to the narrowest "
    "scope the option is set in",
    ParameterDesc{
        { { "add",    { {}, "add to option rather than replacing it" } },
          { "remove", { {}, "remove from option rather than replacing it" } } },
        ParameterDesc::Flags::SwitchesOnlyAtStart, 2, (size_t)-1
    },
    CommandFlags::None,
    option_doc_helper,
    [](const Context& context, CommandParameters params,
       size_t token_to_complete, ByteCount pos_in_token) -> Completions
    {
        if (token_to_complete == 0)
            return menu(complete_scope_including_current)(context, params[0], pos_in_token);
        else if (token_to_complete == 1)
            return { 0_byte, params[1].length(),
                     GlobalScope::instance().option_registry().complete_option_name(params[1], pos_in_token),
                     Completions::Flags::Menu };
        else if (token_to_complete == 2  and params[2].empty() and
                 GlobalScope::instance().option_registry().option_exists(params[1]))
        {
            OptionManager& options = get_scope(params[0], context).options();
            return {0_byte, params[2].length(),
                    {options[params[1]].get_as_string(Quoting::Kakoune)},
                    Completions::Flags::Quoted};
        }
        return Completions{};
    },
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        bool add = (bool)parser.get_switch("add");
        bool remove = (bool)parser.get_switch("remove");
        if (add and remove)
            throw runtime_error("cannot add and remove at the same time");

        Option& opt = get_options(parser[0], context, parser[1]).get_local_option(parser[1]);
        if (add)
            opt.add_from_strings(parser.positionals_from(2));
        else if (remove)
            opt.remove_from_strings(parser.positionals_from(2));
        else
            opt.set_from_strings(parser.positionals_from(2));
    }
};

Completions complete_option(const Context& context, CommandParameters params,
                            size_t token_to_complete, ByteCount pos_in_token)
{
    if (token_to_complete == 0)
        return menu(complete_scope_no_global)(context, params[0], pos_in_token);
    else if (token_to_complete == 1)
        return { 0_byte, params[1].length(),
                 GlobalScope::instance().option_registry().complete_option_name(params[1], pos_in_token),
                 Completions::Flags::Menu };
    return Completions{};
}

const CommandDesc unset_option_cmd = {
    "unset-option",
    "unset",
    "unset-option <scope> <name>: remove <name> option from scope, falling back on parent scope value\n"
    "<scope> can be buffer, window, or current which refers to the narrowest "
    "scope the option is set in",
    double_params,
    CommandFlags::None,
    option_doc_helper,
    complete_option,
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        auto& options = get_options(parser[0], context, parser[1]);
        if (&options == &GlobalScope::instance().options())
            throw runtime_error("cannot unset options in global scope");
        options.unset_option(parser[1]);
    }
};

const CommandDesc update_option_cmd = {
    "update-option",
    nullptr,
    "update-option <scope> <name>: update <name> option from scope\n"
    "some option types, such as line-specs or range-specs can be updated to latest buffer timestamp\n"
    "<scope> can be buffer, window, or current which refers to the narrowest "
    "scope the option is set in",
    double_params,
    CommandFlags::None,
    option_doc_helper,
    complete_option,
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        Option& opt = get_options(parser[0], context, parser[1]).get_local_option(parser[1]);
        opt.update(context);
    }
};

const CommandDesc declare_option_cmd = {
    "declare-option",
    "decl",
    "declare-option [<switches>] <type> <name> [value]: declare option <name> of type <type>.\n"
    "set its initial value to <value> if given and the option did not exist\n"
    "Available types:\n"
    "    int: integer\n"
    "    bool: boolean (true/false or yes/no)\n"
    "    str: character string\n"
    "    regex: regular expression\n"
    "    int-list: list of integers\n"
    "    str-list: list of character strings\n"
    "    completions: list of completion candidates\n"
    "    line-specs: list of line specs\n"
    "    range-specs: list of range specs\n"
    "    str-to-str-map: map from strings to strings\n",
    ParameterDesc{
        { { "hidden",    { {}, "do not display option name when completing" } },
          { "docstring", { ArgCompleter{},  "specify option description" } } },
        ParameterDesc::Flags::SwitchesOnlyAtStart, 2, (size_t)-1
    },
    CommandFlags::None,
    CommandHelper{},
    make_completer(
        [](const Context& context,
           StringView prefix, ByteCount cursor_pos) -> Completions {
               auto c = {"int", "bool", "str", "regex", "int-list", "str-list", "completions", "line-specs", "range-specs", "str-to-str-map"};
               return { 0_byte, cursor_pos, complete(prefix, cursor_pos, c), Completions::Flags::Menu };
    }),
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        Option* opt = nullptr;

        OptionFlags flags = OptionFlags::None;
        if (parser.get_switch("hidden"))
            flags = OptionFlags::Hidden;

        auto docstring = trim_indent(parser.get_switch("docstring").value_or(StringView{}));
        OptionsRegistry& reg = GlobalScope::instance().option_registry();


        if (parser[0] == "int")
            opt = &reg.declare_option<int>(parser[1], docstring, 0, flags);
        else if (parser[0] == "bool")
            opt = &reg.declare_option<bool>(parser[1], docstring, false, flags);
        else if (parser[0] == "str")
            opt = &reg.declare_option<String>(parser[1], docstring, "", flags);
        else if (parser[0] == "regex")
            opt = &reg.declare_option<Regex>(parser[1], docstring, Regex{}, flags);
        else if (parser[0] == "int-list")
            opt = &reg.declare_option<Vector<int, MemoryDomain::Options>>(parser[1], docstring, {}, flags);
        else if (parser[0] == "str-list")
            opt = &reg.declare_option<Vector<String, MemoryDomain::Options>>(parser[1], docstring, {}, flags);
        else if (parser[0] == "completions")
            opt = &reg.declare_option<CompletionList>(parser[1], docstring, {}, flags);
        else if (parser[0] == "line-specs")
            opt = &reg.declare_option<TimestampedList<LineAndSpec>>(parser[1], docstring, {}, flags);
        else if (parser[0] == "range-specs")
            opt = &reg.declare_option<TimestampedList<RangeAndString>>(parser[1], docstring, {}, flags);
        else if (parser[0] == "str-to-str-map")
            opt = &reg.declare_option<HashMap<String, String, MemoryDomain::Options>>(parser[1], docstring, {}, flags);
        else
            throw runtime_error(format("no such option type: '{}'", parser[0]));

        if (parser.positional_count() > 2)
            opt->set_from_strings(parser.positionals_from(2));
    }
};

template<bool unmap>
static Completions map_key_completer(const Context& context, CommandParameters params,
                                     size_t token_to_complete, ByteCount pos_in_token)
{
    if (token_to_complete == 0)
        return menu(complete_scope)(context, params[0], pos_in_token);
    if (token_to_complete == 1)
    {
        auto& user_modes = get_scope(params[0], context).keymaps().user_modes();
        return { 0_byte, params[1].length(),
                 complete(params[1], pos_in_token, concatenated(modes, user_modes)),
                 Completions::Flags::Menu };
    }
    if (unmap and token_to_complete == 2)
    {
        KeymapManager& keymaps = get_scope(params[0], context).keymaps();
        KeymapMode keymap_mode = parse_keymap_mode(params[1], keymaps.user_modes());
        KeyList keys = keymaps.get_mapped_keys(keymap_mode);

        return { 0_byte, params[2].length(),
                 complete(params[2], pos_in_token,
                          keys | transform([](Key k) { return to_string(k); })
                               | gather<Vector<String>>()),
                 Completions::Flags::Menu };
    }
    return {};
}

const CommandDesc map_key_cmd = {
    "map",
    nullptr,
    "map [<switches>] <scope> <mode> <key> <keys>: map <key> to <keys> in given <mode> in <scope>",
    ParameterDesc{
        { { "docstring", { ArgCompleter{},  "specify mapping description" } } },
        ParameterDesc::Flags::None, 4, 4
    },
    CommandFlags::None,
    CommandHelper{},
    map_key_completer<false>,
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        KeymapManager& keymaps = get_scope(parser[0], context).keymaps();
        KeymapMode keymap_mode = parse_keymap_mode(parser[1], keymaps.user_modes());

        KeyList key = parse_keys(parser[2]);
        if (key.size() != 1)
            throw runtime_error("only a single key can be mapped");

        KeyList mapping = parse_keys(parser[3]);
        keymaps.map_key(key[0], keymap_mode, std::move(mapping),
                        trim_indent(parser.get_switch("docstring").value_or("")));
    }
};

const CommandDesc unmap_key_cmd = {
    "unmap",
    nullptr,
    "unmap <scope> <mode> [<key> [<expected-keys>]]: unmap <key> from given <mode> in <scope>.\n"
    "If <expected-keys> is specified, remove the mapping only if its value is <expected-keys>.\n"
    "If only <scope> and <mode> are specified remove all mappings",
    ParameterDesc{{}, ParameterDesc::Flags::None, 2, 4},
    CommandFlags::None,
    CommandHelper{},
    map_key_completer<true>,
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        KeymapManager& keymaps = get_scope(parser[0], context).keymaps();
        KeymapMode keymap_mode = parse_keymap_mode(parser[1], keymaps.user_modes());

        if (parser.positional_count() == 2)
        {
            keymaps.unmap_keys(keymap_mode);
            return;
        }

        KeyList key = parse_keys(parser[2]);
        if (key.size() != 1)
            throw runtime_error("only a single key can be unmapped");

        if (keymaps.is_mapped(key[0], keymap_mode) and
            (parser.positional_count() < 4 or
             keymaps.get_mapping_keys(key[0], keymap_mode) == parse_keys(parser[3])))
            keymaps.unmap_key(key[0], keymap_mode);
    }
};

template<size_t... P>
ParameterDesc make_context_wrap_params_impl(Array<HashItem<String, SwitchDesc>, sizeof...(P)>&& additional_params,
                                            std::index_sequence<P...>)
{
    return { { { "client",     { {client_arg_completer},  "run in the client context for each client in the given comma separatd list" } },
               { "try-client", { {client_arg_completer},  "run in given client context if it exists, or else in the current one" } },
               { "buffer",     { {complete_buffer_name<false>},  "run in a disposable context for each given buffer in the comma separated list argument" } },
               { "draft",      { {}, "run in a disposable context" } },
               { "itersel",    { {}, "run once for each selection with that selection as the only one" } },
               std::move(additional_params[P])...},
        ParameterDesc::Flags::SwitchesOnlyAtStart, 1
    };
}

template<size_t N>
ParameterDesc make_context_wrap_params(Array<HashItem<String, SwitchDesc>, N>&& additional_params)
{
    return make_context_wrap_params_impl(std::move(additional_params), std::make_index_sequence<N>());
}

template<typename Func>
void context_wrap(const ParametersParser& parser, Context& context, StringView default_saved_regs, Func func)
{
    if ((int)(bool)parser.get_switch("buffer") +
        (int)(bool)parser.get_switch("client") +
        (int)(bool)parser.get_switch("try-client") > 1)
        throw runtime_error{"only one of -buffer, -client or -try-client can be specified"};

    const auto& register_manager = RegisterManager::instance();
    auto make_register_restorer = [&](char c) {
        auto& reg = register_manager[c];
        return OnScopeEnd([&, c, save=reg.save(context), d=ScopedSetBool{reg.modified_hook_disabled()}] {
            try
            {
                reg.restore(context, save);
            }
            catch (runtime_error& err)
            {
                write_to_debug_buffer(format("failed to restore register '{}': {}", c, err.what()));
            }
        });
    };
    Vector<decltype(make_register_restorer(0))> saved_registers;
    for (auto c : parser.get_switch("save-regs").value_or(default_saved_regs))
        saved_registers.push_back(make_register_restorer(c));

    if (auto bufnames = parser.get_switch("buffer"))
    {
        auto context_wrap_for_buffer = [&](Buffer& buffer) {
            InputHandler input_handler{{buffer, Selection{}}, Context::Flags::Draft};
            func(parser, input_handler.context());
        };
        if (*bufnames == "*")
        {
            for (auto&& buffer : BufferManager::instance()
                               | transform(&UniquePtr<Buffer>::get)
                               | filter([](Buffer* buf) { return not (buf->flags() & Buffer::Flags::Debug); })
                               | gather<Vector<SafePtr<Buffer>>>()) // gather as we might be mutating the buffer list in the loop.
                context_wrap_for_buffer(*buffer);
        }
        else
            for (auto&& name : *bufnames
                             | split<StringView>(',', '\\')
                             | transform(unescape<',', '\\'>))
                context_wrap_for_buffer(BufferManager::instance().get_buffer(name));
        return;
    }

    auto context_wrap_for_context = [&parser, &func](Context& base_context) {
        Optional<InputHandler> input_handler;

        const bool draft = (bool)parser.get_switch("draft");
        if (draft)
        {
            input_handler.emplace(base_context.selections(),
                                  Context::Flags::Draft,
                                  base_context.name());

            // Preserve window so that window scope is available
            if (base_context.has_window())
                input_handler->context().set_window(base_context.window());

            // We do not want this draft context to commit undo groups if the real one is
            // going to commit the whole thing later
            if (base_context.is_editing())
                input_handler->context().disable_undo_handling();
        }

        Context& c = input_handler ? input_handler->context() : base_context;

        ScopedEdition edition{c};
        ScopedSelectionEdition selection_edition{c};

        if (parser.get_switch("itersel"))
        {
            SelectionList sels{base_context.selections()};
            Vector<Selection> new_sels;
            size_t main = 0;
            size_t timestamp = c.buffer().timestamp();
            bool one_selection_succeeded = false;
            for (auto& sel : sels)
            {
                c.selections_write_only() = SelectionList{sels.buffer(), sel, sels.timestamp()};
                c.selections().update();

                try
                {
                    func(parser, c);
                    one_selection_succeeded = true;

                    if (&sels.buffer() != &c.buffer())
                        throw runtime_error("buffer has changed while iterating on selections");

                    if (not draft)
                    {
                        update_selections(new_sels, main, c.buffer(), timestamp);
                        timestamp = c.buffer().timestamp();
                        if (&sel == &sels.main())
                            main = new_sels.size() + c.selections().main_index();

                        const auto middle = new_sels.insert(new_sels.end(), c.selections().begin(), c.selections().end());
                        std::inplace_merge(new_sels.begin(), middle, new_sels.end(), compare_selections);
                    }
                }
                catch (no_selections_remaining&) {}
            }

            if (not one_selection_succeeded)
            {
                c.selections_write_only() = std::move(sels);
                throw no_selections_remaining{};
            }

            if (not draft)
                c.selections_write_only().set(std::move(new_sels), main);
        }
        else
        {
            const bool collapse_jumps = not (c.flags() & Context::Flags::Draft) and c.has_buffer();
            auto& jump_list = c.jump_list();
            const size_t prev_index = jump_list.current_index();
            auto jump = collapse_jumps ? c.selections() : Optional<SelectionList>{};

            func(parser, c);

            // If the jump list got mutated, collapse all jumps into a single one from original selections
            if (auto index = jump_list.current_index();
                collapse_jumps and index > prev_index and
                contains(BufferManager::instance(), &jump->buffer()))
                jump_list.push(std::move(*jump), prev_index);
        }
    };

    ClientManager& cm = ClientManager::instance();
    if (auto client_names = parser.get_switch("client"))
    {
        if (*client_names == "*")
        {
            for (auto&& client : ClientManager::instance()
                               | transform(&UniquePtr<Client>::get)
                               | gather<Vector<SafePtr<Client>>>()) // gather as we might be mutating the client list in the loop.
                context_wrap_for_context(client->context());
        }
        else
            for (auto&& name : *client_names
                             | split<StringView>(',', '\\')
                             | transform(unescape<',', '\\'>))
                context_wrap_for_context(ClientManager::instance().get_client(name).context());
    }
    else if (auto client_name = parser.get_switch("try-client"))
    {
        Client* client = cm.get_client_ifp(*client_name);
        context_wrap_for_context(client ? client->context() : context);
    }
    else
        context_wrap_for_context(context);
}

const CommandDesc execute_keys_cmd = {
    "execute-keys",
    "exec",
    "execute-keys [<switches>] <keys>: execute given keys as if entered by user",
    make_context_wrap_params<3>({{
        {"save-regs",  {ArgCompleter{}, "restore all given registers after execution (default: '/\"|^@:')"}},
        {"with-maps",  {{}, "use user defined key mapping when executing keys"}},
        {"with-hooks", {{}, "trigger hooks while executing keys"}}
    }}),
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        context_wrap(parser, context, "/\"|^@:", [](const ParametersParser& parser, Context& context) {
            ScopedSetBool disable_keymaps(context.keymaps_disabled(), not parser.get_switch("with-maps"));
            ScopedSetBool disable_hooks(context.hooks_disabled(), not parser.get_switch("with-hooks"));

            for (auto& key : parser | transform(parse_keys) | flatten())
                context.input_handler().handle_key(key);
        });
    }
};


const CommandDesc evaluate_commands_cmd = {
    "evaluate-commands",
    "eval",
    "evaluate-commands [<switches>] <commands>...: execute commands as if entered by user",
    make_context_wrap_params<3>({{
        {"save-regs",  {ArgCompleter{}, "restore all given registers after execution (default: '')"}},
        {"no-hooks", { {}, "disable hooks while executing commands" }},
        {"verbatim", { {}, "do not reparse argument" }}
    }}),
    CommandFlags::None,
    CommandHelper{},
    CommandManager::NestedCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext& shell_context)
    {
        context_wrap(parser, context, {}, [&](const ParametersParser& parser, Context& context) {
            const bool no_hooks = context.hooks_disabled() or parser.get_switch("no-hooks");
            ScopedSetBool disable_hooks(context.hooks_disabled(), no_hooks);

            LocalScope local_scope{context};
            if (parser.get_switch("verbatim"))
                CommandManager::instance().execute_single_command(parser | gather<Vector<String>>(), context, shell_context);
            else
                CommandManager::instance().execute(join(parser, ' ', false), context, shell_context);
        });
    }
};

struct CapturedShellContext
{
    explicit CapturedShellContext(const ShellContext& sc)
      : params{sc.params.begin(), sc.params.end()}, env_vars{sc.env_vars} {}

    Vector<String> params;
    EnvVarMap env_vars;

    operator ShellContext() const { return { params, env_vars }; }
};

const CommandDesc prompt_cmd = {
    "prompt",
    nullptr,
    "prompt [<switches>] <prompt> <command>: prompt the user to enter a text string "
    "and then executes <command>, entered text is available in the 'text' value",
    ParameterDesc{
        { { "init", { ArgCompleter{}, "set initial prompt content" } },
          { "password", { {}, "Do not display entered text and clear reg after command" } },
          { "menu", { {}, "treat completions as the only valid inputs" } },
          { "file-completion", { {}, "use file completion for prompt" } },
          { "client-completion", { {}, "use client completion for prompt" } },
          { "buffer-completion", { {}, "use buffer completion for prompt" } },
          { "command-completion", { {}, "use command completion for prompt" } },
          { "shell-completion", { {}, "use shell command completion for prompt" } },
          { "shell-script-completion", { ArgCompleter{}, "use shell command completion for prompt" } },
          { "shell-script-candidates", { ArgCompleter{}, "use shell command completion for prompt" } },
          { "on-change", { ArgCompleter{}, "command to execute whenever the prompt changes" } },
          { "on-abort", { ArgCompleter{}, "command to execute whenever the prompt is canceled" } } },
        ParameterDesc::Flags::None, 2, 2
    },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext& shell_context)
    {
        const String& command = parser[1];
        auto initstr = parser.get_switch("init").value_or(StringView{});

        const Completions::Flags completions_flags = parser.get_switch("menu") ?
            Completions::Flags::Menu : Completions::Flags::None;
        PromptCompleterAdapter completer = parse_completion_switch(parser, completions_flags);

        const auto flags = parser.get_switch("password") ?
            PromptFlags::Password : PromptFlags::None;

        context.input_handler().prompt(
            parser[0], initstr.str(), {}, context.faces()["Prompt"],
            flags, '_', std::move(completer),
            [command,
             on_change = parser.get_switch("on-change").value_or("").str(),
             on_abort = parser.get_switch("on-abort").value_or("").str(),
             sc = CapturedShellContext{shell_context}]
            (StringView str, PromptEvent event, Context& context) mutable
            {
                if ((event == PromptEvent::Abort and on_abort.empty()) or
                    (event == PromptEvent::Change and on_change.empty()))
                    return;

                sc.env_vars["text"_sv] = String{String::NoCopy{}, str};
                auto remove_text = OnScopeEnd([&] {
                    sc.env_vars.erase("text"_sv);
                });

                StringView cmd;
                switch (event)
                {
                    case PromptEvent::Validate: cmd = command; break;
                    case PromptEvent::Change: cmd = on_change; break;
                    case PromptEvent::Abort: cmd = on_abort; break;
                }
                try
                {
                    CommandManager::instance().execute(cmd, context, sc);
                }
                catch (Kakoune::runtime_error& error)
                {
                    context.print_status({error.what().str(), context.faces()["Error"]});
                    context.hooks().run_hook(Hook::RuntimeError, error.what(), context);
                }
            });
    }
};

const CommandDesc on_key_cmd = {
    "on-key",
    nullptr,
    "on-key [<switches>] <command>: wait for next user key and then execute <command>, "
    "with key available in the `key` value",
    ParameterDesc{
        { { "mode-name", { ArgCompleter{}, "set mode name to use" } } },
        ParameterDesc::Flags::None, 1, 1
    },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext& shell_context)
    {
        String command = parser[0];

        CapturedShellContext sc{shell_context};
        context.input_handler().on_next_key(
            parser.get_switch("mode-name").value_or("on-key"),
            KeymapMode::None, [=](Key key, Context& context) mutable {
            sc.env_vars["key"_sv] = to_string(key);

            CommandManager::instance().execute(command, context, sc);
        });
    }
};

const CommandDesc info_cmd = {
    "info",
    nullptr,
    "info [<switches>] <text>: display an info box containing <text>",
    ParameterDesc{
        { { "anchor", { ArgCompleter{}, "set info anchoring <line>.<column>" } },
          { "style", { {arg_completer(Array{"above", "below", "menu", "modal"})}, "set info style (above, below, menu, modal)" } },
          { "markup", { {}, "parse markup" } },
          { "title", { ArgCompleter{}, "set info title" } } },
        ParameterDesc::Flags::None, 0, 1
    },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        if (not context.has_client())
            return;

        const InfoStyle style = parser.get_switch("style").map(
            [](StringView style) -> Optional<InfoStyle> {
                if (style == "above") return InfoStyle::InlineAbove;
                if (style == "below") return InfoStyle::InlineBelow;
                if (style == "menu") return InfoStyle::MenuDoc;
                if (style == "modal") return InfoStyle::Modal;
                throw runtime_error(format("invalid style: '{}'", style));
            }).value_or(parser.get_switch("anchor") ? InfoStyle::Inline : InfoStyle::Prompt);

        context.client().info_hide(style == InfoStyle::Modal);
        if (parser.positional_count() == 0)
            return;

        const BufferCoord pos = parser.get_switch("anchor").map(
            [](StringView anchor) {
                auto dot = find(anchor, '.');
                if (dot == anchor.end())
                    throw runtime_error("expected <line>.<column> for anchor");

                return BufferCoord{str_to_int({anchor.begin(), dot})-1,
                                   str_to_int({dot+1, anchor.end()})-1};
            }).value_or(BufferCoord{});

        auto title = parser.get_switch("title").value_or(StringView{});
        if (parser.get_switch("markup"))
            context.client().info_show(parse_display_line(title, context.faces()),
                                       parse_display_line_list(parser[0], context.faces()),
                                       pos, style);
        else
            context.client().info_show(title.str(), parser[0], pos, style);
    }
};

const CommandDesc try_catch_cmd = {
    "try",
    nullptr,
    "try <cmds> [catch <error_cmds>]...: execute <cmds> in current context.\n"
    "if an error is raised and <error_cmds> is specified, execute it and do\n"
    "not propagate that error. If <error_cmds> raises an error and another\n"
    "<error_cmds> is provided, execute this one and so-on\n",
    ParameterDesc{{}, ParameterDesc::Flags::None, 1},
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext& shell_context)
    {
        if ((parser.positional_count() % 2) != 1)
            throw wrong_argument_count();

        for (size_t i = 1; i < parser.positional_count(); i += 2)
        {
            if (parser[i] != "catch")
                throw runtime_error("usage: try <commands> [catch <on error commands>]...");
        }

        CommandManager& command_manager = CommandManager::instance();
        Optional<ShellContext> shell_context_with_error;
        for (size_t i = 0; i < parser.positional_count(); i += 2)
        {
            if (i == 0 or i < parser.positional_count() - 1)
            {
                try
                {
                    command_manager.execute(parser[i], context,
                                            shell_context_with_error.value_or(shell_context));
                    return;
                }
                catch (const runtime_error& error)
                {
                    shell_context_with_error.emplace(shell_context);
                    shell_context_with_error->env_vars[StringView{"error"}] = error.what().str();
                }
            }
            else
                command_manager.execute(parser[i], context,
                                        shell_context_with_error.value_or(shell_context));
        }
    }
};

static Completions complete_face(const Context& context,
                                 StringView prefix, ByteCount cursor_pos)
{
    return {0_byte, cursor_pos,
            complete(prefix, cursor_pos, context.faces().flatten_faces() |
                     transform([](auto& entry) -> const String& { return entry.key; }))};
}

static String face_doc_helper(const Context& context, CommandParameters params)
{
    if (params.size() < 2)
        return {};
    try
    {
        auto face = context.faces()[params[1]];
        return format("{}:\n{}", params[1], indent(to_string(face)));
    }
    catch (runtime_error&)
    {
        return {};
    }
}

const CommandDesc set_face_cmd = {
    "set-face",
    "face",
    "set-face <scope> <name> <facespec>: set face <name> to <facespec> in <scope>\n"
    "\n"
    "facespec format is:\n"
    "    <fg color>[,<bg color>[,<underline color>]][+<attributes>][@<base>]\n"
    "colors are either a color name, rgb:######, or rgba:######## values.\n"
    "attributes is a combination of:\n"
    "    u: underline, c: curly underline, U: double underline,\n"
    "    i: italic,            b: bold,            r: reverse,\n"
    "    s: strikethrough,     B: blink,           d: dim,\n"
    "    f: final foreground,              g: final background,\n"
    "    a: final attributes,              F: same as +fga\n"
    "facespec can as well just be the name of another face.\n"
    "if a base face is specified, colors and attributes are applied on top of it",
    ParameterDesc{{}, ParameterDesc::Flags::None, 3, 3},
    CommandFlags::None,
    face_doc_helper,
    make_completer(menu(complete_scope), complete_face, complete_face),
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        get_scope(parser[0], context).faces().add_face(parser[1], parser[2], true);

        for (auto& client : ClientManager::instance())
            client->force_redraw();
    }
};

const CommandDesc unset_face_cmd = {
    "unset-face",
    nullptr,
    "unset-face <scope> <name>: remove <face> from <scope>",
    double_params,
    CommandFlags::None,
    face_doc_helper,
    make_completer(menu(complete_scope), menu(complete_face)),
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
       get_scope(parser[0], context).faces().remove_face(parser[1]);
    }
};

const CommandDesc rename_client_cmd = {
    "rename-client",
    nullptr,
    "rename-client <name>: set current client name to <name>",
    single_param,
    CommandFlags::None,
    CommandHelper{},
    make_single_word_completer([](const Context& context){ return context.name(); }),
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        const String& name = parser[0];
        if (not all_of(name, is_identifier))
            throw runtime_error{format("invalid client name: '{}'", name)};
        else if (ClientManager::instance().client_name_exists(name) and
                 context.name() != name)
            throw runtime_error{format("client name '{}' is not unique", name)};
        else
            context.set_name(name);
    }
};

const CommandDesc set_register_cmd = {
    "set-register",
    "reg",
    "set-register <name> <values>...: set register <name> to <values>",
    ParameterDesc{{}, ParameterDesc::Flags::SwitchesAsPositional, 1},
    CommandFlags::None,
    CommandHelper{},
    make_completer(
         [](const Context& context,
            StringView prefix, ByteCount cursor_pos) -> Completions {
             return { 0_byte, cursor_pos,
                      RegisterManager::instance().complete_register_name(prefix, cursor_pos) };
        }),
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        RegisterManager::instance()[parser[0]].set(context, parser.positionals_from(1));
    }
};

const CommandDesc select_cmd = {
    "select",
    nullptr,
    "select <selection_desc>...: select given selections\n"
    "\n"
    "selection_desc format is <anchor_line>.<anchor_column>,<cursor_line>.<cursor_column>",
    ParameterDesc{{
            {"timestamp", {ArgCompleter{}, "specify buffer timestamp at which those selections are valid"}},
            {"codepoint", {{}, "columns are specified in codepoints, not bytes"}},
            {"display-column", {{}, "columns are specified in display columns, not bytes"}}
        },
        ParameterDesc::Flags::SwitchesOnlyAtStart, 1
    },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        const size_t timestamp = parser.get_switch("timestamp").map(str_to_int_ifp).cast<size_t>().value_or(buffer.timestamp());
        ColumnType column_type = ColumnType::Byte;
        if (parser.get_switch("codepoint"))
            column_type = ColumnType::Codepoint;
        else if (parser.get_switch("display-column"))
            column_type = ColumnType::DisplayColumn;
        ColumnCount tabstop = context.options()["tabstop"].get<int>();
        ScopedSelectionEdition selection_edition{context};
        context.selections_write_only() = selection_list_from_strings(buffer, column_type, parser.positionals_from(0), timestamp, 0, tabstop);
    }
};

const CommandDesc change_directory_cmd = {
    "change-directory",
    "cd",
    "change-directory [<directory>]: change the server's working directory to <directory>, or the home directory if unspecified",
    single_optional_param,
    CommandFlags::None,
    CommandHelper{},
    make_completer(
         [](const Context& context,
            StringView prefix, ByteCount cursor_pos) -> Completions {
             return { 0_byte, cursor_pos,
                      complete_filename(prefix,
                                        context.options()["ignored_files"].get<Regex>(),
                                        cursor_pos, FilenameFlags::OnlyDirectories),
                      Completions::Flags::Menu };
        }),
    [](const ParametersParser& parser, Context& ctx, const ShellContext&)
    {
        StringView target = parser.positional_count() == 1 ? StringView{parser[0]} : "~";
        auto path = real_path(parse_filename(target));
        if (chdir(path.c_str()) != 0)
            throw runtime_error(format("unable to change to directory: '{}'", target));
        for (auto& buffer : BufferManager::instance())
            buffer->update_display_name();
        ctx.hooks().run_hook(Hook::EnterDirectory, path, ctx);
    }
};

const CommandDesc rename_session_cmd = {
    "rename-session",
    nullptr,
    "rename-session <name>: change remote session name",
    single_param,
    CommandFlags::None,
    CommandHelper{},
    make_single_word_completer([](const Context&){ return Server::instance().session(); }),
    [](const ParametersParser& parser, Context& ctx, const ShellContext&)
    {
        String old_name = Server::instance().session();
        if (not Server::instance().rename_session(parser[0]))
            throw runtime_error(format("unable to rename current session: '{}' may be already in use", parser[0]));
        ctx.hooks().run_hook(Hook::SessionRenamed, format("{}:{}", old_name, Server::instance().session()), ctx);
    }
};

const CommandDesc fail_cmd = {
    "fail",
    nullptr,
    "fail [<message>]: raise an error with the given message",
    ParameterDesc{},
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context&, const ShellContext&)
    {
        throw failure{join(parser, " ")};
    }
};

const CommandDesc declare_user_mode_cmd = {
    "declare-user-mode",
    nullptr,
    "declare-user-mode <name>: add a new user keymap mode",
    single_param,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        context.keymaps().add_user_mode(parser[0]);
    }
};

// We need ownership of the mode_name in the lock case
void enter_user_mode(Context& context, String mode_name, KeymapMode mode, bool lock)
{
    on_next_key_with_autoinfo(context, format("user.{}", mode_name), KeymapMode::None,
                             [mode_name, mode, lock](Key key, Context& context) mutable {
        if (key == Key::Escape)
            return;

        if (context.keymaps().is_mapped(key, mode))
        {
            ScopedSetBool disable_keymaps(context.keymaps_disabled());

            InputHandler::ScopedForceNormal force_normal{context.input_handler(), {}};

            ScopedEdition edition(context);
            for (auto& key : context.keymaps().get_mapping_keys(key, mode))
                context.input_handler().handle_key(key);
        }

        if (lock)
            enter_user_mode(context, std::move(mode_name), mode, true);
    }, lock ? format("{} (lock)", mode_name) : mode_name,
    build_autoinfo_for_mapping(context, mode, {}));
}

const CommandDesc enter_user_mode_cmd = {
    "enter-user-mode",
    nullptr,
    "enter-user-mode [<switches>] <name>: enable <name> keymap mode for next key",
    ParameterDesc{
        { { "lock", { {}, "stay in mode until <esc> is pressed" } } },
        ParameterDesc::Flags::None, 1, 1
    },
    CommandFlags::None,
    CommandHelper{},
    [](const Context& context,
       CommandParameters params, size_t token_to_complete,
       ByteCount pos_in_token) -> Completions
    {
        if (token_to_complete == 0)
        {
            return { 0_byte, params[0].length(),
                     complete(params[0], pos_in_token, context.keymaps().user_modes()),
                     Completions::Flags::Menu };
        }
        return {};
    },
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        auto lock = (bool)parser.get_switch("lock");
        KeymapMode mode = parse_keymap_mode(parser[0], context.keymaps().user_modes());
        enter_user_mode(context, parser[0], mode, lock);
    }
};

const CommandDesc provide_module_cmd = {
    "provide-module",
    nullptr,
    "provide-module [<switches>] <name> <cmds>: declares a module <name> provided by <cmds>",
    ParameterDesc{
        { { "override", { {}, "allow overriding an existing module" } } },
        ParameterDesc::Flags::None,
        2, 2
    },
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        const String& module_name = parser[0];
        auto& cm = CommandManager::instance();

        if (not all_of(module_name, is_identifier))
            throw runtime_error(format("invalid module name: '{}'", module_name));

        if (cm.module_defined(module_name) and not parser.get_switch("override"))
            throw runtime_error(format("module '{}' already defined", module_name));
        cm.register_module(module_name, parser[1]);
    }
};

const CommandDesc require_module_cmd = {
    "require-module",
    nullptr,
    "require-module <name>: ensures that <name> module has been loaded",
    single_param,
    CommandFlags::None,
    CommandHelper{},
    make_completer(menu(
         [](const Context&, StringView prefix, ByteCount cursor_pos) {
            return CommandManager::instance().complete_module_name(prefix.substr(0, cursor_pos));
        })),
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        CommandManager::instance().load_module(parser[0], context);
    }
};

}

const CommandDesc tree_sitter_enable_cmd = {
    "tree-sitter-enable",
    nullptr,
    "tree-sitter-enable: enable tree-sitter highlighting for the current buffer",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
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
            create_syntax_tree(buffer, config);
    }
};

const CommandDesc tree_sitter_disable_cmd = {
    "tree-sitter-disable",
    nullptr,
    "tree-sitter-disable: disable tree-sitter highlighting for the current buffer",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        if (has_syntax_tree(buffer))
            remove_syntax_tree(buffer);
    }
};

struct TreeSelectResult
{
    TSNode node;
    bool found;
};

static void ensure_textobject_query(const Buffer& buffer, SyntaxTree& syntax_tree)
{
    if (not has_syntax_tree(buffer))
        throw runtime_error("no tree-sitter syntax tree for this buffer");

    syntax_tree.update(buffer);

    if (not syntax_tree.is_valid())
        throw runtime_error("tree-sitter syntax tree is not valid");
    if (not syntax_tree.config())
        throw runtime_error("no tree-sitter config for this buffer");
    if (not syntax_tree.config()->textobject_query())
        throw runtime_error(format("no textobject query for '{}' — check runtime/queries/{}/textobjects.scm exists",
            syntax_tree.config()->name(), syntax_tree.config()->name()));
}

static uint32_t find_capture_index(TSQuery* query, StringView capture_name)
{
    uint32_t cap_count = ts_query_capture_count(query);
    for (uint32_t i = 0; i < cap_count; ++i)
    {
        uint32_t len = 0;
        const char* name = ts_query_capture_name_for_id(query, i, &len);
        if (StringView{name, (ByteCount)len} == capture_name)
            return i;
    }
    return UINT32_MAX;
}

static Selection node_to_selection(const Buffer& buffer, TSNode node)
{
    TSPoint start_pt = ts_node_start_point(node);
    TSPoint end_pt = ts_node_end_point(node);

    BufferCoord begin{LineCount{(int)start_pt.row}, ByteCount{(int)start_pt.column}};
    BufferCoord end{LineCount{(int)end_pt.row}, ByteCount{(int)end_pt.column}};

    // Clamp to buffer bounds — tree-sitter ERROR nodes can have unexpected coordinates
    auto back = buffer.back_coord();
    begin = std::min(begin, back);
    end = std::min(end, back);

    // Kakoune selections are inclusive on both ends,
    // tree-sitter end is exclusive, so back up one char
    if (end > begin)
        end = buffer.char_prev(end);

    return Selection{begin, end};
}

const CommandDesc tree_select_cmd = {
    "tree-select",
    nullptr,
    "tree-select <object> <inside|around>: select smallest tree-sitter text object containing cursor",
    double_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_textobject_query(buffer, syntax_tree);

        auto object_name = parser[0];
        auto mode = parser[1];

        if (mode != "inside" and mode != "around")
            throw runtime_error("second argument must be 'inside' or 'around'");

        String capture_name = format("{}.{}", object_name, mode);

        auto* query = syntax_tree.config()->textobject_query();
        uint32_t target_capture = find_capture_index(query, capture_name);

        if (target_capture == UINT32_MAX)
            throw runtime_error(format("no textobject capture '{}' for this language", capture_name));

        auto& byte_index = syntax_tree.byte_index();
        auto& selections = context.selections();

        Vector<Selection, MemoryDomain::Selections> new_selections;

        QueryCursorGuard qcursor;
        ts_query_cursor_set_match_limit(qcursor, 256);

        const auto& to_preds = syntax_tree.config()->textobject_predicates();

        for (auto& sel : selections)
        {
            auto cursor = sel.cursor();
            uint32_t cursor_byte = byte_index.byte_offset(cursor);

            ts_query_cursor_exec(qcursor, query, ts_tree_root_node(syntax_tree.tree()));

            TSNode best_node = {};
            uint32_t best_size = UINT32_MAX;
            bool found = false;

            TSQueryMatch match;
            while (ts_query_cursor_next_match(qcursor, &match))
            {
                if (match.pattern_index < (uint32_t)to_preds.size()
                    and not to_preds[(int)match.pattern_index].empty()
                    and not predicates_match(to_preds[(int)match.pattern_index], match, buffer))
                    continue;

                for (uint32_t c = 0; c < match.capture_count; ++c)
                {
                    if (match.captures[c].index != target_capture)
                        continue;

                    TSNode node = match.captures[c].node;
                    uint32_t start = ts_node_start_byte(node);
                    uint32_t end = ts_node_end_byte(node);

                    if (start <= cursor_byte and cursor_byte < end)
                    {
                        uint32_t size = end - start;
                        if (size < best_size)
                        {
                            best_size = size;
                            best_node = node;
                            found = true;
                        }
                    }
                }
            }

            if (found)
                new_selections.push_back(node_to_selection(buffer, best_node));
            else
                new_selections.push_back(sel);
        }

        selections.set(std::move(new_selections), selections.main_index());
    }
};

const CommandDesc tree_select_next_cmd = {
    "tree-select-next",
    nullptr,
    "tree-select-next <object>: select next tree-sitter text object after cursor",
    single_param,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_textobject_query(buffer, syntax_tree);

        auto object_name = parser[0];
        String capture_name = format("{}.around", object_name);

        auto* query = syntax_tree.config()->textobject_query();
        uint32_t target_capture = find_capture_index(query, capture_name);

        if (target_capture == UINT32_MAX)
            throw runtime_error(format("no textobject capture '{}' for this language", capture_name));

        auto& byte_index = syntax_tree.byte_index();
        auto& selections = context.selections();

        Vector<Selection, MemoryDomain::Selections> new_selections;

        QueryCursorGuard qcursor;
        ts_query_cursor_set_match_limit(qcursor, 256);

        const auto& to_preds = syntax_tree.config()->textobject_predicates();

        for (auto& sel : selections)
        {
            auto cursor = sel.cursor();
            uint32_t cursor_byte = byte_index.byte_offset(cursor);

            ts_query_cursor_exec(qcursor, query, ts_tree_root_node(syntax_tree.tree()));

            TSNode best_node = {};
            uint32_t best_start = UINT32_MAX;
            bool found = false;

            TSQueryMatch match;
            while (ts_query_cursor_next_match(qcursor, &match))
            {
                if (match.pattern_index < (uint32_t)to_preds.size()
                    and not to_preds[(int)match.pattern_index].empty()
                    and not predicates_match(to_preds[(int)match.pattern_index], match, buffer))
                    continue;

                for (uint32_t c = 0; c < match.capture_count; ++c)
                {
                    if (match.captures[c].index != target_capture)
                        continue;

                    TSNode node = match.captures[c].node;
                    uint32_t start = ts_node_start_byte(node);

                    if (start > cursor_byte and start < best_start)
                    {
                        best_start = start;
                        best_node = node;
                        found = true;
                    }
                }
            }

            if (found)
                new_selections.push_back(node_to_selection(buffer, best_node));
            else
                new_selections.push_back(sel);
        }

        selections.set(std::move(new_selections), selections.main_index());
    }
};

const CommandDesc tree_select_prev_cmd = {
    "tree-select-prev",
    nullptr,
    "tree-select-prev <object>: select previous tree-sitter text object before cursor",
    single_param,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_textobject_query(buffer, syntax_tree);

        auto object_name = parser[0];
        String capture_name = format("{}.around", object_name);

        auto* query = syntax_tree.config()->textobject_query();
        uint32_t target_capture = find_capture_index(query, capture_name);

        if (target_capture == UINT32_MAX)
            throw runtime_error(format("no textobject capture '{}' for this language", capture_name));

        auto& byte_index = syntax_tree.byte_index();
        auto& selections = context.selections();

        Vector<Selection, MemoryDomain::Selections> new_selections;

        QueryCursorGuard qcursor;
        ts_query_cursor_set_match_limit(qcursor, 256);

        const auto& to_preds = syntax_tree.config()->textobject_predicates();

        for (auto& sel : selections)
        {
            auto cursor = sel.cursor();
            uint32_t cursor_byte = byte_index.byte_offset(cursor);

            ts_query_cursor_exec(qcursor, query, ts_tree_root_node(syntax_tree.tree()));

            TSNode best_node = {};
            uint32_t best_start = 0;
            bool found = false;

            TSQueryMatch match;
            while (ts_query_cursor_next_match(qcursor, &match))
            {
                if (match.pattern_index < (uint32_t)to_preds.size()
                    and not to_preds[(int)match.pattern_index].empty()
                    and not predicates_match(to_preds[(int)match.pattern_index], match, buffer))
                    continue;

                for (uint32_t c = 0; c < match.capture_count; ++c)
                {
                    if (match.captures[c].index != target_capture)
                        continue;

                    TSNode node = match.captures[c].node;
                    uint32_t start = ts_node_start_byte(node);

                    if (start < cursor_byte and start >= best_start)
                    {
                        best_start = start;
                        best_node = node;
                        found = true;
                    }
                }
            }

            if (found)
                new_selections.push_back(node_to_selection(buffer, best_node));
            else
                new_selections.push_back(sel);
        }

        selections.set(std::move(new_selections), selections.main_index());
    }
};

static void ensure_syntax_tree(const Buffer& buffer, SyntaxTree& syntax_tree)
{
    if (not has_syntax_tree(buffer))
        throw runtime_error("no tree-sitter syntax tree for this buffer");

    syntax_tree.update(buffer);

    if (not syntax_tree.is_valid())
        throw runtime_error("tree-sitter syntax tree is not valid");
}

static TSNode find_node_at_cursor(const SyntaxTree& syntax_tree, BufferCoord cursor)
{
    auto& byte_index = syntax_tree.byte_index();
    uint32_t byte = byte_index.byte_offset(cursor);
    return ts_node_named_descendant_for_byte_range(
        ts_tree_root_node(syntax_tree.tree()), byte, byte);
}

const CommandDesc tree_parent_cmd = {
    "tree-parent",
    nullptr,
    "tree-parent: select parent AST node",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto& selections = context.selections();
        Vector<Selection, MemoryDomain::Selections> new_selections;

        for (auto& sel : selections)
        {
            TSNode node = find_node_at_cursor(syntax_tree, sel.cursor());
            if (ts_node_is_null(node))
            {
                new_selections.push_back(sel);
                continue;
            }

            TSNode parent = ts_node_parent(node);
            if (ts_node_is_null(parent))
                new_selections.push_back(sel);
            else
                new_selections.push_back(node_to_selection(buffer, parent));
        }

        selections.set(std::move(new_selections), selections.main_index());
    }
};

const CommandDesc tree_first_child_cmd = {
    "tree-first-child",
    nullptr,
    "tree-first-child: select first named child AST node",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto& selections = context.selections();
        Vector<Selection, MemoryDomain::Selections> new_selections;

        for (auto& sel : selections)
        {
            TSNode node = find_node_at_cursor(syntax_tree, sel.cursor());
            if (ts_node_is_null(node) or ts_node_named_child_count(node) == 0)
            {
                new_selections.push_back(sel);
                continue;
            }

            TSNode child = ts_node_named_child(node, 0);
            if (ts_node_is_null(child))
                new_selections.push_back(sel);
            else
                new_selections.push_back(node_to_selection(buffer, child));
        }

        selections.set(std::move(new_selections), selections.main_index());
    }
};

const CommandDesc tree_next_sibling_cmd = {
    "tree-next-sibling",
    nullptr,
    "tree-next-sibling: select next named sibling AST node",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto& selections = context.selections();
        Vector<Selection, MemoryDomain::Selections> new_selections;

        for (auto& sel : selections)
        {
            TSNode node = find_node_at_cursor(syntax_tree, sel.cursor());
            if (ts_node_is_null(node))
            {
                new_selections.push_back(sel);
                continue;
            }

            TSNode sibling = ts_node_next_named_sibling(node);
            if (ts_node_is_null(sibling))
                new_selections.push_back(sel);
            else
                new_selections.push_back(node_to_selection(buffer, sibling));
        }

        selections.set(std::move(new_selections), selections.main_index());
    }
};

const CommandDesc tree_prev_sibling_cmd = {
    "tree-prev-sibling",
    nullptr,
    "tree-prev-sibling: select previous named sibling AST node",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto& selections = context.selections();
        Vector<Selection, MemoryDomain::Selections> new_selections;

        for (auto& sel : selections)
        {
            TSNode node = find_node_at_cursor(syntax_tree, sel.cursor());
            if (ts_node_is_null(node))
            {
                new_selections.push_back(sel);
                continue;
            }

            TSNode sibling = ts_node_prev_named_sibling(node);
            if (ts_node_is_null(sibling))
                new_selections.push_back(sel);
            else
                new_selections.push_back(node_to_selection(buffer, sibling));
        }

        selections.set(std::move(new_selections), selections.main_index());
    }
};

const CommandDesc tree_select_node_cmd = {
    "tree-select-node",
    nullptr,
    "tree-select-node: select the AST node at cursor position",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto& selections = context.selections();
        Vector<Selection, MemoryDomain::Selections> new_selections;

        for (auto& sel : selections)
        {
            TSNode node = find_node_at_cursor(syntax_tree, sel.cursor());
            if (ts_node_is_null(node))
                new_selections.push_back(sel);
            else
                new_selections.push_back(node_to_selection(buffer, node));
        }

        selections.set(std::move(new_selections), selections.main_index());
    }
};

// Expansion history stack — stores previous selections so tree-shrink
// can retrace the exact path of tree-expand.
struct ExpandHistoryEntry
{
    String buffer_name;
    Vector<Selection, MemoryDomain::Selections> selections;
};
static Vector<ExpandHistoryEntry> expand_history;

const CommandDesc tree_expand_cmd = {
    "tree-expand",
    nullptr,
    "tree-expand: expand selection to next larger AST node",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto& selections = context.selections();

        // Save current selections to history before expanding
        Vector<Selection, MemoryDomain::Selections> saved;
        for (auto& sel : selections)
            saved.push_back(sel);
        expand_history.push_back({buffer.name(), std::move(saved)});

        // Cap history depth
        if (expand_history.size() > 32)
            expand_history.erase(expand_history.begin());

        Vector<Selection, MemoryDomain::Selections> new_selections;

        for (auto& sel : selections)
        {
            TSNode node = find_node_at_cursor(syntax_tree, sel.cursor());
            if (ts_node_is_null(node))
            {
                new_selections.push_back(sel);
                continue;
            }

            auto node_sel = node_to_selection(buffer, node);
            while (not ts_node_is_null(node) and
                   node_sel.min() >= sel.min() and node_sel.max() <= sel.max())
            {
                TSNode parent = ts_node_parent(node);
                if (ts_node_is_null(parent))
                    break;
                node = parent;
                node_sel = node_to_selection(buffer, node);
            }

            new_selections.push_back(node_sel);
        }

        selections.set(std::move(new_selections), selections.main_index());
    }
};

const CommandDesc tree_shrink_cmd = {
    "tree-shrink",
    nullptr,
    "tree-shrink: restore selection from before the last tree-expand",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        if (expand_history.empty())
            throw runtime_error("no tree-expand history to shrink back to");

        if (expand_history.back().buffer_name != context.buffer().name())
        {
            expand_history.clear();
            throw runtime_error("expand history invalid — buffer changed");
        }

        auto& selections = context.selections();
        auto restored = std::move(expand_history.back().selections);
        expand_history.pop_back();

        if (not restored.empty())
            selections.set(std::move(restored), 0);
    }
};

const CommandDesc tree_select_all_cmd = {
    "tree-select-all",
    nullptr,
    "tree-select-all <object>: select all occurrences of a text object in buffer",
    single_param,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_textobject_query(buffer, syntax_tree);

        auto object_name = parser[0];
        String capture_name = format("{}.around", object_name);

        auto* query = syntax_tree.config()->textobject_query();
        uint32_t target = find_capture_index(query, capture_name);

        if (target == UINT32_MAX)
            throw runtime_error(format("no textobject capture '{}' for this language", capture_name));

        QueryCursorGuard cursor;
        ts_query_cursor_set_match_limit(cursor, 256);
        ts_query_cursor_exec(cursor, query, ts_tree_root_node(syntax_tree.tree()));

        const auto& to_preds = syntax_tree.config()->textobject_predicates();

        Vector<Selection, MemoryDomain::Selections> new_selections;
        TSQueryMatch match;
        while (ts_query_cursor_next_match(cursor, &match))
        {
            if (match.pattern_index < (uint32_t)to_preds.size()
                and not to_preds[(int)match.pattern_index].empty()
                and not predicates_match(to_preds[(int)match.pattern_index], match, buffer))
                continue;

            for (uint32_t c = 0; c < match.capture_count; ++c)
            {
                if (match.captures[c].index != target)
                    continue;
                new_selections.push_back(node_to_selection(buffer, match.captures[c].node));
            }
        }

        if (new_selections.empty())
            throw runtime_error(format("no '{}' found in buffer", object_name));

        context.selections().set(std::move(new_selections), 0);
    }
};

const CommandDesc tree_filter_cmd = {
    "tree-filter",
    nullptr,
    "tree-filter <object>: keep only selections inside the given text object",
    single_param,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_textobject_query(buffer, syntax_tree);

        auto object_name = parser[0];
        String capture_name = format("{}.around", object_name);

        auto* query = syntax_tree.config()->textobject_query();
        uint32_t target = find_capture_index(query, capture_name);

        if (target == UINT32_MAX)
            throw runtime_error(format("no textobject capture '{}' for this language", capture_name));

        // Collect all object ranges
        QueryCursorGuard cursor;
        ts_query_cursor_set_match_limit(cursor, 256);
        ts_query_cursor_exec(cursor, query, ts_tree_root_node(syntax_tree.tree()));

        const auto& to_preds = syntax_tree.config()->textobject_predicates();

        Vector<BufferRange> object_ranges;
        TSQueryMatch match;
        while (ts_query_cursor_next_match(cursor, &match))
        {
            if (match.pattern_index < (uint32_t)to_preds.size()
                and not to_preds[(int)match.pattern_index].empty()
                and not predicates_match(to_preds[(int)match.pattern_index], match, buffer))
                continue;

            for (uint32_t c = 0; c < match.capture_count; ++c)
            {
                if (match.captures[c].index != target)
                    continue;
                auto sel = node_to_selection(buffer, match.captures[c].node);
                object_ranges.push_back({sel.min(), sel.max()});
            }
        }

        // Filter selections: keep only those contained in an object range
        auto& selections = context.selections();
        Vector<Selection, MemoryDomain::Selections> kept;
        for (auto& sel : selections)
        {
            for (auto& obj : object_ranges)
            {
                if (sel.min() >= obj.begin and sel.max() <= obj.end)
                {
                    kept.push_back(sel);
                    break;
                }
            }
        }

        if (kept.empty())
            throw runtime_error(format("no selections inside '{}'", object_name));

        selections.set(std::move(kept), 0);
    }
};

// Build a fold range-spec string for the given line range.
// Returns empty Optional if the range is not foldable.
static Optional<String> build_fold_spec(const Buffer& buffer,
                                        LineCount first_line, LineCount last_line)
{
    if (first_line + 1 >= buffer.line_count())
        return {};

    auto prev_line_len = buffer[last_line - 1].length();
    if (prev_line_len == 0)
        return {};

    BufferCoord fold_begin{first_line + 1, 0_byte};
    BufferCoord fold_end = buffer.char_prev({last_line, 0_byte});

    int folded_lines = (int)(last_line - first_line) - 1;
    auto inner_line = buffer[first_line + 1];
    ByteCount ws = 0;
    while (ws < inner_line.length() and
           (inner_line[(int)ws] == ' ' or inner_line[(int)ws] == '\t'))
        ws++;
    auto len = inner_line.length();
    if (len > 0 and inner_line[(int)(len - 1)] == '\n')
        len--;
    String preview{inner_line.substr(ws, std::min(len - ws, 40_byte))};
    String indent{inner_line.substr(0_byte, ws)};
    if (not indent.empty() and indent.back() == '\n')
        indent = String{indent.substr(0_byte, indent.length() - 1)};

    String rs = format("{}.{},{}.{}|",
                       fold_begin.line + 1, fold_begin.column + 1,
                       fold_end.line + 1, fold_end.column + 1);
    rs += "{ts_fold}";
    rs += indent;
    rs += format("+-- {} {}: {}\n", folded_lines,
                 folded_lines == 1 ? "line" : "lines", preview);
    return rs;
}

const CommandDesc tree_fold_cmd = {
    "tree-fold",
    nullptr,
    "tree-fold: fold the AST node at cursor, collapsing inner lines to a placeholder",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto& sel = context.selections().main();
        TSNode node = find_node_at_cursor(syntax_tree, sel.cursor());

        // Walk up to find a multi-line node suitable for folding
        while (not ts_node_is_null(node))
        {
            TSPoint start = ts_node_start_point(node);
            TSPoint end = ts_node_end_point(node);
            if (end.row > start.row + 1)  // spans at least 3 lines
                break;
            node = ts_node_parent(node);
        }

        if (ts_node_is_null(node))
            throw runtime_error("no foldable node at cursor");

        TSPoint start = ts_node_start_point(node);
        TSPoint end = ts_node_end_point(node);

        auto first_line = LineCount{(int)start.row};
        auto last_line = LineCount{(int)end.row};

        auto fold_spec = build_fold_spec(buffer, first_line, last_line);
        if (not fold_spec)
            throw runtime_error("cannot fold — node at buffer boundary");

        Option& opt = context.options().get_local_option("tree_sitter_folds");
        opt.add_from_strings(ConstArrayView<String>{*fold_spec});
    }
};

const CommandDesc tree_unfold_cmd = {
    "tree-unfold",
    nullptr,
    "tree-unfold: remove the fold at cursor position",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto cursor = context.selections().main().cursor();

        // Get the option and update it to current timestamp
        Option& opt = context.options().get_local_option("tree_sitter_folds");
        opt.update(context);

        // Get the current strings representation: "timestamp range1|text range2|text ..."
        auto strs = opt.get_as_strings();
        if (strs.size() <= 1)
            throw runtime_error("no fold at cursor position");

        bool found = false;
        Vector<String> kept;
        kept.push_back(format("{}", buffer.timestamp()));

        for (size_t i = 1; i < strs.size(); ++i)
        {
            // Parse the range part (before |)
            auto pipe = find(strs[i], '|');
            if (pipe == strs[i].end())
            {
                kept.push_back(strs[i]);
                continue;
            }
            StringView range_part{strs[i].begin(), pipe};

            // Parse as InclusiveBufferRange to check containment
            try
            {
                auto range = option_from_string(Meta::Type<InclusiveBufferRange>{}, range_part);
                if (range.first <= cursor and cursor <= range.last)
                {
                    found = true;
                    continue;  // skip this fold
                }
            }
            catch (...)
            {
                // If parsing fails, keep the entry
            }

            kept.push_back(strs[i]);
        }

        if (not found)
            throw runtime_error("no fold at cursor position");

        opt.set_from_strings(ConstArrayView<String>{kept});
    }
};

const CommandDesc tree_fold_all_cmd = {
    "tree-fold-all",
    nullptr,
    "tree-fold-all <object>: fold all occurrences of a text object",
    single_param,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser& parser, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_textobject_query(buffer, syntax_tree);

        auto object_name = parser[0];
        String capture_name = format("{}.around", object_name);

        auto* query = syntax_tree.config()->textobject_query();
        uint32_t target = find_capture_index(query, capture_name);

        if (target == UINT32_MAX)
            throw runtime_error(format("no textobject capture '{}' for this language", capture_name));

        QueryCursorGuard cursor;
        ts_query_cursor_set_match_limit(cursor, 256);
        ts_query_cursor_exec(cursor, query, ts_tree_root_node(syntax_tree.tree()));

        const auto& to_preds = syntax_tree.config()->textobject_predicates();

        Vector<String> range_strs;
        TSQueryMatch match;
        while (ts_query_cursor_next_match(cursor, &match))
        {
            if (match.pattern_index < (uint32_t)to_preds.size()
                and not to_preds[(int)match.pattern_index].empty()
                and not predicates_match(to_preds[(int)match.pattern_index], match, buffer))
                continue;

            for (uint32_t c = 0; c < match.capture_count; ++c)
            {
                if (match.captures[c].index != target)
                    continue;

                TSNode node = match.captures[c].node;
                TSPoint start = ts_node_start_point(node);
                TSPoint end = ts_node_end_point(node);

                // Only fold multi-line nodes
                if (end.row <= start.row + 1)
                    continue;

                auto first_line = LineCount{(int)start.row};
                auto last_line = LineCount{(int)end.row};

                auto fold_spec = build_fold_spec(buffer, first_line, last_line);
                if (fold_spec)
                    range_strs.push_back(std::move(*fold_spec));
            }
        }

        if (range_strs.empty())
            throw runtime_error(format("no foldable '{}' found in buffer", object_name));

        Option& opt = context.options().get_local_option("tree_sitter_folds");
        opt.add_from_strings(ConstArrayView<String>{range_strs});
    }
};

const CommandDesc tree_unfold_all_cmd = {
    "tree-unfold-all",
    nullptr,
    "tree-unfold-all: remove all folds",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto timestamp = format("{}", buffer.timestamp());
        Option& opt = context.options().get_local_option("tree_sitter_folds");
        opt.set_from_strings(ConstArrayView<String>{timestamp});
    }
};

const CommandDesc tree_update_context_cmd = {
    "tree-update-context",
    nullptr,
    "tree-update-context: set tree_context option to the enclosing function/class name",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        if (not context.has_window())
            return;

        auto& buffer = context.buffer();
        if (not has_syntax_tree(buffer))
        {
            context.options().get_local_option("tree_context").set_from_strings(ConstArrayView<String>{""});
            return;
        }

        auto& syntax_tree = get_syntax_tree(buffer);
        syntax_tree.update(buffer);

        if (not syntax_tree.is_valid())
        {
            context.options().get_local_option("tree_context").set_from_strings(ConstArrayView<String>{""});
            return;
        }

        // Get the first visible line from the window's display setup
        auto first_visible_line = context.window().last_display_setup().first_line;

        String context_str;

        auto* config = syntax_tree.config();
        if (config and config->textobject_query())
        {
            auto* query = config->textobject_query();
            uint32_t func_capture = find_capture_index(query, "function.around");
            uint32_t class_capture = find_capture_index(query, "class.around");

            const auto& to_preds = config->textobject_predicates();

            // O(depth): find node at visible line, walk up looking for
            // a function/class ancestor that starts above the viewport.
            auto& byte_index = syntax_tree.byte_index();
            uint32_t visible_byte = byte_index.byte_offset({first_visible_line, 0_byte});
            TSNode node = ts_node_named_descendant_for_byte_range(
                ts_tree_root_node(syntax_tree.tree()), visible_byte, visible_byte);

            while (not ts_node_is_null(node))
            {
                TSPoint nstart = ts_node_start_point(node);
                if ((int)nstart.row < (int)first_visible_line)
                {
                    // Check if this node matches a function/class capture
                    QueryCursorGuard qcursor;
                    ts_query_cursor_set_match_limit(qcursor, 64);
                    ts_query_cursor_set_byte_range(qcursor,
                        ts_node_start_byte(node), ts_node_end_byte(node));
                    ts_query_cursor_exec(qcursor, query,
                        ts_tree_root_node(syntax_tree.tree()));

                    TSQueryMatch match;
                    bool is_context_node = false;
                    while (ts_query_cursor_next_match(qcursor, &match))
                    {
                        if (match.pattern_index < (uint32_t)to_preds.size()
                            and not to_preds[(int)match.pattern_index].empty()
                            and not predicates_match(to_preds[(int)match.pattern_index], match, buffer))
                            continue;

                        for (uint32_t c = 0; c < match.capture_count; ++c)
                        {
                            if ((match.captures[c].index == func_capture or
                                 match.captures[c].index == class_capture) and
                                ts_node_eq(match.captures[c].node, node))
                            {
                                is_context_node = true;
                                break;
                            }
                        }
                        if (is_context_node)
                            break;
                    }

                    if (is_context_node)
                    {
                        auto ctx_line = LineCount{(int)nstart.row};
                        if (ctx_line < buffer.line_count())
                        {
                            auto content = buffer[ctx_line];
                            auto len = content.length();
                            if (len > 0 and content[(int)(len - 1)] == '\n')
                                len = len - 1;
                            ByteCount ws = 0;
                            while (ws < len and (content[(int)ws] == ' ' or content[(int)ws] == '\t'))
                                ws++;
                            auto end = len;
                            while (end > ws and (content[(int)(end - 1)] == '{' or
                                                 content[(int)(end - 1)] == ' ' or
                                                 content[(int)(end - 1)] == '\t'))
                                end--;
                            context_str = String{content.substr(ws, end - ws)};
                        }
                        break;
                    }
                }
                node = ts_node_parent(node);
            }
        }

        context.options().get_local_option("tree_context").set_from_strings(
            ConstArrayView<String>{context_str});
    }
};

const CommandDesc tree_indent_cmd = {
    "tree-indent",
    nullptr,
    "tree-indent: reindent selection based on AST indent queries",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto* config = syntax_tree.config();
        if (not config or not config->indent_query())
            throw runtime_error("no indent query for this buffer's language");

        TSQuery* query = config->indent_query();

        // Find @indent and @outdent capture indices
        uint32_t indent_capture = find_capture_index(query, "indent");
        uint32_t outdent_capture = find_capture_index(query, "outdent");

        // Execute indent query on full tree
        QueryCursorGuard cursor;
        ts_query_cursor_set_match_limit(cursor, 256);
        ts_query_cursor_exec(cursor, query, ts_tree_root_node(syntax_tree.tree()));

        const auto& ind_preds = config->indent_predicates();

        // Build per-line indent adjustments
        auto line_count = (int)buffer.line_count();
        Vector<int> indent_delta(line_count, 0);

        TSQueryMatch match;
        uint32_t capture_index;
        while (ts_query_cursor_next_capture(cursor, &match, &capture_index))
        {
            if (match.pattern_index < (uint32_t)ind_preds.size()
                and not ind_preds[(int)match.pattern_index].empty()
                and not predicates_match(ind_preds[(int)match.pattern_index], match, buffer))
            {
                ts_query_cursor_remove_match(cursor, match.id);
                continue;
            }

            auto& cap = match.captures[capture_index];
            TSPoint start = ts_node_start_point(cap.node);
            TSPoint end = ts_node_end_point(cap.node);

            if (cap.index == indent_capture)
            {
                // Lines inside this node (after the opening line) get +1 indent
                for (uint32_t line = start.row + 1;
                     line <= end.row and (int)line < line_count; ++line)
                    indent_delta[(int)line]++;
            }
            else if (cap.index == outdent_capture)
            {
                // The line with the outdent token gets -1
                if ((int)start.row < line_count)
                    indent_delta[(int)start.row]--;
            }
        }

        // Apply indentation to selected lines
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

        // Apply edits bottom-to-top so earlier replacements don't shift
        // the byte coordinates of later lines.
        ScopedEdition edition(context);
        ScopedSelectionEdition selection_edition{context};

        for (int i = (int)lines_to_indent.size() - 1; i >= 0; --i)
        {
            auto line = lines_to_indent[i];
            int target_level = std::max(0, indent_delta[(int)line]);
            String indent_str(' ', CharCount{target_level * (int)indent_width});

            // Find end of existing leading whitespace
            auto content = buffer[line];
            ByteCount ws_end = 0;
            while (ws_end < content.length() and
                   (content[(int)ws_end] == ' ' or content[(int)ws_end] == '\t'))
                ws_end++;

            buffer.replace(line, {line, ws_end}, indent_str);
        }
    }
};

const CommandDesc tree_indent_newline_cmd = {
    "tree-indent-newline",
    nullptr,
    "tree-indent-newline: auto-indent current line based on AST (for InsertChar hook)",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto* config = syntax_tree.config();
        if (not config or not config->indent_query())
            throw runtime_error("no indent query for this buffer's language");

        TSQuery* query = config->indent_query();
        uint32_t indent_capture = find_capture_index(query, "indent");
        uint32_t outdent_capture = find_capture_index(query, "outdent");

        auto cursor = context.selections().main().cursor();
        auto line = cursor.line;

        // Nothing to indent on the first line
        if (line == 0)
            return;

        auto prev_line = line - 1;

        // Copy previous line's leading whitespace as baseline
        auto prev_content = buffer[prev_line];
        ByteCount prev_ws_end = 0;
        int baseline_spaces = 0;
        while (prev_ws_end < prev_content.length())
        {
            char c = prev_content[(int)prev_ws_end];
            if (c == ' ')
            {
                baseline_spaces++;
                prev_ws_end++;
            }
            else if (c == '\t')
            {
                ColumnCount tabstop = context.options()["tabstop"].get<int>();
                baseline_spaces += (int)tabstop;
                prev_ws_end++;
            }
            else
                break;
        }

        ColumnCount indent_width = context.options()["indentwidth"].get<int>();
        ColumnCount tabstop = context.options()["tabstop"].get<int>();
        if (indent_width == 0)
            indent_width = tabstop;

        // Run indent query over the tree and compute delta for cursor line
        QueryCursorGuard qcursor;
        ts_query_cursor_set_match_limit(qcursor, 256);
        // Restrict query to relevant range (previous line through current line)
        uint32_t prev_row = (uint32_t)(int)prev_line;
        uint32_t cur_row = (uint32_t)(int)line;
        ts_query_cursor_set_point_range(qcursor,
            {prev_row, 0},
            {cur_row + 1, 0});
        ts_query_cursor_exec(qcursor, query, ts_tree_root_node(syntax_tree.tree()));

        const auto& ind_preds = config->indent_predicates();

        int indent_delta = 0;
        TSQueryMatch match;
        uint32_t cap_index;
        while (ts_query_cursor_next_capture(qcursor, &match, &cap_index))
        {
            if (match.pattern_index < (uint32_t)ind_preds.size()
                and not ind_preds[(int)match.pattern_index].empty()
                and not predicates_match(ind_preds[(int)match.pattern_index], match, buffer))
            {
                ts_query_cursor_remove_match(qcursor, match.id);
                continue;
            }

            auto& cap = match.captures[cap_index];
            TSPoint start = ts_node_start_point(cap.node);
            TSPoint end = ts_node_end_point(cap.node);

            if (cap.index == indent_capture)
            {
                // If this @indent node starts on or before prev_line
                // and ends on or after current line, the current line
                // is inside the indent scope -> add one level
                if (start.row <= prev_row and end.row >= cur_row)
                    indent_delta++;
            }
            else if (cap.index == outdent_capture)
            {
                // If an @outdent node starts on the current line,
                // remove one indent level
                if (start.row == cur_row)
                    indent_delta--;
            }
        }

        int target_spaces = std::max(0, baseline_spaces + indent_delta * (int)indent_width);
        String indent_str(' ', CharCount{target_spaces});

        // Replace current line's leading whitespace with computed indent
        auto cur_content = buffer[line];
        ByteCount cur_ws_end = 0;
        while (cur_ws_end < cur_content.length() and
               (cur_content[(int)cur_ws_end] == ' ' or cur_content[(int)cur_ws_end] == '\t'))
            cur_ws_end++;

        ScopedEdition edition(context);
        ScopedSelectionEdition selection_edition{context};
        buffer.replace({line, ByteCount{0}}, {line, cur_ws_end}, indent_str);
    }
};

const CommandDesc tree_sitter_scopes_cmd = {
    "tree-sitter-scopes",
    nullptr,
    "tree-sitter-scopes: show AST ancestor chain from root to cursor node",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto cursor = context.selections().main().cursor();
        TSNode node = find_node_at_cursor(syntax_tree, cursor);
        if (ts_node_is_null(node))
            throw runtime_error("no AST node at cursor");

        Vector<String> chain;
        TSNode walk = node;
        while (not ts_node_is_null(walk))
        {
            chain.push_back(String{ts_node_type(walk)});
            walk = ts_node_parent(walk);
        }

        // Reverse to show root -> leaf, with indentation
        String info;
        for (int i = (int)chain.size() - 1; i >= 0; --i)
        {
            for (int indent = (int)chain.size() - 1 - i; indent > 0; --indent)
                info += "  ";
            info += chain[i];
            if (i > 0) info += "\n";
        }

        if (context.has_client())
            context.client().info_show("Scopes", info, {}, InfoStyle::Prompt);
    }
};

const CommandDesc tree_sitter_highlight_name_cmd = {
    "tree-sitter-highlight-name",
    nullptr,
    "tree-sitter-highlight-name: show node type and all highlight captures at cursor",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto cursor = context.selections().main().cursor();
        TSNode node = find_node_at_cursor(syntax_tree, cursor);
        String node_type = ts_node_is_null(node) ? String{"(none)"} : String{ts_node_type(node)};

        auto* config = syntax_tree.config();
        if (not config or not config->highlight_query())
        {
            String result = format("Node: {}\nCaptures:\n  (no highlight query)", node_type);
            if (context.has_client())
                context.client().info_show("Highlight", result, {}, InfoStyle::Prompt);
            return;
        }

        auto* query = config->highlight_query();
        auto& byte_index = syntax_tree.byte_index();
        uint32_t cursor_byte = byte_index.byte_offset(cursor);

        QueryCursorGuard qcursor;
        ts_query_cursor_set_match_limit(qcursor, 256);
        ts_query_cursor_set_byte_range(qcursor, cursor_byte, cursor_byte + 1);
        ts_query_cursor_exec(qcursor, query, ts_tree_root_node(syntax_tree.tree()));

        const auto& hl_preds = config->highlight_predicates();

        String captures;
        TSQueryMatch match;
        uint32_t capture_index;
        while (ts_query_cursor_next_capture(qcursor, &match, &capture_index))
        {
            if (match.pattern_index < (uint32_t)hl_preds.size()
                and not hl_preds[(int)match.pattern_index].empty()
                and not predicates_match(hl_preds[(int)match.pattern_index], match, buffer))
            {
                ts_query_cursor_remove_match(qcursor, match.id);
                continue;
            }

            auto& cap = match.captures[capture_index];
            uint32_t start = ts_node_start_byte(cap.node);
            uint32_t end = ts_node_end_byte(cap.node);
            if (start <= cursor_byte and cursor_byte < end)
            {
                uint32_t len;
                const char* name = ts_query_capture_name_for_id(query, cap.index, &len);
                auto face_name = capture_to_face_name({name, (ByteCount)len});
                captures += format("  @{} -> {}\n", StringView{name, (ByteCount)len}, face_name);
            }
        }

        String result = format("Node: {}\nCaptures:\n{}", node_type,
                               captures.empty() ? String{"  (none)\n"} : captures);
        // Remove trailing newline
        if (not result.empty() and result.back() == '\n')
            result = String{result.substr(0_byte, result.length() - 1)};

        if (context.has_client())
            context.client().info_show("Highlight", result, {}, InfoStyle::Prompt);
    }
};

const CommandDesc tree_sitter_subtree_cmd = {
    "tree-sitter-subtree",
    nullptr,
    "tree-sitter-subtree: show AST subtree covering selection as S-expression",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto& sel = context.selections().main();
        auto& byte_index = syntax_tree.byte_index();
        uint32_t sel_start = byte_index.byte_offset(sel.min());
        uint32_t sel_end = byte_index.byte_offset(sel.max());
        TSNode node = ts_node_named_descendant_for_byte_range(
            ts_tree_root_node(syntax_tree.tree()), sel_start, sel_end);
        if (ts_node_is_null(node))
            throw runtime_error("no AST node at selection");

        char* sexp = ts_node_string(node);
        String result{sexp};
        free(sexp);

        // Truncate long output at ~60 lines
        constexpr int max_lines = 60;
        int line_count = 0;
        for (ByteCount i = 0; i < result.length(); ++i)
        {
            if (result[i] == '\n' and ++line_count >= max_lines)
            {
                result = String{result.substr(0_byte, i)} + "\n... (truncated)";
                break;
            }
        }

        if (context.has_client())
            context.client().info_show("Subtree", result, {}, InfoStyle::Prompt);
    }
};

const CommandDesc tree_sitter_layers_cmd = {
    "tree-sitter-layers",
    nullptr,
    "tree-sitter-layers: show injection layers at cursor position",
    no_params,
    CommandFlags::None,
    CommandHelper{},
    CommandCompleter{},
    [](const ParametersParser&, Context& context, const ShellContext&)
    {
        auto& buffer = context.buffer();
        auto& syntax_tree = get_syntax_tree(buffer);
        ensure_syntax_tree(buffer, syntax_tree);

        auto cursor = context.selections().main().cursor();
        auto& byte_index = syntax_tree.byte_index();
        uint32_t cursor_byte = byte_index.byte_offset(cursor);

        String result = format("Root: {}\n", syntax_tree.language_name());

        auto layers = syntax_tree.injection_layers();
        String injections;
        for (auto& layer : layers)
        {
            for (auto& range : layer.ranges)
            {
                if (range.start_byte <= cursor_byte and cursor_byte < range.end_byte)
                {
                    injections += format("  {} (bytes {}-{})\n",
                                         layer.language_name,
                                         range.start_byte, range.end_byte);
                    break;  // one entry per layer is enough
                }
            }
        }

        if (injections.empty())
            result += "Injection layers:\n  (none at cursor)";
        else
            result += "Injection layers:\n" + injections;

        // Remove trailing newline
        if (not result.empty() and result.back() == '\n')
            result = String{result.substr(0_byte, result.length() - 1)};

        if (context.has_client())
            context.client().info_show("Layers", result, {}, InfoStyle::Prompt);
    }
};

void register_commands()
{
    CommandManager& cm = CommandManager::instance();
    cm.register_command("nop", [](const ParametersParser&, Context&, const ShellContext&){}, "do nothing",
        {{}, ParameterDesc::Flags::IgnoreUnknownSwitches});

    auto register_command = [&](const CommandDesc& c)
    {
        cm.register_command(c.name, c.func, c.docstring, c.params, c.flags, c.helper, c.completer);
        if (c.alias)
            GlobalScope::instance().aliases().add_alias(c.alias, c.name);
    };

    register_command(edit_cmd);
    register_command(force_edit_cmd);
    register_command(write_cmd);
    register_command(force_write_cmd);
    register_command(write_all_cmd);
    register_command(write_all_quit_cmd);
    register_command(kill_cmd);
    register_command(force_kill_cmd);
    register_command(daemonize_session_cmd);
    register_command(quit_cmd);
    register_command(force_quit_cmd);
    register_command(write_quit_cmd);
    register_command(force_write_quit_cmd);
    register_command(buffer_cmd);
    register_command(buffer_next_cmd);
    register_command(buffer_previous_cmd);
    register_command(delete_buffer_cmd);
    register_command(force_delete_buffer_cmd);
    register_command(rename_buffer_cmd);
    register_command(arrange_buffers_cmd);
    register_command(add_highlighter_cmd);
    register_command(remove_highlighter_cmd);
    register_command(add_hook_cmd);
    register_command(remove_hook_cmd);
    register_command(trigger_user_hook_cmd);
    register_command(define_command_cmd);
    register_command(complete_command_cmd);
    register_command(alias_cmd);
    register_command(unalias_cmd);
    register_command(echo_cmd);
    register_command(debug_cmd);
    register_command(source_cmd);
    register_command(set_option_cmd);
    register_command(unset_option_cmd);
    register_command(update_option_cmd);
    register_command(declare_option_cmd);
    register_command(map_key_cmd);
    register_command(unmap_key_cmd);
    register_command(execute_keys_cmd);
    register_command(evaluate_commands_cmd);
    register_command(prompt_cmd);
    register_command(on_key_cmd);
    register_command(info_cmd);
    register_command(try_catch_cmd);
    register_command(set_face_cmd);
    register_command(unset_face_cmd);
    register_command(rename_client_cmd);
    register_command(set_register_cmd);
    register_command(select_cmd);
    register_command(change_directory_cmd);
    register_command(rename_session_cmd);
    register_command(fail_cmd);
    register_command(declare_user_mode_cmd);
    register_command(enter_user_mode_cmd);
    register_command(provide_module_cmd);
    register_command(require_module_cmd);
    register_command(tree_sitter_enable_cmd);
    register_command(tree_sitter_disable_cmd);
    register_command(tree_select_cmd);
    register_command(tree_select_next_cmd);
    register_command(tree_select_prev_cmd);
    register_command(tree_parent_cmd);
    register_command(tree_first_child_cmd);
    register_command(tree_next_sibling_cmd);
    register_command(tree_prev_sibling_cmd);
    register_command(tree_select_node_cmd);
    register_command(tree_expand_cmd);
    register_command(tree_shrink_cmd);
    register_command(tree_select_all_cmd);
    register_command(tree_filter_cmd);
    register_command(tree_fold_cmd);
    register_command(tree_unfold_cmd);
    register_command(tree_fold_all_cmd);
    register_command(tree_unfold_all_cmd);
    register_command(tree_update_context_cmd);
    register_command(tree_indent_cmd);
    register_command(tree_indent_newline_cmd);
    register_command(tree_sitter_scopes_cmd);
    register_command(tree_sitter_highlight_name_cmd);
    register_command(tree_sitter_subtree_cmd);
    register_command(tree_sitter_layers_cmd);
}

}
