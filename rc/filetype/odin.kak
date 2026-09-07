hook global BufCreate .*\.odin %{
    set-option buffer filetype odin
}

# Initialization
# ‾‾‾‾‾‾‾‾‾‾‾‾‾‾

hook global WinSetOption filetype=odin %{
    require-module odin

    set-option window static_words %opt{odin_static_words}

    # cleanup trailing whitespaces when exiting insert mode
    hook window ModeChange pop:insert:.* -group odin-trim-indent %{ try %{ execute-keys -draft xs^\h+$<ret>d } }
    hook window InsertChar \n -group odin-insert odin-insert-on-new-line
    hook window InsertChar \n -group odin-indent odin-indent-on-new-line
    hook window InsertChar \{ -group odin-indent odin-indent-on-opening-curly-brace
    hook window InsertChar \} -group odin-indent odin-indent-on-closing-curly-brace

    hook -once -always window WinSetOption filetype=.* %{ remove-hooks window odin-.+ }
}

hook -group odin-highlight global WinSetOption filetype=odin %{
    add-highlighter window/odin ref odin
    hook -once -always window WinSetOption filetype=.* %{ remove-highlighter window/odin }
}

provide-module odin %§

add-highlighter shared/odin regions
add-highlighter shared/odin/code default-region group
add-highlighter shared/odin/string region %{(?<!')"} %{(?<!\\)(\\\\)*"} fill string
add-highlighter shared/odin/rawstring region ` ` fill string
add-highlighter shared/odin/code/character regex %{(\b|\B)'((\\.)|[^'\\])'\B} 0:value

add-highlighter shared/odin/comment region -recurse /\* /\* \*/ fill comment
add-highlighter shared/odin/inline_documentation region /// $ fill documentation
add-highlighter shared/odin/line_comment region // $ fill comment

add-highlighter shared/odin/code/ regex "(?<!\w)@\w+\b" 0:meta
add-highlighter shared/odin/code/operator regex "(=|!|#|@|\$|^|\?|\+|-|\*|/|%|%%|&|\||~|&~|<<|>>|&&|\|\||\+=|-=|\*=|/=|%=|%%=|&=|\|=|~=|&~=|<<=|>>=|&&=|\|\|=|\+\+|--|---|==|!=|<|>|<=|>=|:|\.\.|\.\.=|\.\.<|\|\*\*)" 1:operator
add-highlighter shared/odin/code/function_call regex "\b(\w*)\b\h*(?:\[[\w\s\.,]*\])?\h*\(" 1:function

# Commands
# ‾‾‾‾‾‾‾‾

define-command -hidden odin-insert-on-new-line %[
    # copy // comments prefix and following white spaces
    try %{ execute-keys -draft <semicolon><c-s>kx s ^\h*\K/{2,}\h* <ret> y<c-o>P<esc> }
]

define-command -hidden odin-indent-on-new-line %<
	evaluate-commands -draft -itersel %=
        # preserve previous line indent
        try %{ execute-keys -draft <semicolon>K<a-&> }
        # indent after lines ending with { or (
        try %[ execute-keys -draft kx <a-k> [{(]\h*$ <ret> j<a-gt> ]
        # cleanup trailing white spaces on the previous line
        try %{ execute-keys -draft kx s \h+$ <ret>d }
        # align to opening paren of previous line
        try %{ execute-keys -draft [( <a-k> \A\(\N+\n\N*\n?\z <ret> s \A\(\h*.|.\z <ret> '<a-;>' & }
        # indent after a switch's case/default statements
        try %[ execute-keys -draft kx <a-k> ^\h*(case|default).*:$ <ret> j<a-gt> ]
        # indent after keywords
        try %[ execute-keys -draft <semicolon><a-F>)MB <a-k> \A(if|else|while|for|try|catch)\h*\(.*\)\h*\n\h*\n?\z <ret> s \A|.\z <ret> 1<a-&>1<a-,><a-gt> ]
        # deindent closing brace(s) when after cursor
        try %[ execute-keys -draft x <a-k> ^\h*[})] <ret> gh / [})] <ret> m <a-S> 1<a-&> ]
    =
>

define-command -hidden odin-indent-on-opening-curly-brace %[
    # align indent with opening paren when { is entered on a new line after the closing paren
    try %[ execute-keys -draft -itersel h<a-F>)M <a-k> \A\(.*\)\h*\n\h*\{\z <ret> s \A|.\z <ret> 1<a-&> ]
]

define-command -hidden odin-indent-on-closing-curly-brace %[
    # align to opening curly brace when alone on a line
    try %[ execute-keys -itersel -draft <a-h><a-k>^\h+\}$<ret>hms\A|.\z<ret>1<a-&> ]
]

evaluate-commands %sh{
    values='false true nil ---'
    types='bool b8 b16 b32 b64
           int  i8 i16 i32 i64 i128
           uint u8 u16 u32 u64 u128 uintptr
           i16le i32le i64le i128le u16le u32le u64le u128le
           i16be i32be i64be i128be u16be u32be u64be u128be
           f16 f32 f64
           f16le f32le f64le
           f16be f32be f64be
           complex32 complex64 complex128
           quaternion64 quaternion128 quaternion256
           byte rune
           string cstring
           rawptr
           typeid
           any'
    keywords='asm auto_cast bit_field bit_set break case cast context continue defer distinct
              do dynamic else enum fallthrough for foreign if import in map matrix not_in
              or_break or_continue or_else or_return package proc return struct switch
              transmute typeid union using when where'
    attributes=''
    builtins='
      append                                             non_zero_append_elem_fixed_capacity_string
      append_elem                                        non_zero_append_elems
      append_elems                                       non_zero_append_elem_string
      append_elem_string                                 non_zero_append_soa_elem
      append_fixed_capacity_elem                         non_zero_append_soa_elems
      append_fixed_capacity_elems                        non_zero_reserve
      append_fixed_capacity_string                       non_zero_reserve_dynamic_array
      append_nothing                                     non_zero_reserve_soa
      append_nothing_dynamic_array                       non_zero_resize
      append_nothing_fixed_capacity_dynamic_array        non_zero_resize_dynamic_array
      append_nothing_soa                                 non_zero_resize_fixed_capacity_dynamic_array
      append_soa                                         non_zero_resize_soa
      append_soa_elem                                    ordered_remove
      append_soa_elems                                   ordered_remove_dynamic_array
      append_string                                      ordered_remove_fixed_capacity_dynamic_array
      assert                                             ordered_remove_soa
      assert_contextless                                 panic
      assign_at                                          panic_contextless
      assign_at_elem                                     pop
      assign_at_elem_fixed_capacity_dynamic_array        pop_dynamic_array
      assign_at_elems                                    pop_fixed_capacity_dynamic_array
      assign_at_elems_fixed_capacity_dynamic_array       pop_front
      assign_at_elem_string                              pop_front_dynamic_array
      assign_at_elem_string_fixed_capacity_dynamic_array pop_front_fixed_capacity_dynamic_array
      card                                               pop_front_safe
      clear                                              pop_front_safe_dynamic_array
      clear_dynamic_array                                pop_front_safe_fixed_capacity_dynamic_array
      clear_fixed_capacity_dynamic_array                 pop_safe
      clear_map                                          pop_safe_dynamic_array
      clear_soa                                          pop_safe_fixed_capacity_dynamic_array
      clear_soa_dynamic_array                            raw_soa_footer_dynamic_array
      copy                                               raw_soa_footer_slice
      delete                                             remove_range
      delete_cstring                                     remove_range_dynamic_array
      delete_cstring16                                   remove_range_fixed_capacity_dynamic_array
      delete_dynamic_array                               reserve
      delete_key                                         reserve_dynamic_array
      delete_map                                         reserve_map
      delete_slice                                       reserve_soa
      delete_soa                                         resize
      delete_soa_dynamic_array                           resize_dynamic_array
      delete_soa_slice                                   resize_fixed_capacity_dynamic_array
      delete_string                                      resize_soa
      delete_string16                                    shrink
      ensure                                             shrink_dynamic_array
      ensure_contextless                                 shrink_map
      free                                               unimplemented
      free_all                                           unimplemented_contextless
      init_global_temporary_allocator                    unordered_remove
      inject_at                                          unordered_remove_dynamic_array
      inject_at_elem                                     unordered_remove_fixed_capacity_dynamic_array
      inject_at_elem_fixed_capacity_dynamic_array        unordered_remove_soa
      inject_at_elems                                    abs
      inject_at_elems_fixed_capacity_dynamic_array       conj
      inject_at_elem_soa                                 max
      inject_at_elems_soa                                quaternion
      inject_at_elem_string                              swizzle
      inject_at_elem_string_fixed_capacity_dynamic_array align_of
      inject_at_soa                                      expand_values
      make                                               min
      make_aligned                                       raw_data
      make_dynamic_array                                 typeid_of
      make_dynamic_array_len                             cap
      make_dynamic_array_len_cap                         imag
      make_map                                           offset_of
      make_map_cap                                       real
      make_multi_pointer                                 type_info_of
      make_slice                                         clamp
      make_soa                                           jmag
      make_soa_aligned                                   offset_of_by_string
      make_soa_dynamic_array                             size_of
      make_soa_dynamic_array_len                         type_of
      make_soa_dynamic_array_len_cap                     complex
      make_soa_slice                                     kmag
      map_entry                                          offset_of_member
      map_insert                                         soa_unzip
      map_upsert                                         unreachable
      new                                                compress_values
      new_aligned                                        len
      new_clone                                          offset_of_selector
      non_zero_append                                    soa_zip
      non_zero_append_elem
    '
    join() { sep=$2; eval set -- $1; IFS="$sep"; echo "$*"; }

    add_highlighter() { printf "add-highlighter shared/odin/code/ regex %s %s\n" "$1" "$2"; }

    add_word_highlighter() {
      while [ $# -gt 0 ]; do
          words=$1 face=$2; shift 2
          regex="\\b($(join "${words}" '|'))\\b"
          add_highlighter "$regex" "1:$face"
      done
    }

    printf %s\\n "declare-option str-list odin_static_words $(join "${values} ${types} ${keywords} ${attributes} ${modules} ${builtins}" ' ')"

    add_word_highlighter "$values" "value" "$types" "type" "$keywords" "keyword" "$attributes" "attribute" "$builtins" "builtin"
}

§
