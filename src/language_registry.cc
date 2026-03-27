#include "language_registry.hh"

#include "buffer.hh"
#include "debug.hh"
#include "exception.hh"
#include "file.hh"
#include "format.hh"
#include "ranges.hh"
#include "regex.hh"

#include <dlfcn.h>
#include <sys/stat.h>

namespace Kakoune
{

String capture_to_face_name(StringView capture_name)
{
    String result = "ts_";
    for (auto c : capture_name)
        result += (c == '.') ? '_' : c;
    return result;
}

LanguageConfig::~LanguageConfig()
{
    if (m_tags_query)
        ts_query_delete(m_tags_query);
    if (m_fold_query)
        ts_query_delete(m_fold_query);
    if (m_locals_query)
        ts_query_delete(m_locals_query);
    if (m_indent_query)
        ts_query_delete(m_indent_query);
    if (m_textobject_query)
        ts_query_delete(m_textobject_query);
    if (m_injection_query)
        ts_query_delete(m_injection_query);
    if (m_highlight_query)
        ts_query_delete(m_highlight_query);
    if (m_grammar_handle)
        dlclose(m_grammar_handle);
}

LanguageConfig::LanguageConfig(LanguageConfig&& other) noexcept
    : m_name(std::move(other.m_name)),
      m_language(other.m_language),
      m_highlight_query(other.m_highlight_query),
      m_capture_faces(std::move(other.m_capture_faces)),
      m_grammar_handle(other.m_grammar_handle),
      m_injection_query(other.m_injection_query),
      m_injection_patterns(std::move(other.m_injection_patterns)),
      m_injection_content_capture(other.m_injection_content_capture),
      m_injection_language_capture(other.m_injection_language_capture),
      m_textobject_query(other.m_textobject_query),
      m_indent_query(other.m_indent_query),
      m_locals_query(other.m_locals_query),
      m_fold_query(other.m_fold_query),
      m_tags_query(other.m_tags_query),
      m_highlight_predicates(std::move(other.m_highlight_predicates)),
      m_injection_predicates(std::move(other.m_injection_predicates)),
      m_textobject_predicates(std::move(other.m_textobject_predicates)),
      m_indent_predicates(std::move(other.m_indent_predicates)),
      m_locals_predicates(std::move(other.m_locals_predicates)),
      m_fold_predicates(std::move(other.m_fold_predicates)),
      m_tags_predicates(std::move(other.m_tags_predicates)),
      m_indent_scopes(std::move(other.m_indent_scopes))
{
    other.m_language = nullptr;
    other.m_highlight_query = nullptr;
    other.m_grammar_handle = nullptr;
    other.m_injection_query = nullptr;
    other.m_injection_content_capture = UINT32_MAX;
    other.m_injection_language_capture = UINT32_MAX;
    other.m_textobject_query = nullptr;
    other.m_indent_query = nullptr;
    other.m_locals_query = nullptr;
    other.m_fold_query = nullptr;
    other.m_tags_query = nullptr;
}

LanguageConfig& LanguageConfig::operator=(LanguageConfig&& other) noexcept
{
    if (this != &other)
    {
        if (m_tags_query)
            ts_query_delete(m_tags_query);
        if (m_fold_query)
            ts_query_delete(m_fold_query);
        if (m_locals_query)
            ts_query_delete(m_locals_query);
        if (m_indent_query)
            ts_query_delete(m_indent_query);
        if (m_textobject_query)
            ts_query_delete(m_textobject_query);
        if (m_injection_query)
            ts_query_delete(m_injection_query);
        if (m_highlight_query)
            ts_query_delete(m_highlight_query);
        if (m_grammar_handle)
            dlclose(m_grammar_handle);

        m_name = std::move(other.m_name);
        m_language = other.m_language;
        m_highlight_query = other.m_highlight_query;
        m_capture_faces = std::move(other.m_capture_faces);
        m_grammar_handle = other.m_grammar_handle;
        m_injection_query = other.m_injection_query;
        m_injection_patterns = std::move(other.m_injection_patterns);
        m_injection_content_capture = other.m_injection_content_capture;
        m_injection_language_capture = other.m_injection_language_capture;
        m_textobject_query = other.m_textobject_query;
        m_indent_query = other.m_indent_query;
        m_locals_query = other.m_locals_query;
        m_fold_query = other.m_fold_query;
        m_tags_query = other.m_tags_query;
        m_highlight_predicates = std::move(other.m_highlight_predicates);
        m_injection_predicates = std::move(other.m_injection_predicates);
        m_textobject_predicates = std::move(other.m_textobject_predicates);
        m_indent_predicates = std::move(other.m_indent_predicates);
        m_locals_predicates = std::move(other.m_locals_predicates);
        m_fold_predicates = std::move(other.m_fold_predicates);
        m_tags_predicates = std::move(other.m_tags_predicates);
        m_indent_scopes = std::move(other.m_indent_scopes);

        other.m_language = nullptr;
        other.m_highlight_query = nullptr;
        other.m_grammar_handle = nullptr;
        other.m_injection_query = nullptr;
        other.m_injection_content_capture = UINT32_MAX;
        other.m_injection_language_capture = UINT32_MAX;
        other.m_textobject_query = nullptr;
        other.m_indent_query = nullptr;
        other.m_locals_query = nullptr;
        other.m_fold_query = nullptr;
        other.m_tags_query = nullptr;
    }
    return *this;
}

LanguageRegistry::LanguageRegistry(String runtime_dir, String config_dir)
    : m_runtime_dir(std::move(runtime_dir))
    , m_config_dir(std::move(config_dir))
{
}

// Resolve a path: check config dir first, fall back to runtime dir
static String resolve_path(const String& config_dir, const String& runtime_dir,
                           StringView relative_path)
{
    if (not config_dir.empty())
    {
        String config_path = format("{}/{}", config_dir, relative_path);
        struct stat st;
        if (stat(config_path.c_str(), &st) == 0)
            return config_path;
    }
    return format("{}/{}", runtime_dir, relative_path);
}

StringView LanguageRegistry::filetype_to_language(StringView filetype)
{
    if (filetype == "sh" or filetype == "bash" or filetype == "zsh")
        return "bash";
    if (filetype == "cpp" or filetype == "objc")
        return "cpp";
    if (filetype == "javascript" or filetype == "jsx")
        return "javascript";
    if (filetype == "typescript" or filetype == "tsx")
        return "tsx";
    return filetype;
}

const LanguageConfig* LanguageRegistry::get(StringView name)
{
    auto it = m_languages.find(name);
    if (it != m_languages.end())
    {
        if (not it->value or it->value->m_language == nullptr)
            return nullptr;
        return it->value.get();
    }
    return load_language(name);
}

static String node_text_for_predicate(TSNode node, const Buffer& buffer)
{
    const TSPoint start = ts_node_start_point(node);
    const TSPoint end = ts_node_end_point(node);

    if (start.row >= (uint32_t)(int)buffer.line_count())
        return {};

    // Fast path: single-line node (common case for identifiers/keywords)
    if (start.row == end.row)
    {
        const StringView line = buffer[LineCount{(int)start.row}];
        auto col_start = ByteCount{(int)start.column};
        auto col_end = ByteCount{(int)end.column};
        if (col_end > line.length())
            col_end = line.length();
        if (col_start >= col_end)
            return {};
        return line.substr(col_start, col_end - col_start).str();
    }

    // Multi-line: concatenate
    String result;
    for (uint32_t row = start.row; row <= end.row
         and row < (uint32_t)(int)buffer.line_count(); ++row)
    {
        const StringView line = buffer[LineCount{(int)row}];
        const auto col_start = ByteCount{(row == start.row) ? (int)start.column : 0};
        const auto col_end = (row == end.row)
            ? ByteCount{(int)end.column}
            : line.length();
        if (col_start < col_end and col_end <= line.length())
            result += line.substr(col_start, col_end - col_start);
    }
    return result;
}

static void parse_single_predicate(const TSQuery* query,
                                   const TSQueryPredicateStep* steps,
                                   uint32_t count,
                                   Vector<QueryPredicate>& out,
                                   HashSet<String>& logged_unknowns)
{
    if (count < 2 or steps[0].type != TSQueryPredicateStepTypeString)
        return;

    uint32_t name_len = 0;
    const char* name_str = ts_query_string_value_for_id(
        query, steps[0].value_id, &name_len);
    StringView name{name_str, (ByteCount)name_len};

    // Helper to get capture ID from a step
    auto get_capture = [](const TSQueryPredicateStep& step) -> Optional<uint32_t> {
        if (step.type == TSQueryPredicateStepTypeCapture)
            return step.value_id;
        return {};
    };

    // Helper to get string value from a step
    auto get_string = [&](const TSQueryPredicateStep& step) -> Optional<String> {
        if (step.type == TSQueryPredicateStepTypeString)
        {
            uint32_t len = 0;
            const char* str = ts_query_string_value_for_id(query, step.value_id, &len);
            return String{str, (ByteCount)len};
        }
        return {};
    };

    if ((name == "eq?" or name == "not-eq?") and count >= 3)
    {
        auto cap = get_capture(steps[1]);
        if (not cap)
            return;

        QueryPredicate pred;
        pred.type = (name == "eq?") ? PredicateType::Eq : PredicateType::NotEq;
        pred.capture_id = *cap;

        // Second arg can be string or capture
        if (steps[2].type == TSQueryPredicateStepTypeString)
            pred.value = *get_string(steps[2]);
        else if (steps[2].type == TSQueryPredicateStepTypeCapture)
            pred.capture_id2 = steps[2].value_id;
        else
            return;

        out.push_back(std::move(pred));
    }
    else if ((name == "match?" or name == "not-match?") and count >= 3)
    {
        auto cap = get_capture(steps[1]);
        auto pattern = get_string(steps[2]);
        if (not cap or not pattern)
            return;

        try
        {
            QueryPredicate pred;
            pred.type = (name == "match?") ? PredicateType::Match : PredicateType::NotMatch;
            pred.capture_id = *cap;
            pred.regex = Regex{*pattern};
            out.push_back(std::move(pred));
        }
        catch (regex_error&)
        {
            write_to_debug_buffer(format("tree-sitter: invalid regex '{}' in predicate, skipping",
                                         *pattern));
        }
    }
    else if ((name == "any-of?" or name == "not-any-of?") and count >= 3)
    {
        auto cap = get_capture(steps[1]);
        if (not cap)
            return;

        QueryPredicate pred;
        pred.type = (name == "any-of?") ? PredicateType::AnyOf : PredicateType::NotAnyOf;
        pred.capture_id = *cap;
        for (uint32_t i = 2; i < count; ++i)
        {
            auto str = get_string(steps[i]);
            if (str)
                pred.values.push_back(std::move(*str));
        }
        out.push_back(std::move(pred));
    }
    else if (name == "not-kind-eq?" and count >= 3)
    {
        auto cap = get_capture(steps[1]);
        auto kind = get_string(steps[2]);
        if (not cap or not kind)
            return;

        QueryPredicate pred;
        pred.type = PredicateType::NotKindEq;
        pred.capture_id = *cap;
        pred.value = std::move(*kind);
        out.push_back(std::move(pred));
    }
    else if ((name == "same-line?" or name == "not-same-line?") and count >= 3)
    {
        auto cap1 = get_capture(steps[1]);
        auto cap2 = get_capture(steps[2]);
        if (not cap1 or not cap2)
            return;

        QueryPredicate pred;
        pred.type = (name == "same-line?") ? PredicateType::SameLine : PredicateType::NotSameLine;
        pred.capture_id = *cap1;
        pred.capture_id2 = *cap2;
        out.push_back(std::move(pred));
    }
    else if ((name == "one-line?" or name == "not-one-line?") and count >= 2)
    {
        auto cap = get_capture(steps[1]);
        if (not cap)
            return;

        QueryPredicate pred;
        pred.type = (name == "one-line?") ? PredicateType::OneLine : PredicateType::NotOneLine;
        pred.capture_id = *cap;
        out.push_back(std::move(pred));
    }
    else if (name != "set!" and name != "is?" and name != "is-not?")
    {
        String name_s{name};
        if (logged_unknowns.find(name_s) == logged_unknowns.end())
        {
            logged_unknowns.insert(name_s);
            write_to_debug_buffer(format("tree-sitter: unknown predicate '{}', skipping", name));
        }
    }
}

PatternPredicates parse_query_predicates(const TSQuery* query)
{
    PatternPredicates result;
    if (not query)
        return result;

    HashSet<String> logged_unknowns;
    uint32_t pattern_count = ts_query_pattern_count(query);
    result.resize((int)pattern_count);

    for (uint32_t p = 0; p < pattern_count; ++p)
    {
        uint32_t step_count = 0;
        const TSQueryPredicateStep* steps =
            ts_query_predicates_for_pattern(query, p, &step_count);

        // Split by Done sentinel into individual predicates
        uint32_t pred_start = 0;
        for (uint32_t s = 0; s <= step_count; ++s)
        {
            if (s == step_count or steps[s].type == TSQueryPredicateStepTypeDone)
            {
                if (s > pred_start)
                    parse_single_predicate(query, steps + pred_start,
                                           s - pred_start, result[(int)p],
                                           logged_unknowns);
                pred_start = s + 1;
            }
        }
    }
    return result;
}

bool predicates_match(const Vector<QueryPredicate>& predicates,
                      const TSQueryMatch& match,
                      const Buffer& buffer,
                      Optional<uint32_t> new_line_byte_pos)
{
    for (const auto& pred : predicates)
    {
        // Find the captured node for this predicate
        TSNode node = {};
        bool found = false;
        for (uint16_t c = 0; c < match.capture_count; ++c)
        {
            if (match.captures[c].index == pred.capture_id)
            {
                node = match.captures[c].node;
                found = true;
                break;
            }
        }
        if (not found)
            return false;

        String text = node_text_for_predicate(node, buffer);

        switch (pred.type)
        {
        case PredicateType::Eq:
            if (pred.capture_id2)
            {
                // capture-vs-capture: find second capture
                bool found2 = false;
                String text2;
                for (uint16_t c = 0; c < match.capture_count; ++c)
                {
                    if (match.captures[c].index == *pred.capture_id2)
                    {
                        text2 = node_text_for_predicate(match.captures[c].node, buffer);
                        found2 = true;
                        break;
                    }
                }
                if (not found2 or text != text2)
                    return false;
            }
            else if (text != pred.value)
                return false;
            break;

        case PredicateType::NotEq:
            if (pred.capture_id2)
            {
                bool found2 = false;
                String text2;
                for (uint16_t c = 0; c < match.capture_count; ++c)
                {
                    if (match.captures[c].index == *pred.capture_id2)
                    {
                        text2 = node_text_for_predicate(match.captures[c].node, buffer);
                        found2 = true;
                        break;
                    }
                }
                if (not found2)
                    return false;  // missing capture -> conservative fail
                if (text == text2)
                    return false;
            }
            else if (text == pred.value)
                return false;
            break;

        case PredicateType::Match:
            if (pred.regex and not regex_search(text.begin(), text.end(),
                                                text.begin(), text.end(), *pred.regex))
                return false;
            break;

        case PredicateType::NotMatch:
            if (pred.regex and regex_search(text.begin(), text.end(),
                                            text.begin(), text.end(), *pred.regex))
                return false;
            break;

        case PredicateType::AnyOf:
        {
            bool in_set = false;
            for (const auto& v : pred.values)
            {
                if (text == v)
                {
                    in_set = true;
                    break;
                }
            }
            if (not in_set)
                return false;
            break;
        }

        case PredicateType::NotAnyOf:
        {
            for (const auto& v : pred.values)
            {
                if (text == v)
                    return false;
            }
            break;
        }

        case PredicateType::NotKindEq:
        {
            const char* kind = ts_node_type(node);
            if (kind and StringView{kind} == pred.value)
                return false;
            break;
        }

        case PredicateType::SameLine:
        case PredicateType::NotSameLine:
        {
            // Find second capture node
            bool found2 = false;
            TSNode node2 = {};
            for (uint16_t c = 0; c < match.capture_count; ++c)
            {
                if (match.captures[c].index == *pred.capture_id2)
                {
                    node2 = match.captures[c].node;
                    found2 = true;
                    break;
                }
            }
            if (not found2)
                return false;  // missing capture -> conservative fail

            // Adjust line numbers for virtual newline insertion
            uint32_t row1 = ts_node_start_point(node).row;
            uint32_t row2 = ts_node_start_point(node2).row;
            if (new_line_byte_pos)
            {
                if (ts_node_start_byte(node) >= *new_line_byte_pos)
                    row1++;
                if (ts_node_start_byte(node2) >= *new_line_byte_pos)
                    row2++;
            }
            bool same = row1 == row2;
            if (pred.type == PredicateType::SameLine and not same)
                return false;
            if (pred.type == PredicateType::NotSameLine and same)
                return false;
            break;
        }

        case PredicateType::OneLine:
        case PredicateType::NotOneLine:
        {
            // Adjust for virtual newline insertion (start uses >=, end uses >)
            uint32_t start_row = ts_node_start_point(node).row;
            uint32_t end_row = ts_node_end_point(node).row;
            if (new_line_byte_pos)
            {
                if (ts_node_start_byte(node) >= *new_line_byte_pos)
                    start_row++;
                if (ts_node_end_byte(node) > *new_line_byte_pos)
                    end_row++;
            }
            bool one = start_row == end_row;
            if (pred.type == PredicateType::OneLine and not one)
                return false;
            if (pred.type == PredicateType::NotOneLine and one)
                return false;
            break;
        }
        }
    }
    return true;
}

// Resolve "; inherits: lang1,lang2" directives by recursively loading
// parent query content. Matches Helix/tree-house regex:
//   ;+\s*inherits\s*:?\s*([a-z_,()-]+)\s*
String LanguageRegistry::resolve_query_inherits(StringView query_text, StringView query_type)
{
    String result;
    for (auto line : query_text | split<StringView>('\n'))
    {
        // Check for inherits directive
        auto it = line.begin();
        while (it != line.end() and *it == ';')
            ++it;
        // Skip whitespace
        while (it != line.end() and (*it == ' ' or *it == '\t'))
            ++it;

        StringView rest{it, line.end()};
        bool is_inherits = false;
        StringView langs;

        if (rest.length() >= 8 and rest.substr(0_byte, 8_byte) == "inherits")
        {
            auto after = rest.begin() + 8;
            // Skip optional colon and whitespace
            while (after != rest.end() and (*after == ':' or *after == ' ' or *after == '\t'))
                ++after;
            if (after != rest.end())
            {
                // Trim trailing whitespace
                auto end = rest.end();
                while (end != after and (*(end-1) == ' ' or *(end-1) == '\t' or *(end-1) == '\n' or *(end-1) == '\r'))
                    --end;
                langs = StringView{after, end};
                is_inherits = true;
            }
        }

        if (is_inherits)
        {
            // Split comma-separated language names and load each
            for (auto parent : langs | split<StringView>(','))
            {
                // Trim whitespace from parent name
                auto pb = parent.begin();
                auto pe = parent.end();
                while (pb != pe and (*pb == ' ' or *pb == '\t')) ++pb;
                while (pe != pb and (*(pe-1) == ' ' or *(pe-1) == '\t')) --pe;
                if (pb == pe) continue;

                StringView parent_name{pb, pe};
                String parent_path = resolve_path(m_config_dir, m_runtime_dir,
                    format("runtime/queries/{}/{}", parent_name, query_type));
                try
                {
                    String parent_text = read_file(parent_path);
                    if (not parent_text.empty())
                    {
                        // Recursively resolve inherits in parent
                        result += resolve_query_inherits(parent_text, query_type);
                        result += '\n';
                    }
                }
                catch (runtime_error&)
                {
                    write_to_debug_buffer(format("tree-sitter: inherits '{}' not found for query type '{}'",
                                                 parent_name, query_type));
                }
            }
        }
        else
        {
            result += line;
            result += '\n';
        }
    }
    return result;
}

const LanguageConfig* LanguageRegistry::load_language(StringView name)
{
    // Try to dlopen the grammar shared library
    String so_path = resolve_path(m_config_dir, m_runtime_dir, format("runtime/grammars/{}.so", name));
    void* handle = dlopen(so_path.c_str(), RTLD_LAZY);

#ifdef __APPLE__
    if (not handle)
    {
        String dylib_path = resolve_path(m_config_dir, m_runtime_dir, format("runtime/grammars/{}.dylib", name));
        handle = dlopen(dylib_path.c_str(), RTLD_LAZY);
    }
#endif

    if (not handle)
    {
        write_to_debug_buffer(format("tree-sitter: failed to load grammar '{}': {}",
                                     name, dlerror()));
        m_languages[name.str()] = make_unique_ptr<LanguageConfig>();
        return nullptr;
    }

    // Look up the grammar function
    String symbol_name = format("tree_sitter_{}", name);
    using GrammarFn = TSLanguage* (*)();
    auto grammar_fn = reinterpret_cast<GrammarFn>(dlsym(handle, symbol_name.c_str()));
    if (not grammar_fn)
    {
        write_to_debug_buffer(format("tree-sitter: symbol '{}' not found in grammar library: {}",
                                     symbol_name, dlerror()));
        dlclose(handle);
        m_languages[name.str()] = make_unique_ptr<LanguageConfig>();
        return nullptr;
    }

    TSLanguage* lang = grammar_fn();
    if (not lang)
    {
        write_to_debug_buffer(format("tree-sitter: grammar function '{}' returned null",
                                     symbol_name));
        dlclose(handle);
        m_languages[name.str()] = make_unique_ptr<LanguageConfig>();
        return nullptr;
    }

    uint32_t abi = ts_language_abi_version(lang);
    if (abi > TREE_SITTER_LANGUAGE_VERSION or
        abi < TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION)
    {
        write_to_debug_buffer(format("tree-sitter: grammar '{}' ABI version {} "
                                     "not compatible (need {}-{})",
                                     name, abi,
                                     TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION,
                                     TREE_SITTER_LANGUAGE_VERSION));
        dlclose(handle);
        m_languages[name.str()] = make_unique_ptr<LanguageConfig>();
        return nullptr;
    }

    // Read the highlights query file
    String query_path = resolve_path(m_config_dir, m_runtime_dir, format("runtime/queries/{}/highlights.scm", name));
    String query_text;
    try
    {
        query_text = resolve_query_inherits(read_file(query_path), "highlights.scm");
    }
    catch (runtime_error& err)
    {
        write_to_debug_buffer(format("tree-sitter: failed to read query file '{}': {}",
                                     query_path, err.what()));
        dlclose(handle);
        m_languages[name.str()] = make_unique_ptr<LanguageConfig>();
        return nullptr;
    }

    // Compile the query
    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    TSQuery* query = ts_query_new(lang, query_text.c_str(),
                                  (uint32_t)(int)query_text.length(),
                                  &error_offset, &error_type);
    if (not query)
    {
        write_to_debug_buffer(format("tree-sitter: query compilation failed for '{}' at offset {}, error type {}",
                                     name, error_offset, (int)error_type));
        dlclose(handle);
        m_languages[name.str()] = make_unique_ptr<LanguageConfig>();
        return nullptr;
    }

    // Build capture_faces vector
    uint32_t capture_count = ts_query_capture_count(query);
    Vector<String> faces;
    faces.reserve(capture_count);
    for (uint32_t i = 0; i < capture_count; ++i)
    {
        uint32_t length = 0;
        const char* capture_name = ts_query_capture_name_for_id(query, i, &length);
        faces.push_back(capture_to_face_name({capture_name, (ByteCount)length}));
    }

    // Store the config
    LanguageConfig config;
    config.m_name = name.str();
    config.m_language = lang;
    config.m_highlight_query = query;
    config.m_capture_faces = std::move(faces);
    config.m_grammar_handle = handle;
    config.m_highlight_predicates = parse_query_predicates(config.m_highlight_query);

    // Try to load injections.scm (optional)
    String inj_path = resolve_path(m_config_dir, m_runtime_dir, format("runtime/queries/{}/injections.scm", name));
    try
    {
        String injection_text = resolve_query_inherits(read_file(inj_path), "injections.scm");
        if (not injection_text.empty())
        {
            uint32_t inj_error_offset = 0;
            TSQueryError inj_error_type = TSQueryErrorNone;
            TSQuery* inj_query = ts_query_new(lang, injection_text.c_str(),
                                               (uint32_t)(int)injection_text.length(),
                                               &inj_error_offset, &inj_error_type);
            if (inj_query)
            {
                config.m_injection_query = inj_query;

                // Find capture indices for @injection.content and @injection.language
                uint32_t cap_count = ts_query_capture_count(inj_query);
                for (uint32_t i = 0; i < cap_count; ++i)
                {
                    uint32_t len = 0;
                    const char* cap_name = ts_query_capture_name_for_id(inj_query, i, &len);
                    StringView cap{cap_name, (ByteCount)len};
                    if (cap == "injection.content")
                        config.m_injection_content_capture = i;
                    else if (cap == "injection.language")
                        config.m_injection_language_capture = i;
                }

                // Extract #set! predicates per pattern
                uint32_t pattern_count = ts_query_pattern_count(inj_query);
                config.m_injection_patterns.resize((int)pattern_count);

                for (uint32_t p = 0; p < pattern_count; ++p)
                {
                    uint32_t step_count = 0;
                    const TSQueryPredicateStep* steps =
                        ts_query_predicates_for_pattern(inj_query, p, &step_count);

                    for (uint32_t s = 0; s < step_count; ++s)
                    {
                        if (steps[s].type != TSQueryPredicateStepTypeString)
                            continue;

                        uint32_t pred_len = 0;
                        const char* pred_name = ts_query_string_value_for_id(
                            inj_query, steps[s].value_id, &pred_len);
                        StringView pred{pred_name, (ByteCount)pred_len};

                        if (pred == "set!" and s + 2 < step_count
                            and steps[s+1].type == TSQueryPredicateStepTypeString)
                        {
                            uint32_t key_len = 0;
                            const char* key_str = ts_query_string_value_for_id(
                                inj_query, steps[s+1].value_id, &key_len);
                            StringView key{key_str, (ByteCount)key_len};

                            if (key == "injection.language"
                                and s + 2 < step_count
                                and steps[s+2].type == TSQueryPredicateStepTypeString)
                            {
                                uint32_t val_len = 0;
                                const char* val_str = ts_query_string_value_for_id(
                                    inj_query, steps[s+2].value_id, &val_len);
                                config.m_injection_patterns[(int)p].language =
                                    String{val_str, (ByteCount)val_len};
                                s += 2;
                            }
                            else if (key == "injection.combined")
                            {
                                config.m_injection_patterns[(int)p].combined = true;
                                s += 1;
                            }
                            else if (key == "injection.include-children")
                            {
                                config.m_injection_patterns[(int)p].include_children = true;
                                s += 1;
                            }
                        }
                    }
                }

                config.m_injection_predicates = parse_query_predicates(config.m_injection_query);

                write_to_debug_buffer(format("tree-sitter: loaded injection query for '{}' with {} patterns",
                                             name, pattern_count));
            }
            else
            {
                write_to_debug_buffer(format("tree-sitter: injection query error in {}/injections.scm at offset {}",
                                             name, inj_error_offset));
            }
        }
    }
    catch (runtime_error&)
    {
        // No injections.scm file — that is fine, not all languages have one
    }

    // Try to load textobjects.scm (optional)
    String textobj_path = resolve_path(m_config_dir, m_runtime_dir, format("runtime/queries/{}/textobjects.scm", name));
    try
    {
        String textobj_text = resolve_query_inherits(read_file(textobj_path), "textobjects.scm");
        if (not textobj_text.empty())
        {
            uint32_t error_offset = 0;
            TSQueryError error_type = TSQueryErrorNone;
            config.m_textobject_query = ts_query_new(lang, textobj_text.c_str(),
                                                      (uint32_t)(int)textobj_text.length(),
                                                      &error_offset, &error_type);
            if (config.m_textobject_query)
            {
                config.m_textobject_predicates = parse_query_predicates(config.m_textobject_query);
                write_to_debug_buffer(format("tree-sitter: loaded textobjects for '{}' with {} captures",
                                             name, ts_query_capture_count(config.m_textobject_query)));
            }
            else
                write_to_debug_buffer(format("tree-sitter: textobject query error in {}/textobjects.scm at offset {} type {}",
                                             name, error_offset, (int)error_type));
        }
    }
    catch (runtime_error&)
    {
        // No textobjects.scm file — that is fine
    }

    // Try to load indents.scm (optional)
    String indent_path = resolve_path(m_config_dir, m_runtime_dir, format("runtime/queries/{}/indents.scm", name));
    try
    {
        String indent_text = resolve_query_inherits(read_file(indent_path), "indents.scm");
        if (not indent_text.empty())
        {
            uint32_t error_offset = 0;
            TSQueryError error_type = TSQueryErrorNone;
            config.m_indent_query = ts_query_new(lang, indent_text.c_str(),
                                                  (uint32_t)(int)indent_text.length(),
                                                  &error_offset, &error_type);
            if (config.m_indent_query)
            {
                config.m_indent_predicates = parse_query_predicates(config.m_indent_query);

                // Parse #set! "scope" for indent patterns
                uint32_t pattern_count = ts_query_pattern_count(config.m_indent_query);
                config.m_indent_scopes.resize((int)pattern_count);  // default: empty Optional (no override)

                for (uint32_t p = 0; p < pattern_count; ++p)
                {
                    uint32_t step_count = 0;
                    const TSQueryPredicateStep* steps =
                        ts_query_predicates_for_pattern(config.m_indent_query, p, &step_count);

                    for (uint32_t s = 0; s < step_count; ++s)
                    {
                        if (steps[s].type != TSQueryPredicateStepTypeString)
                            continue;

                        uint32_t pred_len = 0;
                        const char* pred_name = ts_query_string_value_for_id(
                            config.m_indent_query, steps[s].value_id, &pred_len);
                        StringView pred{pred_name, (ByteCount)pred_len};

                        if (pred == "set!" and s + 2 < step_count
                            and steps[s+1].type == TSQueryPredicateStepTypeString)
                        {
                            uint32_t key_len = 0;
                            const char* key_str = ts_query_string_value_for_id(
                                config.m_indent_query, steps[s+1].value_id, &key_len);
                            StringView key{key_str, (ByteCount)key_len};

                            if (key == "scope" and s + 2 < step_count
                                and steps[s+2].type == TSQueryPredicateStepTypeString)
                            {
                                uint32_t val_len = 0;
                                const char* val_str = ts_query_string_value_for_id(
                                    config.m_indent_query, steps[s+2].value_id, &val_len);
                                StringView val{val_str, (ByteCount)val_len};

                                if (val == "all")
                                    config.m_indent_scopes[(int)p] = IndentScope::All;
                                else if (val == "tail")
                                    config.m_indent_scopes[(int)p] = IndentScope::Tail;

                                s += 2;
                            }
                        }
                    }
                }
            }
            else
                write_to_debug_buffer(format("tree-sitter: indent query error in {}/indents.scm at offset {}",
                                             name, error_offset));
        }
    }
    catch (runtime_error&)
    {
        // No indents.scm file — that is fine, not all languages have one
    }

    // Try to load locals.scm (optional)
    String locals_path = resolve_path(m_config_dir, m_runtime_dir, format("runtime/queries/{}/locals.scm", name));
    try
    {
        String locals_text = resolve_query_inherits(read_file(locals_path), "locals.scm");
        if (not locals_text.empty())
        {
            uint32_t error_offset = 0;
            TSQueryError error_type = TSQueryErrorNone;
            config.m_locals_query = ts_query_new(lang, locals_text.c_str(),
                                                  (uint32_t)(int)locals_text.length(),
                                                  &error_offset, &error_type);
            if (config.m_locals_query)
            {
                config.m_locals_predicates = parse_query_predicates(config.m_locals_query);
                write_to_debug_buffer(format("tree-sitter: loaded locals for '{}' with {} captures",
                                             name, ts_query_capture_count(config.m_locals_query)));
            }
            else
                write_to_debug_buffer(format("tree-sitter: locals query error in {}/locals.scm at offset {} type {}",
                                             name, error_offset, (int)error_type));
        }
    }
    catch (runtime_error&)
    {
        // No locals.scm file — that is fine, not all languages have one
    }

    // Load folds query (optional)
    String folds_path = resolve_path(m_config_dir, m_runtime_dir, format("runtime/queries/{}/folds.scm", name));
    try
    {
        String folds_text = resolve_query_inherits(read_file(folds_path), "folds.scm");
        if (not folds_text.empty())
        {
            uint32_t error_offset = 0;
            TSQueryError error_type = TSQueryErrorNone;
            config.m_fold_query = ts_query_new(lang, folds_text.c_str(),
                                               (uint32_t)(int)folds_text.length(),
                                               &error_offset, &error_type);
            if (config.m_fold_query)
            {
                config.m_fold_predicates = parse_query_predicates(config.m_fold_query);
                write_to_debug_buffer(format("tree-sitter: loaded folds for '{}' with {} captures",
                                             name, ts_query_capture_count(config.m_fold_query)));
            }
            else
                write_to_debug_buffer(format("tree-sitter: folds query error in {}/folds.scm at offset {} type {}",
                                             name, error_offset, (int)error_type));
        }
    }
    catch (runtime_error&)
    {
        // No folds.scm file — that is fine
    }

    // Load tags query (optional)
    String tags_path = resolve_path(m_config_dir, m_runtime_dir, format("runtime/queries/{}/tags.scm", name));
    try
    {
        String tags_text = resolve_query_inherits(read_file(tags_path), "tags.scm");
        if (not tags_text.empty())
        {
            uint32_t error_offset = 0;
            TSQueryError error_type = TSQueryErrorNone;
            config.m_tags_query = ts_query_new(lang, tags_text.c_str(),
                                               (uint32_t)(int)tags_text.length(),
                                               &error_offset, &error_type);
            if (config.m_tags_query)
            {
                config.m_tags_predicates = parse_query_predicates(config.m_tags_query);
                write_to_debug_buffer(format("tree-sitter: loaded tags for '{}' with {} captures",
                                             name, ts_query_capture_count(config.m_tags_query)));
            }
            else
                write_to_debug_buffer(format("tree-sitter: tags query error in {}/tags.scm at offset {} type {}",
                                             name, error_offset, (int)error_type));
        }
    }
    catch (runtime_error&)
    {
        // No tags.scm file — that is fine
    }

    auto ptr = make_unique_ptr<LanguageConfig>(std::move(config));
    auto* raw = ptr.get();
    m_languages[name.str()] = std::move(ptr);

    write_to_debug_buffer(format("tree-sitter: loaded grammar '{}' with {} captures",
                                 name, capture_count));
    return raw;
}

} // namespace Kakoune
