# https://github.com/cucumber/gherkin
# https://cucumber.io/docs/gherkin/reference
# ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
#
# Detection
# ‾‾‾‾‾‾‾‾‾
hook global BufCreate '.*\.feature$' %{
    set-option buffer filetype gherkin
}

# Initialization
# ‾‾‾‾‾‾‾‾‾‾‾‾‾‾
hook global WinSetOption filetype=gherkin %{
    require-module gherkin

    evaluate-commands set-option window static_words %opt{gherkin_static_words}

    # cleanup trailing whitespaces when exiting insert mode
    hook window ModeChange pop:insert:.* -group gherkin-trim-indent %{ try %{ execute-keys -draft xs^\h+$<ret>d } }
    hook window InsertChar \n -group gherkin-insert gherkin-insert-on-new-line
    hook window InsertChar \n -group gherkin-indent gherkin-indent-on-new-line

    hook -once -always window WinSetOption filetype=.* %{ remove-hooks window gherkin-.+ }
}

hook -group gherkin-highlight global WinSetOption filetype=gherkin %{
    add-highlighter window/gherkin ref gherkin
    hook -once -always window WinSetOption filetype=.* %{ remove-highlighter window/gherkin }
}

provide-module gherkin %§

# Highlighters
# ‾‾‾‾‾‾‾‾‾‾‾‾
add-highlighter shared/gherkin regions
add-highlighter shared/gherkin/code default-region group
# Double quoted strings
add-highlighter shared/gherkin/double_quote region %{(?<!')"} %{(?<!\\)(\\\\)*"} fill string
# Triple quoted strings
add-highlighter shared/gherkin/triple_quote region %{(?<!')"""} %{(?<!\\)(\\\\)*"""} fill bright-green
# Comments - single line
add-highlighter shared/gherkin/comment_line region %{#} %{$} fill comment
# Tags
add-highlighter shared/gherkin/code/ regex %{\h*(@[\w\d]+(-[\w\d]+)?)\b} 1:meta
# Data table
add-highlighter shared/gherkin/code/ regex %{(?S)^\h*(\|.*\|)$} 1:yellow
# Keywords
add-highlighter shared/gherkin/code/ regex %{\b(Feature|Rule|Examples?|Scenarios?|Outline|Template|Given|When|Then|And|But|\*|Background)\b} 1:keyword

# Completers
# ‾‾‾‾‾‾‾‾‾‾
declare-option -hidden str-list gherkin_static_words 'Feature' 'Rule' 'Example' 'Examples' 'Scenario' 'Outline' 'Template' 'Scenarios' 'Given' 'When' 'Then' 'And' 'But' '*' 'Background'

# Commands
# ‾‾‾‾‾‾‾‾
define-command -hidden gherkin-insert-on-new-line %[
    evaluate-commands -draft -itersel %[
        # copy # comments prefix and following white spaces
        try %{ execute-keys -draft <semicolon><c-s>kx s ^\h*\K#\h* <ret> y<c-o>P<esc> }
    ]
]

define-command -hidden gherkin-indent-on-new-line %[
    evaluate-commands -draft -itersel %[
        # preserve previous line indent
        try %{ execute-keys -draft <semicolon>K<a-&> }
        # cleanup trailing white spaces on the previous line
        try %{ execute-keys -draft kx s \h+$ <ret>d }
    ]
]
§
