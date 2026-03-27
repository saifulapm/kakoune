; Inject bash into -shell-script-candidates and -shell-script switch values
((switch
  (percent_string
    content: (string_content) @injection.content)) @_sw
  (#match? @_sw "^-shell-script")
  (#set! injection.language "bash")
  (#set! injection.include-children))

; Inject bash into complete-command shell-script-candidates param
((complete_command
  type: (completion_type) @_type
  param: (argument
    (percent_string
      content: (string_content) @injection.content)))
  (#match? @_type "^shell-script")
  (#set! injection.language "bash")
  (#set! injection.include-children))

; Inject bash into %sh{...} expansion content
((expansion
  type: (expansion_type) @_type
  content: (string_content) @injection.content)
 (#eq? @_type "sh")
 (#set! injection.language "bash"))

; Self-inject kak into %{...} and %exp{...} in argument/evaluate contexts
; (kak_block bodies are parsed natively and don't need injection)
(evaluate_commands
  (argument
    (percent_string
      content: (string_content) @injection.content))
  (#set! injection.language "kak")
  (#set! injection.include-children))

(evaluate_commands
  (argument
    (expansion
      content: (string_content) @injection.content))
  (#set! injection.language "kak")
  (#set! injection.include-children))
