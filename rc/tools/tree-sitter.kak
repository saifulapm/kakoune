# Tree-sitter highlight group faces
# Maps tree-sitter capture names to Kakoune faces
# Naming: @capture.name → ts_capture_name (dots → underscores)

# --- Variables ---
set-face global ts_variable                          variable
set-face global ts_variable_builtin                  builtin
set-face global ts_variable_parameter                variable
set-face global ts_variable_parameter_builtin        builtin
set-face global ts_variable_member                   variable
set-face global ts_variable_other_member             variable
set-face global ts_variable_other_member_private     variable

# --- Constants ---
set-face global ts_constant                          value
set-face global ts_constant_builtin                  builtin
set-face global ts_constant_builtin_boolean          value
set-face global ts_constant_character                value
set-face global ts_constant_character_escape         value
set-face global ts_constant_macro                    meta
set-face global ts_constant_numeric                  value

# --- Numbers (alternative captures) ---
set-face global ts_constant_integer                  value
set-face global ts_constant_float                    value
set-face global ts_number                            value
set-face global ts_number_float                      value
set-face global ts_boolean                           value
set-face global ts_character                         value
set-face global ts_character_special                 value

# --- Modules ---
set-face global ts_module                            module
set-face global ts_module_builtin                    builtin
set-face global ts_namespace                         module
set-face global ts_label                             meta

# --- Strings ---
set-face global ts_string                            string
set-face global ts_string_documentation              string
set-face global ts_string_regexp                     meta
set-face global ts_string_escape                     value
set-face global ts_string_special                    meta
set-face global ts_string_special_symbol             meta
set-face global ts_string_special_url                link
set-face global ts_string_special_path               string

# --- Types ---
set-face global ts_type                              type
set-face global ts_type_builtin                      builtin
set-face global ts_type_definition                   type
set-face global ts_type_parameter                    type
set-face global ts_type_enum                         type
set-face global ts_type_enum_variant                 value

# --- Attributes ---
set-face global ts_attribute                         attribute
set-face global ts_attribute_builtin                 attribute
set-face global ts_property                          variable

# --- Functions ---
set-face global ts_function                          function
set-face global ts_function_builtin                  builtin
set-face global ts_function_call                     function
set-face global ts_function_macro                    function
set-face global ts_function_method                   function
set-face global ts_function_method_call              function
set-face global ts_function_method_private           function
set-face global ts_function_special                  function
set-face global ts_constructor                       function

# --- Operators ---
set-face global ts_operator                          operator

# --- Keywords ---
set-face global ts_keyword                           keyword
set-face global ts_keyword_coroutine                 keyword
set-face global ts_keyword_function                  keyword
set-face global ts_keyword_operator                  operator
set-face global ts_keyword_import                    keyword
set-face global ts_keyword_type                      keyword
set-face global ts_keyword_modifier                  keyword
set-face global ts_keyword_repeat                    keyword
set-face global ts_keyword_return                    keyword
set-face global ts_keyword_debug                     keyword
set-face global ts_keyword_exception                 keyword
set-face global ts_keyword_conditional               keyword
set-face global ts_keyword_conditional_ternary       keyword
set-face global ts_keyword_directive                 meta
set-face global ts_keyword_directive_define          meta
set-face global ts_keyword_storage                   keyword
set-face global ts_keyword_storage_type              type
set-face global ts_keyword_storage_modifier          keyword
set-face global ts_keyword_deprecated                keyword
set-face global ts_keyword_control                   keyword
set-face global ts_keyword_control_conditional       keyword
set-face global ts_keyword_control_repeat            keyword
set-face global ts_keyword_control_import            keyword
set-face global ts_keyword_control_return            keyword
set-face global ts_keyword_control_exception         keyword

# --- Punctuation ---
set-face global ts_punctuation                       default
set-face global ts_punctuation_delimiter             default
set-face global ts_punctuation_bracket               default
set-face global ts_punctuation_special               meta

