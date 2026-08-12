hook global WinCreate ".*\.tl" %{
  set-option window filetype teal
}

provide-module -override teal %§
  require-module lua
  add-highlighter shared/teal regions
  add-highlighter shared/teal/code default-region group
  add-highlighter shared/teal/code/base ref lua
  add-highlighter shared/teal/code/constant regex '\b([A-Z_]+)\b' 0:value
  add-highlighter shared/teal/code/operators regex '\b(is|as)\b' 0:operator
  add-highlighter shared/teal/code/function_call regex '\b([a-zA-Z_]\w*)\h*(?=[\(\{"''])' 1:function
  add-highlighter shared/teal/code/default_types regex '\b(boolean|number|integer|thread|any)\b' 0:type
  add-highlighter shared/teal/code/keywords regex '\b(enum|record|interface|global|macroexp|metamethod|metatable|where)\b' 0:keyword
  add-highlighter shared/teal/code/type_keyword regex '\b(type)\b\h*h*(?![\(\{\h"''])' 1:keyword

  add-highlighter shared/teal/multiline_string  region -match-capture   '\[(=*)\[' '\](=*)\]' fill string
  add-highlighter shared/teal/multiline_comment region -match-capture '--\[(=*)\[' '\](=*)\]' fill comment
  add-highlighter shared/teal/double_string region '"' (?<!\\)(?:\\\\)*" fill string
  add-highlighter shared/teal/single_string region "'" (?<!\\)(?:\\\\)*' fill string
  add-highlighter shared/teal/comment region '--' $ fill comment

  declare-option str-list tl_static_words                                        \
  'function' 'boolean' 'number' 'integer' 'string' 'nil' 'thread' 'any' 'macroexp' 'enum' 'record' 'interface'     \
  'local' 'global' 'is' 'end' 'metamethod' 'return' 'print' 'table' 'false' 'true' 'self' 'break' 'and' 'or' 'not'   \
  'do' 'else' 'elseif' 'end' 'for' 'function' 'goto' 'if' 'in' 'local' 'repeat' 'return' 'then' 'until' 'while'    \
  'require' 'as' 'where' '__is' '__add' '__sub' '__mul' '__div' '__mod' '__pow' '__unm' '__idiv' '__band' '__bor'  \
  '__bxor' '__bnot' '__shl' '__shr' '__concat' '__len' '__eq' '__lt' '__le' '__index' '__newindex' '__call'      \
  '__tostring' '__tonumber' '__gc' '__close' '__pairs' '__ipairs' 'io' 'debug' 'pairs' 'ipairs' 'assert' 'tostring'  \
  'tonumber' 'os' '_VERSION' '_G' 'math' 'setmetatable' 'getmetatable' 'coroutine' 'pcall' 'xpcall'
§

hook global WinSetOption filetype=teal %§
  require-module teal

  set-option window comment_line '--'
  set-option window comment_block_begin '--[['
  set-option window comment_block_end ']]'

  add-highlighter window/teal ref teal
  hook -once -always window WinSetOption filetype=.* %{ remove-highlighter window/teal }
  set-option window static_words %opt{tl_static_words}

  define-command -hidden teal-trim-indent %[
    # remove trailing whitespaces
    try %[ execute-keys -draft -itersel x s \h+$ <ret> d ]
  ]

  define-command -hidden teal-indent-on-char %[
    evaluate-commands -no-hooks -draft -itersel %[
      # unindent middle and end structures
      try %[ execute-keys -draft \
        <a-h><a-k>^\h*(\b(end|else|elseif|until)\b|[)}])$<ret> \
        :teal-indent-on-new-line<ret> \
        <a-lt>
      ]
    ]
  ]

  define-command -hidden teal-indent-on-new-line %[
    evaluate-commands -no-hooks -draft -itersel %[
      # remove trailing white spaces from previous line
      try %[ execute-keys -draft k : teal-trim-indent <ret> ]
      # preserve previous non-empty line indent
      try %[ execute-keys -draft ,gh<a-?>^\N+$<ret>s\A|.\z<ret>)<a-&> ]
      # add one indentation level if the previous line is not a comment and:
      #   - starts with a block keyword that is not closed on the same line,
      #   - or contains an unclosed function expression,
      #   - or ends with an enclosed '(' or '{'
      try %[ execute-keys -draft \
        , Kx \
        <a-K>\A\h*--<ret> \
        <a-K>\A\N*\b(end|until)\b<ret> \
        <a-k>\A(\h*\b(do|else|elseif|for|(local\h+)?function|if|repeat|while)\b|\N*[({]$|\N*\bfunction\b\h*[(])<ret> \
        <a-:><semicolon><a-gt>
      ]
    ]
  ]

  define-command -hidden teal-insert-on-new-line %[
    evaluate-commands -no-hooks -draft -itersel %[
      # copy -- comment prefix and following white spaces
      try %[ execute-keys -draft kxs^\h*\K--\h*<ret> y gh j x<semicolon> P ]
      # wisely add end structure
      evaluate-commands -save-regs x %[
        # save previous line indent in register x
        try %[ execute-keys -draft kxs^\h+<ret>"xy ] catch %[ set-register x '' ]
        try %[
          # check that starts with a block keyword that is not closed on the same line
          execute-keys -draft \
            kx \
            <a-k>^\h*\b(else|elseif|do|for|(local\h+)?function|if|while)\b|\N\bfunction\b\h*[(]<ret> \
            <a-K>\bend\b<ret>
          # check that the block is empty and is not closed on a different line
          execute-keys -draft <a-a>i <a-K>^\N+\n\N+\n<ret> jx <a-K>^<c-r>x\b(else|elseif|end)\b<ret>
          # auto insert end
          execute-keys -draft o<c-r>xend<esc>
          # auto insert ) for anonymous function
          execute-keys -draft kx<a-k>\([^)\n]*function\b<ret>jjA)<esc>
        ]
      ]
    ]
  ]

  hook window InsertChar .* -group teal-indent teal-indent-on-char
  hook window InsertChar \n -group teal-indent teal-indent-on-new-line
  hook window InsertChar \n -group teal-insert teal-insert-on-new-line
§
