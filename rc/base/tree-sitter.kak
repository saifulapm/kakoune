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

# Rainbow bracket faces (depth-cycled)
set-face global ts_rainbow_1 red
set-face global ts_rainbow_2 green
set-face global ts_rainbow_3 yellow
set-face global ts_rainbow_4 blue
set-face global ts_rainbow_5 magenta
set-face global ts_rainbow_6 cyan

# Fold face — for fold summary lines (inherits from comment, adds italic)
set-face global ts_fold comment

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
