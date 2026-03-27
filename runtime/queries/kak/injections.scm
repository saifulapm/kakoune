; Inject bash into -shell-script-candidates and -shell-script switch values
((switch
  (percent_string
    content: (string_content) @injection.content)) @_sw
  (#match? @_sw "^-shell-script")
  (#set! injection.language "bash")
  (#set! injection.include-children))

; Inject bash into %sh{...} expansion content
((expansion
  type: (expansion_type) @_type
  content: (string_content) @injection.content)
 (#eq? @_type "sh")
 (#set! injection.language "bash"))

; Self-inject kak into %{...} block bodies
; (try, hook, define-command, provide-module, prompt, on-key)
(try_statement
  body: (percent_string
    content: (string_content) @injection.content)
  (#set! injection.language "kak")
  (#set! injection.include-children))

(try_statement
  handler: (percent_string
    content: (string_content) @injection.content)
  (#set! injection.language "kak")
  (#set! injection.include-children))

(hook_definition
  body: (percent_string
    content: (string_content) @injection.content)
  (#set! injection.language "kak")
  (#set! injection.include-children))

(define_command
  body: (percent_string
    content: (string_content) @injection.content)
  (#set! injection.language "kak")
  (#set! injection.include-children))

(provide_module
  body: (percent_string
    content: (string_content) @injection.content)
  (#set! injection.language "kak")
  (#set! injection.include-children))

(evaluate_commands
  (argument
    (percent_string
      content: (string_content) @injection.content))
  (#set! injection.language "kak")
  (#set! injection.include-children))

(prompt_command
  body: (percent_string
    content: (string_content) @injection.content)
  (#set! injection.language "kak")
  (#set! injection.include-children))

(on_key_command
  body: (percent_string
    content: (string_content) @injection.content)
  (#set! injection.language "kak")
  (#set! injection.include-children))