# --- Comments ---
set-face global ts_comment                           comment
set-face global ts_comment_line                      comment
set-face global ts_comment_line_documentation        documentation
set-face global ts_comment_block                     comment
set-face global ts_comment_block_documentation       documentation
set-face global ts_comment_unused                    comment
set-face global ts_comment_documentation             documentation
set-face global ts_comment_error                     Error
set-face global ts_comment_warning                   Error
set-face global ts_comment_todo                      comment
set-face global ts_comment_note                      comment

# --- Markup ---
set-face global ts_markup                            default
set-face global ts_markup_strong                     default
set-face global ts_markup_bold                       default
set-face global ts_markup_italic                     default
set-face global ts_markup_strikethrough              default
set-face global ts_markup_underline                  default
set-face global ts_markup_heading                    title
set-face global ts_markup_heading_marker             title
set-face global ts_markup_heading_1                  title
set-face global ts_markup_heading_2                  title
set-face global ts_markup_heading_3                  title
set-face global ts_markup_heading_4                  title
set-face global ts_markup_heading_5                  title
set-face global ts_markup_heading_6                  title
set-face global ts_markup_quote                      comment
set-face global ts_markup_math                       value
set-face global ts_markup_link                       link
set-face global ts_markup_link_label                 link
set-face global ts_markup_link_url                   link
set-face global ts_markup_link_text                  link
set-face global ts_markup_raw                        mono
set-face global ts_markup_raw_block                  mono
set-face global ts_markup_raw_inline                 mono
set-face global ts_markup_list                       bullet
set-face global ts_markup_list_checked               bullet
set-face global ts_markup_list_unchecked             bullet
set-face global ts_markup_list_unnumbered            bullet
set-face global ts_markup_list_numbered              bullet

# --- Diff ---
set-face global ts_diff                              default
set-face global ts_diff_plus                         green
set-face global ts_diff_plus_gutter                  green
set-face global ts_diff_minus                        red
set-face global ts_diff_minus_gutter                 red
set-face global ts_diff_delta                        yellow
set-face global ts_diff_delta_moved                  yellow
set-face global ts_diff_delta_conflict               red
set-face global ts_diff_delta_gutter                 yellow

# --- Tags (HTML/XML) ---
set-face global ts_tag                               keyword
set-face global ts_tag_builtin                       keyword
set-face global ts_tag_attribute                     attribute
set-face global ts_tag_delimiter                     default

# --- Special ---
set-face global ts_text                              default
set-face global ts_text_reference                    link
set-face global ts_none                              default
set-face global ts_special                           meta
set-face global ts_conceal                           default
set-face global ts_embedded                          default

# Rainbow bracket faces (depth-cycled)
set-face global ts_rainbow_1 red
set-face global ts_rainbow_2 green
set-face global ts_rainbow_3 yellow
set-face global ts_rainbow_4 blue
set-face global ts_rainbow_5 magenta
set-face global ts_rainbow_6 cyan

# Fold face — for fold summary lines (inherits from comment, adds italic)
set-face global ts_fold comment

