# http://asciidoc.org/
# ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾

# Detection
# ‾‾‾‾‾‾‾‾‾

hook global BufCreate .+\.(a(scii)?doc|asc) %{
    set-option buffer filetype asciidoc
}

# Initialization
# ‾‾‾‾‾‾‾‾‾‾‾‾‾‾

hook -group asciidoc-highlight global WinSetOption filetype=asciidoc %{
    require-module asciidoc

    add-highlighter window/asciidoc ref asciidoc
    hook -once -always window WinSetOption filetype=.* %{ remove-highlighter window/asciidoc }
}

provide-module asciidoc %{

# Highlighters
# ‾‾‾‾‾‾‾‾‾‾‾‾

add-highlighter shared/asciidoc group

# Titles and headers (multi-line style)
add-highlighter shared/asciidoc/ regex (\A|\n\n)\N+\n={2,}\h*$ 0:title
add-highlighter shared/asciidoc/ regex (\A|\n\n)\N+\n-{2,}\h*$ 0:header
add-highlighter shared/asciidoc/ regex (\A|\n\n)\N+\n~{2,}\h*$ 0:header
add-highlighter shared/asciidoc/ regex (\A|\n\n)\N+\n\^{2,}\h*$ 0:header

# Titles and headerss (one-line style)
add-highlighter shared/asciidoc/ regex (\A|\n\n)=\h+\N+$ 0:title
add-highlighter shared/asciidoc/ regex (\A|\n\n)={2,}\h+\N+$ 0:header

# Comments
add-highlighter shared/asciidoc/ regex ^//(?:[^\n/]\N*|)$ 0:comment
add-highlighter shared/asciidoc/ regex ^(/{4,}).*?\n(/{4,})$ 0:comment

# List titles
add-highlighter shared/asciidoc/ regex ^\.[^\h\W]\N*$ 0:title

# Bulleted lists
add-highlighter shared/asciidoc/ regex ^\h*(?<bullet>[-\*])\h+\N+$ 0:list bullet:bullet
add-highlighter shared/asciidoc/ regex ^\h*(?<bullet>[-\*]+)\h+\N+(\n\h+[^-\*\n]*)?$ 0:list bullet:bullet

# Delimited blocks
# https://docs.asciidoctor.org/asciidoc/latest/blocks/delimited/
add-highlighter shared/asciidoc/ regex ^(-{4,})\n[^\n\h].*?\n(-{4,})$ 0:block
add-highlighter shared/asciidoc/ regex ^(={4,})\n[^\n\h].*?\n(={4,})$ 0:block
add-highlighter shared/asciidoc/ regex ^(~{4,})\n[^\n\h].*?\n(~{4,})$ 0:block
add-highlighter shared/asciidoc/ regex ^(\*{4,})\n[^\n\h].*?\n(\*{4,})$ 0:block

# Monospaced text
add-highlighter shared/asciidoc/ regex \B(?:\+\N+?\+|`\N+?`)\B 0:mono

# Bolded text
add-highlighter shared/asciidoc/ regex \s\*[^\n\*]+\*\B 0:+b
add-highlighter shared/asciidoc/ regex \h\*[^\n\*]+\*\B 0:+b
add-highlighter shared/asciidoc/ regex \*{2}(?!\h)[^\n\*]+\*{2} 0:+b
add-highlighter shared/asciidoc/ regex \h\*{2}[^\n\*]+\*{2} 0:+b

# Italicized text
# (these are simpler since they aren't able to _also_ be bullet characters.)
add-highlighter shared/asciidoc/ regex \b_\N+?_\b 0:+i
add-highlighter shared/asciidoc/ regex __\N+?__ 0:+i

# Attributes
add-highlighter shared/asciidoc/ regex ^:(?:(?<neg>!?)[-\w]+|[-\w]+(?<neg>!?)): 0:meta neg:operator
add-highlighter shared/asciidoc/ regex [^\\](\{[-\w]+\})[^\\]? 1:meta

# Options
add-highlighter shared/asciidoc/ regex ^\[\N+\]$ 0:operator

# Admonition pargraphs
add-highlighter shared/asciidoc/ regex ^(NOTE|TIP|IMPORTANT|CAUTION|WARNING): 0:block
add-highlighter shared/asciidoc/ regex ^\[(NOTE|TIP|IMPORTANT|CAUTION|WARNING)\]$ 0:block

# Links, inline macros
add-highlighter shared/asciidoc/ regex \b((?:https?|ftp|irc://)[^\h\[]+)\[(\N*)?\] 1:link 2:+i
add-highlighter shared/asciidoc/ regex (link|mailto):(\N+)(?:\[(\N*)\]) 1:keyword 2:link 3:+i
add-highlighter shared/asciidoc/ regex (xref):(\N+)(?:\[(\N*)\]) 1:keyword 2:meta 3:+i
add-highlighter shared/asciidoc/ regex (<<([^\n><]+)>>) 1:link 2:meta

# Commands
# ‾‾‾‾‾‾‾‾

}
