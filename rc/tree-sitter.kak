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