# Grammar installation
define-command tree-sitter-install -params 0..1 \
    -docstring "tree-sitter-install [lang]: install grammar for current buffer or given language" %{
    evaluate-commands %sh{
        lang="${1:-$kak_opt_tree_sitter_lang}"
        source="$kak_opt_tree_sitter_source"
        rev="$kak_opt_tree_sitter_rev"
        subpath="$kak_opt_tree_sitter_subpath"

        if [ -z "$lang" ]; then
            printf 'fail "no tree-sitter grammar defined for this filetype"\n'
            exit
        fi

        # Determine install directory and extension
        config_dir="$kak_config"
        if [ -z "$config_dir" ]; then
            config_dir="${XDG_CONFIG_HOME:-$HOME/.config}/kak"
        fi
        grammar_dir="$config_dir/runtime/grammars"
        mkdir -p "$grammar_dir"

        case "$(uname)" in
            Darwin) ext="dylib" ;;
            *)      ext="so" ;;
        esac

        lib_path="$grammar_dir/$lang.$ext"

        # Check if already installed
        if [ -f "$lib_path" ]; then
            printf 'echo -markup "{Information}grammar '\''%s'\'' already installed"\n' "$lang"
            exit
        fi

        # Need source info
        if [ -z "$source" ] || [ -z "$rev" ]; then
            printf 'fail "no source defined for grammar '\''%s'\''"\n' "$lang"
            exit
        fi

        printf 'echo -markup "{Information}installing grammar '\''%s'\''..."\n' "$lang"
        printf 'nop %%sh{\n'
        printf '    set -e\n'
        printf '    tmpdir=$(mktemp -d)\n'
        printf '    trap "rm -rf $tmpdir" EXIT\n'
        printf '    git init "$tmpdir/repo" >/dev/null 2>&1\n'
        printf '    cd "$tmpdir/repo"\n'
        printf '    git remote add origin "%s" >/dev/null 2>&1\n' "$source"
        printf '    git fetch --depth 1 origin "%s" >/dev/null 2>&1\n' "$rev"
        printf '    git checkout FETCH_HEAD >/dev/null 2>&1\n'
        printf '    srcdir="$tmpdir/repo"\n'
        printf '    if [ -n "%s" ]; then srcdir="$srcdir/%s"; fi\n' "$subpath" "$subpath"
        printf '    srcdir="$srcdir/src"\n'
        printf '    cc_cmd="cc"\n'
        printf '    scanner=""\n'
        printf '    if [ -f "$srcdir/scanner.cc" ]; then\n'
        printf '        c++ -std=c++14 -fPIC -c -I "$srcdir" "$srcdir/scanner.cc" -o "$tmpdir/scanner.o"\n'
        printf '        scanner="$tmpdir/scanner.o"\n'
        printf '    elif [ -f "$srcdir/scanner.c" ]; then\n'
        printf '        cc -std=c11 -fPIC -c -I "$srcdir" "$srcdir/scanner.c" -o "$tmpdir/scanner.o"\n'
        printf '        scanner="$tmpdir/scanner.o"\n'
        printf '    fi\n'
        printf '    cc -std=c11 -shared -fPIC -fno-exceptions -I "$srcdir" \\\n'
        printf '        -o "%s" "$srcdir/parser.c" $scanner\n' "$lib_path"
        printf '}\n'
        printf 'echo -markup "{Information}grammar '\''%s'\'' installed"\n' "$lang"
    }
}

define-command tree-sitter-install-list -docstring "list installed grammars" %{
    info -title "Installed grammars" %sh{
        config_dir="$kak_config"
        if [ -z "$config_dir" ]; then
            config_dir="${XDG_CONFIG_HOME:-$HOME/.config}/kak"
        fi
        grammar_dir="$config_dir/runtime/grammars"
        runtime_dir="$kak_runtime/runtime/grammars"

        {
            ls "$grammar_dir" 2>/dev/null
            ls "$runtime_dir" 2>/dev/null
        } | sed 's/\.\(so\|dylib\)$//' | sort -u | while read -r name; do
            if [ -n "$name" ]; then
                printf '%s\n' "$name"
            fi
        done
    }
}

# Auto-install grammar on buffer enter if not already available
hook -group tree-sitter-auto-install global WinSetOption filetype=.+ %{
    evaluate-commands %sh{
        lang="$kak_opt_tree_sitter_lang"
        if [ -z "$lang" ]; then exit; fi

        config_dir="$kak_config"
        if [ -z "$config_dir" ]; then
            config_dir="${XDG_CONFIG_HOME:-$HOME/.config}/kak"
        fi

        case "$(uname)" in
            Darwin) ext="dylib" ;;
            *)      ext="so" ;;
        esac

        # Check config dir first, then runtime dir
        if [ -f "$config_dir/runtime/grammars/$lang.$ext" ]; then exit; fi
        if [ -f "$kak_runtime/runtime/grammars/$lang.$ext" ]; then exit; fi

        # Not installed — auto-install
        printf 'tree-sitter-install\n'
    }
}

