# --------------------------------
# Treesitter Filetypes
# --------------------------------
# Only patterns NOT already handled by upstream rc/filetype/*.kak detection
# hooks belong here: duplicating an upstream regex with a different filetype
# value makes the winner depend on unsorted autoload order. Filetype names
# that differ from helix grammar names are mapped in
# LanguageRegistry::filetype_to_language (src/language_registry.cc).
# hook global BufCreate .*[.](a(scii)?doc|asc) %{ set-option buffer filetype asciidoc }
hook global BufCreate .*[.](astro) %{ set-option buffer filetype astro }
hook global BufCreate .*[.](csv) %{ set-option buffer filetype csv }
hook global BufCreate .*[.](dart) %{ set-option buffer filetype dart }
hook global BufCreate .*[.](diff|patch) %{ set-option buffer filetype diff }
hook global BufCreate .*(ghostty/config) %{ set-option buffer filetype ghostty }
hook global BufCreate .*[.](gitattributes) %{ set-option buffer filetype gitattributes }
hook global BufCreate '\*git_blame\*' %{ set buffer filetype git_blame }
hook global BufCreate .*[.](go) %{ set-option buffer filetype go }
hook global BufCreate .*[.](html) %{ set-option buffer filetype html }
hook global BufCreate .*[.](hurl) %{ set-option buffer filetype hurl }
hook global BufCreate .*[.](ini) %{ set-option buffer filetype ini }
hook global BufCreate .*[.](jsonc) %{ set-option buffer filetype jsonc }
hook global BufCreate .*[.](kdl) %{ set-option buffer filetype kdl }
hook global BufCreate .*[.](log) %{ set-option buffer filetype log }
hook global BufCreate .*[.](liquid) %{ set-option buffer filetype liquid }
hook global BufCreate .*[.](lua) %{ set-option buffer filetype lua }
hook global BufCreate (.*[.](markdown|md|mkd|Rmd)|\*aichat\*) %{ set-option buffer filetype markdown }
hook global BufCreate .*[.](nu) %{ set-option buffer filetype nu }
hook global BufCreate .*[.](org) %{ set-option buffer filetype org }
hook global BufCreate .*[.](pest) %{ set-option buffer filetype pest }
hook global BufCreate .*(?<!\.blade)\.php$ %{ set-option buffer filetype php }
hook global BufCreate .*\.blade\.php$ %{ set-option buffer filetype blade }
hook global BufCreate .*[.](prisma) %{ set-option buffer filetype prisma }
hook global BufCreate .*(([.](rb))|(irbrc)|(pryrc)|(Brewfile)|(Capfile|[.]cap)|(Gemfile|[.]gemspec)|(Guardfile)|(Rakefile|[.]rake)|(Thorfile|[.]thor)|(Vagrantfile))  %{ set-option buffer filetype ruby }
hook global BufCreate .*[.](slang) %{ set-option buffer filetype slang }
hook global BufCreate .*[.](svelte) %{ set-option buffer filetype svelte }
hook global BufCreate .*[.](swift) %{ set-option buffer filetype swift }
hook global BufCreate .*[.](task) %{ set-option buffer filetype task }
hook global BufCreate .*[.](toml) %{ set-option buffer filetype toml }
hook global BufCreate .*[.](tsx) %{ set-option buffer filetype tsx }
hook global BufCreate .*[.][cm]?(ts) %{ set-option buffer filetype typescript }
hook global BufCreate .*[.](vue) %{ set-option buffer filetype vue }
hook global BufCreate .*[.](xml) %{ set-option buffer filetype xml }