# Code folding support
declare-option range-specs tree_sitter_folds

# Sticky context: shows enclosing function/class when scrolled past
declare-option str tree_context ""

# Auto-enable tree-sitter highlighting on filetype change
hook -group tree-sitter-auto global WinSetOption filetype=.+ %{
    try %{ add-highlighter window/tree-sitter tree-sitter }
    try %{ add-highlighter window/tree-sitter-folds replace-ranges tree_sitter_folds }
}

hook -group tree-sitter-auto global WinSetOption filetype= %{
    try %{ remove-highlighter window/tree-sitter }
    try %{ remove-highlighter window/tree-sitter-folds }
}

# Update sticky context on window display and cursor movement
hook -group tree-sitter-context global NormalIdle .* %{
    try %{ tree-update-context }
}

# Catch windows where filetype was set before this script loaded (autoload order)
# Use a one-shot hook: remove itself after first execution
hook -group tree-sitter-late-init global WinDisplay .* %{
    evaluate-commands %sh{
        if [ -n "$kak_opt_filetype" ]; then
            printf 'try %%{ add-highlighter window/tree-sitter tree-sitter }\n'
            printf 'try %%{ add-highlighter window/tree-sitter-folds replace-ranges tree_sitter_folds }\n'
        fi
    }
    remove-hooks global tree-sitter-late-init
}

# Auto-indent on newline using tree-sitter
hook -group tree-sitter-indent global WinSetOption filetype=.+ %{
    hook -group tree-sitter-indent window InsertChar \n %{
        try %{ tree-indent-newline }
    }
}

hook -group tree-sitter-indent global WinSetOption filetype= %{
    remove-hooks window tree-sitter-indent
}

# Tree-sitter text object key mappings
# Enter with: map global user t ': enter-user-mode tree<ret>'
declare-user-mode tree

map global tree f ': tree-select function around<ret>' -docstring 'function (around)'
map global tree F ': tree-select function inside<ret>' -docstring 'function (inside)'
map global tree c ': tree-select class around<ret>' -docstring 'class (around)'
map global tree C ': tree-select class inside<ret>' -docstring 'class (inside)'
map global tree a ': tree-select parameter around<ret>' -docstring 'parameter (around)'
map global tree A ': tree-select parameter inside<ret>' -docstring 'parameter (inside)'
map global tree o ': tree-select comment around<ret>' -docstring 'comment'
map global tree n ': tree-select-next function<ret>' -docstring 'next function'
map global tree p ': tree-select-prev function<ret>' -docstring 'prev function'
map global tree s ': tree-select-node<ret>' -docstring 'select node'
map global tree P ': tree-parent<ret>' -docstring 'parent'
map global tree i ': tree-first-child<ret>' -docstring 'first child'
map global tree ] ': tree-next-sibling<ret>' -docstring 'next sibling'
map global tree [ ': tree-prev-sibling<ret>' -docstring 'prev sibling'
map global tree e ': tree-expand<ret>' -docstring 'expand selection'
map global tree E ': tree-shrink<ret>' -docstring 'shrink selection'
map global tree '*' ': tree-select-all function<ret>' -docstring 'select all functions'
map global tree '/' ': tree-filter function<ret>' -docstring 'filter to functions'
map global tree z  ': tree-fold<ret>' -docstring 'fold node'
map global tree Z  ': tree-unfold<ret>' -docstring 'unfold at cursor'
map global tree <a-z> ': tree-fold-all function<ret>' -docstring 'fold all functions'
map global tree <a-Z> ': tree-unfold-all<ret>' -docstring 'unfold all'
map global tree '?' ': tree-sitter-scopes<ret>' -docstring 'show scopes at cursor'
map global tree h ': tree-sitter-highlight-name<ret>' -docstring 'show highlight face'
map global tree t ': tree-sitter-subtree<ret>' -docstring 'show AST subtree'
map global tree l ': tree-sitter-layers<ret>' -docstring 'show injection layers'
