# Tree-sitter grammar definitions
# Auto-generated from Helix languages.toml
# User can override by defining hooks for specific filetypes in kakrc

declare-option -hidden str tree_sitter_lang
declare-option -hidden str tree_sitter_source
declare-option -hidden str tree_sitter_rev
declare-option -hidden str tree_sitter_subpath

hook global BufSetOption filetype=ada %{
    set-option buffer tree_sitter_lang "ada"
    set-option buffer tree_sitter_source "https://github.com/briot/tree-sitter-ada"
    set-option buffer tree_sitter_rev "ba0894efa03beb70780156b91e28c716b7a4764d"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=adl %{
    set-option buffer tree_sitter_lang "adl"
    set-option buffer tree_sitter_source "https://github.com/adl-lang/tree-sitter-adl"
    set-option buffer tree_sitter_rev "2787d04beadfbe154d3f2da6e98dc45a1b134bbf"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=agda %{
    set-option buffer tree_sitter_lang "agda"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-agda"
    set-option buffer tree_sitter_rev "c21c3a0f996363ed17b8ac99d827fe5a4821f217"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=alloy %{
    set-option buffer tree_sitter_lang "alloy"
    set-option buffer tree_sitter_source "https://github.com/mattsre/tree-sitter-alloy"
    set-option buffer tree_sitter_rev "58d462b1cdb077682b130caa324f3822aeb00b8e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=amber %{
    set-option buffer tree_sitter_lang "amber"
    set-option buffer tree_sitter_source "https://github.com/amber-lang/tree-sitter-amber"
    set-option buffer tree_sitter_rev "dc69f42449fd3c807bacefa398263f62e581f755"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=astro %{
    set-option buffer tree_sitter_lang "astro"
    set-option buffer tree_sitter_source "https://github.com/virchau13/tree-sitter-astro"
    set-option buffer tree_sitter_rev "947e93089e60c66e681eba22283f4037841451e7"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=awk %{
    set-option buffer tree_sitter_lang "awk"
    set-option buffer tree_sitter_source "https://github.com/Beaglefoot/tree-sitter-awk"
    set-option buffer tree_sitter_rev "a799bc5da7c2a84bc9a06ba5f3540cf1191e4ee3"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:bash|env|pkgbuild) %{
    set-option buffer tree_sitter_lang "bash"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-bash"
    set-option buffer tree_sitter_rev "487734f87fd87118028a65a4599352fa99c9cde8"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=basic %{
    set-option buffer tree_sitter_lang "basic"
    set-option buffer tree_sitter_source "https://github.com/Ra77a3l3-jar/tree-sitter-basic"
    set-option buffer tree_sitter_rev "a98449c11d6c688b54c1ca132148a62d7e586a2a"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=bass %{
    set-option buffer tree_sitter_lang "bass"
    set-option buffer tree_sitter_source "https://github.com/vito/tree-sitter-bass"
    set-option buffer tree_sitter_rev "501133e260d768ed4e1fd7374912ed5c86d6fd90"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=beancount %{
    set-option buffer tree_sitter_lang "beancount"
    set-option buffer tree_sitter_source "https://github.com/polarmutex/tree-sitter-beancount"
    set-option buffer tree_sitter_rev "f3741a3a68ade59ec894ed84a64673494d2ba8f3"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=bibtex %{
    set-option buffer tree_sitter_lang "bibtex"
    set-option buffer tree_sitter_source "https://github.com/latex-lsp/tree-sitter-bibtex"
    set-option buffer tree_sitter_rev "ccfd77db0ed799b6c22c214fe9d2937f47bc8b34"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=bicep %{
    set-option buffer tree_sitter_lang "bicep"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-bicep"
    set-option buffer tree_sitter_rev "0092c7d1bd6bb22ce0a6f78497d50ea2b87f19c0"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=bitbake %{
    set-option buffer tree_sitter_lang "bitbake"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-bitbake"
    set-option buffer tree_sitter_rev "10bacac929ff36a1e8f4056503fe4f8717b21b94"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=blade %{
    set-option buffer tree_sitter_lang "blade"
    set-option buffer tree_sitter_source "https://github.com/EmranMR/tree-sitter-blade"
    set-option buffer tree_sitter_rev "59ce5b68e288002e3aee6cf5a379bbef21adbe6c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=blueprint %{
    set-option buffer tree_sitter_lang "blueprint"
    set-option buffer tree_sitter_source "https://gitlab.com/gabmus/tree-sitter-blueprint"
    set-option buffer tree_sitter_rev "355ef84ef8a958ac822117b652cf4d49bac16c79"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=c %{
    set-option buffer tree_sitter_lang "c"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-c"
    set-option buffer tree_sitter_rev "7fa1be1b694b6e763686793d97da01f36a0e5c12"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=c-sharp %{
    set-option buffer tree_sitter_lang "c-sharp"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-c-sharp"
    set-option buffer tree_sitter_rev "b5eb5742f6a7e9438bee22ce8026d6b927be2cd7"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=c3 %{
    set-option buffer tree_sitter_lang "c3"
    set-option buffer tree_sitter_source "https://github.com/c3lang/tree-sitter-c3"
    set-option buffer tree_sitter_rev "15d3502510af0a6c888c663ec8d6aca8791216ce"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=caddyfile %{
    set-option buffer tree_sitter_lang "caddyfile"
    set-option buffer tree_sitter_source "https://github.com/caddyserver/tree-sitter-caddyfile"
    set-option buffer tree_sitter_rev "8951716aa2855e02dada302c540a90f277546095"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=cairo %{
    set-option buffer tree_sitter_lang "cairo"
    set-option buffer tree_sitter_source "https://github.com/starkware-libs/tree-sitter-cairo"
    set-option buffer tree_sitter_rev "4c6a25680546761b80a710ead1dd34e76c203125"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=capnp %{
    set-option buffer tree_sitter_lang "capnp"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-capnp"
    set-option buffer tree_sitter_rev "7b0883c03e5edd34ef7bcf703194204299d7099f"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=cel %{
    set-option buffer tree_sitter_lang "cel"
    set-option buffer tree_sitter_source "https://github.com/bufbuild/tree-sitter-cel"
    set-option buffer tree_sitter_rev "9f2b65da14c216df53933748e489db0f11121464"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=chuck %{
    set-option buffer tree_sitter_lang "chuck"
    set-option buffer tree_sitter_source "https://github.com/tymbalodeon/tree-sitter-chuck"
    set-option buffer tree_sitter_rev "2fd18bcd48e7b23ee47f854e639020df90b1270b"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=circom %{
    set-option buffer tree_sitter_lang "circom"
    set-option buffer tree_sitter_source "https://github.com/Decurity/tree-sitter-circom"
    set-option buffer tree_sitter_rev "02150524228b1e6afef96949f2d6b7cc0aaf999e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=clarity %{
    set-option buffer tree_sitter_lang "clarity"
    set-option buffer tree_sitter_source "https://github.com/xlittlerag/tree-sitter-clarity"
    set-option buffer tree_sitter_rev "7fa54825fdd971a1a676f885384f024fe2b7384a"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=clojure %{
    set-option buffer tree_sitter_lang "clojure"
    set-option buffer tree_sitter_source "https://github.com/sogaiu/tree-sitter-clojure"
    set-option buffer tree_sitter_rev "e57c569ae332ca365da623712ae1f50f84daeae2"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=cmake %{
    set-option buffer tree_sitter_lang "cmake"
    set-option buffer tree_sitter_source "https://github.com/uyha/tree-sitter-cmake"
    set-option buffer tree_sitter_rev "6e51463ef3052dd3b328322c22172eda093727ad"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=comment %{
    set-option buffer tree_sitter_lang "comment"
    set-option buffer tree_sitter_source "https://github.com/stsewd/tree-sitter-comment"
    set-option buffer tree_sitter_rev "aefcc2813392eb6ffe509aa0fc8b4e9b57413ee1"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=common-lisp %{
    set-option buffer tree_sitter_lang "commonlisp"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-commonlisp"
    set-option buffer tree_sitter_rev "32323509b3d9fe96607d151c2da2c9009eb13a2f"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=cpon %{
    set-option buffer tree_sitter_lang "cpon"
    set-option buffer tree_sitter_source "https://github.com/fvacek/tree-sitter-cpon"
    set-option buffer tree_sitter_rev "0d01fcdae5a53191df5b1349f9bce053833270e7"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=cpp %{
    set-option buffer tree_sitter_lang "cpp"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-cpp"
    set-option buffer tree_sitter_rev "56455f4245baf4ea4e0881c5169de69d7edd5ae7"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=crystal %{
    set-option buffer tree_sitter_lang "crystal"
    set-option buffer tree_sitter_source "https://github.com/crystal-lang-tools/tree-sitter-crystal"
    set-option buffer tree_sitter_rev "76afc1f53518a2b68b51a5abcde01d268a9cb47c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=css %{
    set-option buffer tree_sitter_lang "css"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-css"
    set-option buffer tree_sitter_rev "6e327db434fec0ee90f006697782e43ec855adf5"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=csv %{
    set-option buffer tree_sitter_lang "csv"
    set-option buffer tree_sitter_source "https://github.com/weartist/rainbow-csv-tree-sitter"
    set-option buffer tree_sitter_rev "d3dbf916446131417e4c2ea9eb8591b23b466d27"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=cue %{
    set-option buffer tree_sitter_lang "cue"
    set-option buffer tree_sitter_source "https://github.com/eonpatapon/tree-sitter-cue"
    set-option buffer tree_sitter_rev "8a5f273bfa281c66354da562f2307c2d394b6c81"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=cylc %{
    set-option buffer tree_sitter_lang "cylc"
    set-option buffer tree_sitter_source "https://github.com/elliotfontaine/tree-sitter-cylc"
    set-option buffer tree_sitter_rev "30dd40d9bf23912e4aefa93eeb4c7090bda3d0f6"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=cython %{
    set-option buffer tree_sitter_lang "cython"
    set-option buffer tree_sitter_source "https://github.com/b0o/tree-sitter-cython"
    set-option buffer tree_sitter_rev "62f44f5e7e41dde03c5f0a05f035e293bcf2bcf8"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=d %{
    set-option buffer tree_sitter_lang "d"
    set-option buffer tree_sitter_source "https://github.com/gdamore/tree-sitter-d"
    set-option buffer tree_sitter_rev "5566f8ce8fc24186fad06170bbb3c8d97c935d74"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=dart %{
    set-option buffer tree_sitter_lang "dart"
    set-option buffer tree_sitter_source "https://github.com/UserNobody14/tree-sitter-dart"
    set-option buffer tree_sitter_rev "e398400a0b785af3cf571f5a57eccab242f0cdf9"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=dbml %{
    set-option buffer tree_sitter_lang "dbml"
    set-option buffer tree_sitter_source "https://github.com/dynamotn/tree-sitter-dbml"
    set-option buffer tree_sitter_rev "2e2fa5640268c33c3d3f27f7e676f631a9c68fd9"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=debian %{
    set-option buffer tree_sitter_lang "debian"
    set-option buffer tree_sitter_source "https://gitlab.com/MggMuggins/tree-sitter-debian"
    set-option buffer tree_sitter_rev "9b3f4b78c45aab8a2f25a5f9e7bbc00995bc3dde"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=devicetree %{
    set-option buffer tree_sitter_lang "devicetree"
    set-option buffer tree_sitter_source "https://github.com/joelspadin/tree-sitter-devicetree"
    set-option buffer tree_sitter_rev "877adbfa0174d25894c40fa75ad52d4515a36368"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=dhall %{
    set-option buffer tree_sitter_lang "dhall"
    set-option buffer tree_sitter_source "https://github.com/jbellerb/tree-sitter-dhall"
    set-option buffer tree_sitter_rev "affb6ee38d629c9296749767ab832d69bb0d9ea8"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=diff %{
    set-option buffer tree_sitter_lang "diff"
    set-option buffer tree_sitter_source "https://github.com/the-mikedavis/tree-sitter-diff"
    set-option buffer tree_sitter_rev "fd74c78fa88a20085dbc7bbeaba066f4d1692b63"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=djot %{
    set-option buffer tree_sitter_lang "djot"
    set-option buffer tree_sitter_source "https://github.com/treeman/tree-sitter-djot"
    set-option buffer tree_sitter_rev "67e6e23ba7be81a4373e0f49e21207bdc32d12a5"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=dockerfile %{
    set-option buffer tree_sitter_lang "dockerfile"
    set-option buffer tree_sitter_source "https://github.com/camdencheek/tree-sitter-dockerfile"
    set-option buffer tree_sitter_rev "087daa20438a6cc01fa5e6fe6906d77c869d19fe"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=dot %{
    set-option buffer tree_sitter_lang "dot"
    set-option buffer tree_sitter_source "https://github.com/rydesun/tree-sitter-dot"
    set-option buffer tree_sitter_rev "917230743aa10f45a408fea2ddb54bbbf5fbe7b7"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=doxyfile %{
    set-option buffer tree_sitter_lang "doxyfile"
    set-option buffer tree_sitter_source "https://github.com/tingerrr/tree-sitter-doxyfile/"
    set-option buffer tree_sitter_rev "18e44c6da639632a4e42264c7193df34be915f34"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=drools %{
    set-option buffer tree_sitter_lang "drools"
    set-option buffer tree_sitter_source "https://github.com/iByteABit256/tree-sitter-drools"
    set-option buffer tree_sitter_rev "f1404d2a3974dfcfd4193246e763edeeb4b399c1"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=dtd %{
    set-option buffer tree_sitter_lang "dtd"
    set-option buffer tree_sitter_source "https://github.com/KMikeeU/tree-sitter-dtd"
    set-option buffer tree_sitter_rev "6116becb02a6b8e9588ef73d300a9ba4622e156f"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=dunstrc %{
    set-option buffer tree_sitter_lang "dunstrc"
    set-option buffer tree_sitter_source "https://github.com/rotmh/tree-sitter-dunstrc"
    set-option buffer tree_sitter_rev "9cb9d5cc51cf5e2a47bb2a0e2f2e519ff11c1431"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=earthfile %{
    set-option buffer tree_sitter_lang "earthfile"
    set-option buffer tree_sitter_source "https://github.com/glehmann/tree-sitter-earthfile"
    set-option buffer tree_sitter_rev "dbfb970a59cd87b628d087eb8e3fbe19c8e20601"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ebnf %{
    set-option buffer tree_sitter_lang "ebnf"
    set-option buffer tree_sitter_source "https://github.com/RubixDev/ebnf/"
    set-option buffer tree_sitter_rev "8e635b0b723c620774dfb8abf382a7f531894b40"
    set-option buffer tree_sitter_subpath "crates/tree-sitter-ebnf"
}

hook global BufSetOption filetype=edoc %{
    set-option buffer tree_sitter_lang "edoc"
    set-option buffer tree_sitter_source "https://github.com/the-mikedavis/tree-sitter-edoc"
    set-option buffer tree_sitter_rev "74774af7b45dd9cefbf9510328fc6ff2374afc50"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=eex %{
    set-option buffer tree_sitter_lang "eex"
    set-option buffer tree_sitter_source "https://github.com/connorlay/tree-sitter-eex"
    set-option buffer tree_sitter_rev "f742f2fe327463335e8671a87c0b9b396905d1d1"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=eiffel %{
    set-option buffer tree_sitter_lang "eiffel"
    set-option buffer tree_sitter_source "https://github.com/imustafin/tree-sitter-eiffel"
    set-option buffer tree_sitter_rev "d934fb44f1d22bb76be6b56a7b2425ab3b1daf8b"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=elisp %{
    set-option buffer tree_sitter_lang "elisp"
    set-option buffer tree_sitter_source "https://github.com/Wilfred/tree-sitter-elisp"
    set-option buffer tree_sitter_rev "e5524fdccf8c22fc726474a910e4ade976dfc7bb"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=elixir %{
    set-option buffer tree_sitter_lang "elixir"
    set-option buffer tree_sitter_source "https://github.com/elixir-lang/tree-sitter-elixir"
    set-option buffer tree_sitter_rev "02a6f7fd4be28dd94ee4dd2ca19cb777053ea74e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=elm %{
    set-option buffer tree_sitter_lang "elm"
    set-option buffer tree_sitter_source "https://github.com/elm-tooling/tree-sitter-elm"
    set-option buffer tree_sitter_rev "df4cb639c01b76bc9ac9cc66788709a6da20002c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=elvish %{
    set-option buffer tree_sitter_lang "elvish"
    set-option buffer tree_sitter_source "https://github.com/ckafi/tree-sitter-elvish"
    set-option buffer tree_sitter_rev "e50787cadd3bc54f6d9c0704493a79078bb8a4e5"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=embedded-perl %{
    set-option buffer tree_sitter_lang "embedded-perl"
    set-option buffer tree_sitter_source "https://github.com/jobindex-open/tree-sitter-embedded-perl"
    set-option buffer tree_sitter_rev "14b9a948030edd99c132b78a3d3dc2c7537a061b"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:ejs|erb) %{
    set-option buffer tree_sitter_lang "embedded-template"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-embedded-template"
    set-option buffer tree_sitter_rev "d21df11b0ecc6fd211dbe11278e92ef67bd17e97"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=erlang %{
    set-option buffer tree_sitter_lang "erlang"
    set-option buffer tree_sitter_source "https://github.com/the-mikedavis/tree-sitter-erlang"
    set-option buffer tree_sitter_rev "33a3e4f1fa77a3e1a2736813f4b27c358f6c0b63"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=esdl %{
    set-option buffer tree_sitter_lang "esdl"
    set-option buffer tree_sitter_source "https://github.com/hongquan/tree-sitter-esdl"
    set-option buffer tree_sitter_rev "c824fe2bbbed6b29e50c694420aa2b377782f80a"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=fennel %{
    set-option buffer tree_sitter_lang "fennel"
    set-option buffer tree_sitter_source "https://github.com/alexmozaidze/tree-sitter-fennel"
    set-option buffer tree_sitter_rev "cfbfa478dc2dbef267ee94ae4323d9c886f45e94"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=fga %{
    set-option buffer tree_sitter_lang "fga"
    set-option buffer tree_sitter_source "https://github.com/matoous/tree-sitter-fga"
    set-option buffer tree_sitter_rev "ce72d1c484ba133a18e966d67be66bce85695451"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=fidl %{
    set-option buffer tree_sitter_lang "fidl"
    set-option buffer tree_sitter_source "https://github.com/google/tree-sitter-fidl"
    set-option buffer tree_sitter_rev "bdbb635a7f5035e424f6173f2f11b9cd79703f8d"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=fish %{
    set-option buffer tree_sitter_lang "fish"
    set-option buffer tree_sitter_source "https://github.com/ram02z/tree-sitter-fish"
    set-option buffer tree_sitter_rev "a78aef9abc395c600c38a037ac779afc7e3cc9e0"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=flatbuffers %{
    set-option buffer tree_sitter_lang "flatbuffers"
    set-option buffer tree_sitter_source "https://github.com/yuanchenxi95/tree-sitter-flatbuffers"
    set-option buffer tree_sitter_rev "95e6f9ef101ea97e870bf6eebc0bd1fdfbaf5490"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=forth %{
    set-option buffer tree_sitter_lang "forth"
    set-option buffer tree_sitter_source "https://github.com/alexanderbrevig/tree-sitter-forth"
    set-option buffer tree_sitter_rev "360ef13f8c609ec6d2e80782af69958b84e36cd0"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=fortran %{
    set-option buffer tree_sitter_lang "fortran"
    set-option buffer tree_sitter_source "https://github.com/stadelmanma/tree-sitter-fortran"
    set-option buffer tree_sitter_rev "2880b7aab4fb7cc618de1ef3d4c6d93b2396c031"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=freebasic %{
    set-option buffer tree_sitter_lang "freebasic"
    set-option buffer tree_sitter_source "https://github.com/Ra77a3l3-jar/tree-sitter-freebasic"
    set-option buffer tree_sitter_rev "dbf696adb4c0b9c020074e75043c90592981ee7f"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=fsharp %{
    set-option buffer tree_sitter_lang "fsharp"
    set-option buffer tree_sitter_source "https://github.com/ionide/tree-sitter-fsharp"
    set-option buffer tree_sitter_rev "996ea9982bd4e490029f84682016b6793940113b"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gas %{
    set-option buffer tree_sitter_lang "gas"
    set-option buffer tree_sitter_source "https://github.com/sirius94/tree-sitter-gas"
    set-option buffer tree_sitter_rev "60f443646b20edee3b7bf18f3a4fb91dc214259a"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gdscript %{
    set-option buffer tree_sitter_lang "gdscript"
    set-option buffer tree_sitter_source "https://github.com/PrestonKnopp/tree-sitter-gdscript"
    set-option buffer tree_sitter_rev "1f1e782fe2600f50ae57b53876505b8282388d77"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gemini %{
    set-option buffer tree_sitter_lang "gemini"
    set-option buffer tree_sitter_source "https://git.sr.ht/~nbsp/tree-sitter-gemini"
    set-option buffer tree_sitter_rev "3cc5e4bdf572d5df4277fc2e54d6299bd59a54b3"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gherkin %{
    set-option buffer tree_sitter_lang "gherkin"
    set-option buffer tree_sitter_source "https://github.com/SamyAB/tree-sitter-gherkin"
    set-option buffer tree_sitter_rev "43873ee8de16476635b48d52c46f5b6407cb5c09"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ghostty %{
    set-option buffer tree_sitter_lang "ghostty"
    set-option buffer tree_sitter_source "https://github.com/bezhermoso/tree-sitter-ghostty"
    set-option buffer tree_sitter_rev "8438a93b44367e962b2ea3a3b6511885bebd196a"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=git-config %{
    set-option buffer tree_sitter_lang "git-config"
    set-option buffer tree_sitter_source "https://github.com/the-mikedavis/tree-sitter-git-config"
    set-option buffer tree_sitter_rev "9c2a1b7894e6d9eedfe99805b829b4ecd871375e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=git-rebase %{
    set-option buffer tree_sitter_lang "git-rebase"
    set-option buffer tree_sitter_source "https://github.com/the-mikedavis/tree-sitter-git-rebase"
    set-option buffer tree_sitter_rev "d8a4207ebbc47bd78bacdf48f883db58283f9fd8"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=git-attributes %{
    set-option buffer tree_sitter_lang "gitattributes"
    set-option buffer tree_sitter_source "https://github.com/mtoohey31/tree-sitter-gitattributes"
    set-option buffer tree_sitter_rev "3dd50808e3096f93dccd5e9dc7dc3dba2eb12dc4"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:git-commit|git-notes) %{
    set-option buffer tree_sitter_lang "gitcommit"
    set-option buffer tree_sitter_source "https://github.com/gbprod/tree-sitter-gitcommit"
    set-option buffer tree_sitter_rev "a716678c0f00645fed1e6f1d0eb221481dbd6f6d"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=git-ignore %{
    set-option buffer tree_sitter_lang "gitignore"
    set-option buffer tree_sitter_source "https://github.com/shunsambongi/tree-sitter-gitignore"
    set-option buffer tree_sitter_rev "f4685bf11ac466dd278449bcfe5fd014e94aa504"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gleam %{
    set-option buffer tree_sitter_lang "gleam"
    set-option buffer tree_sitter_source "https://github.com/gleam-lang/tree-sitter-gleam"
    set-option buffer tree_sitter_rev "dae1551a9911b24f41d876c23f2ab05ece0a9d4c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=glimmer %{
    set-option buffer tree_sitter_lang "glimmer"
    set-option buffer tree_sitter_source "https://github.com/ember-tooling/tree-sitter-glimmer"
    set-option buffer tree_sitter_rev "5dc6d1040e8ff8978ff3680e818d85447bbc10aa"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gjs %{
    set-option buffer tree_sitter_lang "glimmer-javascript"
    set-option buffer tree_sitter_source "https://github.com/ember-tooling/tree-sitter-glimmer-javascript"
    set-option buffer tree_sitter_rev "5cc865a2a0a77cbfaf5062c8fcf2a9919bd54f87"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gts %{
    set-option buffer tree_sitter_lang "glimmer-typescript"
    set-option buffer tree_sitter_source "https://github.com/ember-tooling/tree-sitter-glimmer-typescript"
    set-option buffer tree_sitter_rev "12d98944c1d5077b957cbdb90d663a7c4d50118c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=glsl %{
    set-option buffer tree_sitter_lang "glsl"
    set-option buffer tree_sitter_source "https://github.com/theHamsta/tree-sitter-glsl"
    set-option buffer tree_sitter_rev "24a6c8ef698e4480fecf8340d771fbcb5de8fbb4"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gn %{
    set-option buffer tree_sitter_lang "gn"
    set-option buffer tree_sitter_source "https://github.com/willcassella/tree-sitter-gn"
    set-option buffer tree_sitter_rev "e18d6e36a79b20dafb58f19d407bd38b0e60260e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gnuplot %{
    set-option buffer tree_sitter_lang "gnuplot"
    set-option buffer tree_sitter_source "https://codeberg.org/maribu/tree-sitter-gnuplot"
    set-option buffer tree_sitter_rev "21a3a3929facb964b3592daeb69119294ff84cf2"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=go %{
    set-option buffer tree_sitter_lang "go"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-go"
    set-option buffer tree_sitter_rev "12fe553fdaaa7449f764bc876fd777704d4fb752"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=go-format-string %{
    set-option buffer tree_sitter_lang "go-format-string"
    set-option buffer tree_sitter_source "https://codeberg.org/kpbaks/tree-sitter-go-format-string"
    set-option buffer tree_sitter_rev "06587ea641155db638f46a32c959d68796cd36bb"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=godot-resource %{
    set-option buffer tree_sitter_lang "godot-resource"
    set-option buffer tree_sitter_source "https://github.com/PrestonKnopp/tree-sitter-godot-resource"
    set-option buffer tree_sitter_rev "2ffb90de47417018651fc3b970e5f6b67214dc9d"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gomod %{
    set-option buffer tree_sitter_lang "gomod"
    set-option buffer tree_sitter_source "https://github.com/camdencheek/tree-sitter-go-mod"
    set-option buffer tree_sitter_rev "6efb59652d30e0e9cd5f3b3a669afd6f1a926d3c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:gotmpl|helm) %{
    set-option buffer tree_sitter_lang "gotmpl"
    set-option buffer tree_sitter_source "https://github.com/ngalaiko/tree-sitter-go-template"
    set-option buffer tree_sitter_rev "ca26229bafcd3f37698a2496c2a5efa2f07e86bc"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gowork %{
    set-option buffer tree_sitter_lang "gowork"
    set-option buffer tree_sitter_source "https://github.com/omertuc/tree-sitter-go-work"
    set-option buffer tree_sitter_rev "6dd9dd79fb51e9f2abc829d5e97b15015b6a8ae2"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gpr %{
    set-option buffer tree_sitter_lang "gpr"
    set-option buffer tree_sitter_source "https://github.com/brownts/tree-sitter-gpr"
    set-option buffer tree_sitter_rev "cea857d3c18d1385d1f5b66cd09ea1e44173945c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=graphql %{
    set-option buffer tree_sitter_lang "graphql"
    set-option buffer tree_sitter_source "https://github.com/bkegley/tree-sitter-graphql"
    set-option buffer tree_sitter_rev "5e66e961eee421786bdda8495ed1db045e06b5fe"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=gren %{
    set-option buffer tree_sitter_lang "gren"
    set-option buffer tree_sitter_source "https://github.com/MaeBrooks/tree-sitter-gren"
    set-option buffer tree_sitter_rev "76554f4f2339f5a24eed19c58f2079b51c694152"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=groovy %{
    set-option buffer tree_sitter_lang "groovy"
    set-option buffer tree_sitter_source "https://github.com/murtaza64/tree-sitter-groovy"
    set-option buffer tree_sitter_rev "235009aad0f580211fc12014bb0846c3910130c1"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=hare %{
    set-option buffer tree_sitter_lang "hare"
    set-option buffer tree_sitter_source "https://git.sr.ht/~ecs/tree-sitter-hare"
    set-option buffer tree_sitter_rev "07035a248943575444aa0b893ffe306e1444c0ab"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=haskell %{
    set-option buffer tree_sitter_lang "haskell"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-haskell"
    set-option buffer tree_sitter_rev "0975ef72fc3c47b530309ca93937d7d143523628"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=haskell-literate %{
    set-option buffer tree_sitter_lang "haskell-literate"
    set-option buffer tree_sitter_source "https://github.com/LaurentRDC/tree-sitter-haskell-literate"
    set-option buffer tree_sitter_rev "8ad7bd1b1595f4cc1a4ccc775d4a3c460f43a596"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=haskell-persistent %{
    set-option buffer tree_sitter_lang "haskell-persistent"
    set-option buffer tree_sitter_source "https://github.com/MercuryTechnologies/tree-sitter-haskell-persistent"
    set-option buffer tree_sitter_rev "58a6ccfd56d9f1de8fb9f77e6c42151f8f0d0f3d"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=haxe %{
    set-option buffer tree_sitter_lang "haxe"
    set-option buffer tree_sitter_source "https://github.com/vantreeseba/tree-sitter-haxe"
    set-option buffer tree_sitter_rev "a55f3e2cf1e4449200fd089a80d3af642bcf5f94"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:docker-bake|hcl|tfvars) %{
    set-option buffer tree_sitter_lang "hcl"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-hcl"
    set-option buffer tree_sitter_rev "9e3ec9848f28d26845ba300fd73c740459b83e9b"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=hdl %{
    set-option buffer tree_sitter_lang "hdl"
    set-option buffer tree_sitter_source "https://github.com/quantonganh/tree-sitter-hdl"
    set-option buffer tree_sitter_rev "293902330423b2cd36ab1ec4b6b967163a4ed57b"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=heex %{
    set-option buffer tree_sitter_lang "heex"
    set-option buffer tree_sitter_source "https://github.com/phoenixframework/tree-sitter-heex"
    set-option buffer tree_sitter_rev "f6b83f305a755cd49cf5f6a66b2b789be93dc7b9"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=hocon %{
    set-option buffer tree_sitter_lang "hocon"
    set-option buffer tree_sitter_source "https://github.com/antosha417/tree-sitter-hocon"
    set-option buffer tree_sitter_rev "c390f10519ae69fdb03b3e5764f5592fb6924bcc"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=hoon %{
    set-option buffer tree_sitter_lang "hoon"
    set-option buffer tree_sitter_source "https://github.com/urbit-pilled/tree-sitter-hoon"
    set-option buffer tree_sitter_rev "1d5df35af3e0afe592832a67b9fb3feeeba1f7b6"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=hosts %{
    set-option buffer tree_sitter_lang "hosts"
    set-option buffer tree_sitter_source "https://github.com/ath3/tree-sitter-hosts"
    set-option buffer tree_sitter_rev "301b9379ce7dfc8bdbe2c2699a6887dcb73953f9"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:html|webc) %{
    set-option buffer tree_sitter_lang "html"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-html"
    set-option buffer tree_sitter_rev "cbb91a0ff3621245e890d1c50cc811bffb77a26b"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=htmldjango %{
    set-option buffer tree_sitter_lang "htmldjango"
    set-option buffer tree_sitter_source "https://github.com/interdependence/tree-sitter-htmldjango"
    set-option buffer tree_sitter_rev "3a643167ad9afac5d61e092f08ff5b054576fadf"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=hurl %{
    set-option buffer tree_sitter_lang "hurl"
    set-option buffer tree_sitter_source "https://github.com/pfeiferj/tree-sitter-hurl"
    set-option buffer tree_sitter_rev "1124058cd192e80d80914652a5850a5b1887cc10"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=hyprlang %{
    set-option buffer tree_sitter_lang "hyprlang"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-hyprlang"
    set-option buffer tree_sitter_rev "27af9b74acf89fa6bed4fb8cb8631994fcb2e6f3"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=iex %{
    set-option buffer tree_sitter_lang "iex"
    set-option buffer tree_sitter_source "https://github.com/elixir-lang/tree-sitter-iex"
    set-option buffer tree_sitter_rev "39f20bb51f502e32058684e893c0c0b00bb2332c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:ini|systemd) %{
    set-option buffer tree_sitter_lang "ini"
    set-option buffer tree_sitter_source "https://github.com/justinmk/tree-sitter-ini"
    set-option buffer tree_sitter_rev "32b31863f222bf22eb43b07d4e9be8017e36fb31"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ink %{
    set-option buffer tree_sitter_lang "ink"
    set-option buffer tree_sitter_source "https://github.com/rhizoome/tree-sitter-ink"
    set-option buffer tree_sitter_rev "8486e9b1627b0bc6b2deb9ee8102277a7c1281ac"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=inko %{
    set-option buffer tree_sitter_lang "inko"
    set-option buffer tree_sitter_source "https://github.com/inko-lang/tree-sitter-inko"
    set-option buffer tree_sitter_rev "f58a87ac4dc6a7955c64c9e4408fbd693e804686"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=janet %{
    set-option buffer tree_sitter_lang "janet-simple"
    set-option buffer tree_sitter_source "https://github.com/sogaiu/tree-sitter-janet-simple"
    set-option buffer tree_sitter_rev "51271e260346878e1a1aa6c506ce6a797b7c25e2"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=java %{
    set-option buffer tree_sitter_lang "java"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-java"
    set-option buffer tree_sitter_rev "09d650def6cdf7f479f4b78f595e9ef5b58ce31e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:javascript|jsx) %{
    set-option buffer tree_sitter_lang "javascript"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-javascript"
    set-option buffer tree_sitter_rev "3a837b6f3658ca3618f2022f8707e29739c91364"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:jinja|nunjucks) %{
    set-option buffer tree_sitter_lang "jinja2"
    set-option buffer tree_sitter_source "https://github.com/varpeti/tree-sitter-jinja2"
    set-option buffer tree_sitter_rev "a533cd3c33aea6acb0f9bf9a56f35dcfe6a8eb53"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=jjdescription %{
    set-option buffer tree_sitter_lang "jjdescription"
    set-option buffer tree_sitter_source "https://github.com/kareigu/tree-sitter-jjdescription"
    set-option buffer tree_sitter_rev "1613b8c85b6ead48464d73668f39910dcbb41911"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=jjrevset %{
    set-option buffer tree_sitter_lang "jjrevset"
    set-option buffer tree_sitter_source "https://github.com/bryceberger/tree-sitter-jjrevset"
    set-option buffer tree_sitter_rev "d9af23944b884ec528b505f41d81923bb3136a51"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=jjtemplate %{
    set-option buffer tree_sitter_lang "jjtemplate"
    set-option buffer tree_sitter_source "https://github.com/bryceberger/tree-sitter-jjtemplate"
    set-option buffer tree_sitter_rev "4313eda8ac31c60e550e3ad5841b100a0a686715"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=jq %{
    set-option buffer tree_sitter_lang "jq"
    set-option buffer tree_sitter_source "https://github.com/flurie/tree-sitter-jq"
    set-option buffer tree_sitter_rev "13990f530e8e6709b7978503da9bc8701d366791"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=jsdoc %{
    set-option buffer tree_sitter_lang "jsdoc"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-jsdoc"
    set-option buffer tree_sitter_rev "189a6a4829beb9cdbe837260653b4a3dfb0cc3db"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:json|json-ld|jsonc) %{
    set-option buffer tree_sitter_lang "json"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-json"
    set-option buffer tree_sitter_rev "73076754005a460947cafe8e03a8cf5fa4fa2938"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=json5 %{
    set-option buffer tree_sitter_lang "json5"
    set-option buffer tree_sitter_source "https://github.com/Joakker/tree-sitter-json5"
    set-option buffer tree_sitter_rev "c23f7a9b1ee7d45f516496b1e0e4be067264fa0d"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=jsonnet %{
    set-option buffer tree_sitter_lang "jsonnet"
    set-option buffer tree_sitter_source "https://github.com/sourcegraph/tree-sitter-jsonnet"
    set-option buffer tree_sitter_rev "0475a5017ad7dc84845d1d33187f2321abcb261d"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=julia %{
    set-option buffer tree_sitter_lang "julia"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-julia"
    set-option buffer tree_sitter_rev "e84f10db8eeb8b9807786bfc658808edaa1b4fa2"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=just %{
    set-option buffer tree_sitter_lang "just"
    set-option buffer tree_sitter_source "https://github.com/poliorcetics/tree-sitter-just"
    set-option buffer tree_sitter_rev "b75dace757e5d122d25c1a1a7772cb87b560f829"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=kak %{
    set-option buffer tree_sitter_lang "kak"
    set-option buffer tree_sitter_source "https://github.com/saifulapm/tree-sitter-kak"
    set-option buffer tree_sitter_rev "0ea9aee7972752f86ac6d16e180ada77074ea83f"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=kcl %{
    set-option buffer tree_sitter_lang "kcl"
    set-option buffer tree_sitter_source "https://github.com/KittyCAD/tree-sitter-kcl"
    set-option buffer tree_sitter_rev "8905e0bdbf5870b50bc3f24345f1af27746f42e8"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=kconfig %{
    set-option buffer tree_sitter_lang "kconfig"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-kconfig"
    set-option buffer tree_sitter_rev "9ac99fe4c0c27a35dc6f757cef534c646e944881"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=kdl %{
    set-option buffer tree_sitter_lang "kdl"
    set-option buffer tree_sitter_source "https://github.com/amaanq/tree-sitter-kdl"
    set-option buffer tree_sitter_rev "3ca569b9f9af43593c24f9e7a21f02f43a13bb88"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=klog %{
    set-option buffer tree_sitter_lang "klog"
    set-option buffer tree_sitter_source "https://github.com/Ansimorph/tree-sitter-klog/"
    set-option buffer tree_sitter_rev "0b215fe75bdeb8368546e3cee36aca8c19212d06"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=koka %{
    set-option buffer tree_sitter_lang "koka"
    set-option buffer tree_sitter_source "https://github.com/koka-community/tree-sitter-koka"
    set-option buffer tree_sitter_rev "fd3b482274d6988349ba810ea5740e29153b1baf"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=kotlin %{
    set-option buffer tree_sitter_lang "kotlin"
    set-option buffer tree_sitter_source "https://github.com/fwcd/tree-sitter-kotlin"
    set-option buffer tree_sitter_rev "c4ddea359a7ff4d92360b2efcd6cfce5dc25afe6"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=koto %{
    set-option buffer tree_sitter_lang "koto"
    set-option buffer tree_sitter_source "https://github.com/koto-lang/tree-sitter-koto"
    set-option buffer tree_sitter_rev "633744bca404ae4edb961a3c2d7bc947a987afa4"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=latex %{
    set-option buffer tree_sitter_lang "latex"
    set-option buffer tree_sitter_source "https://github.com/latex-lsp/tree-sitter-latex"
    set-option buffer tree_sitter_rev "8c75e93cd08ccb7ce1ccab22c1fbd6360e3bcea6"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ld %{
    set-option buffer tree_sitter_lang "ld"
    set-option buffer tree_sitter_source "https://github.com/mtoohey31/tree-sitter-ld"
    set-option buffer tree_sitter_rev "0e9695ae0ede47b8744a8e2ad44d4d40c5d4e4c9"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ldif %{
    set-option buffer tree_sitter_lang "ldif"
    set-option buffer tree_sitter_source "https://github.com/kepet19/tree-sitter-ldif"
    set-option buffer tree_sitter_rev "0a917207f65ba3e3acfa9cda16142ee39c4c1aaa"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=lean %{
    set-option buffer tree_sitter_lang "lean"
    set-option buffer tree_sitter_source "https://github.com/Julian/tree-sitter-lean"
    set-option buffer tree_sitter_rev "d98426109258b266e1e92358c5f11716d2e8f638"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ledger %{
    set-option buffer tree_sitter_lang "ledger"
    set-option buffer tree_sitter_source "https://github.com/cbarrete/tree-sitter-ledger"
    set-option buffer tree_sitter_rev "1f864fb2bf6a87fe1b48545cc6adc6d23090adf7"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=less %{
    set-option buffer tree_sitter_lang "less"
    set-option buffer tree_sitter_source "https://github.com/jimliang/tree-sitter-less"
    set-option buffer tree_sitter_rev "e5ae6245f841b5778c79ac93b28fa4f56b679c5d"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=liquid %{
    set-option buffer tree_sitter_lang "liquid"
    set-option buffer tree_sitter_source "https://github.com/saifulapm/shopify-liquid-treesitter"
    set-option buffer tree_sitter_rev "68941733fb8c4f594a33c8c60dad391513193186"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=llvm %{
    set-option buffer tree_sitter_lang "llvm"
    set-option buffer tree_sitter_source "https://github.com/benwilliamgraham/tree-sitter-llvm"
    set-option buffer tree_sitter_rev "c14cb839003348692158b845db9edda201374548"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=llvm-mir %{
    set-option buffer tree_sitter_lang "llvm-mir"
    set-option buffer tree_sitter_source "https://github.com/Flakebi/tree-sitter-llvm-mir"
    set-option buffer tree_sitter_rev "d166ff8c5950f80b0a476956e7a0ad2f27c12505"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=log %{
    set-option buffer tree_sitter_lang "log"
    set-option buffer tree_sitter_source "https://github.com/Tudyx/tree-sitter-log"
    set-option buffer tree_sitter_rev "62cfe307e942af3417171243b599cc7deac5eab9"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=lpf %{
    set-option buffer tree_sitter_lang "lpf"
    set-option buffer tree_sitter_source "https://gitlab.com/TheZoq2/tree-sitter-lpf"
    set-option buffer tree_sitter_rev "db7372e60c722ca7f12ab359e57e6bf7611ab126"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=lua %{
    set-option buffer tree_sitter_lang "lua"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-lua"
    set-option buffer tree_sitter_rev "e40f5b6e6df9c2d1d6d664ff5d346a75d71ee6b2"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=lua-format-string %{
    set-option buffer tree_sitter_lang "lua-format-string"
    set-option buffer tree_sitter_source "https://codeberg.org/kpbaks/tree-sitter-lua-format-string"
    set-option buffer tree_sitter_rev "b667c8cab109df307e1b4d56b0b43f5c4a353533"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=luap %{
    set-option buffer tree_sitter_lang "luap"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-luap"
    set-option buffer tree_sitter_rev "c134aaec6acf4fa95fe4aa0dc9aba3eacdbbe55a"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=luau %{
    set-option buffer tree_sitter_lang "luau"
    set-option buffer tree_sitter_source "https://github.com/polychromatist/tree-sitter-luau"
    set-option buffer tree_sitter_rev "ec187cafba510cddac265329ca7831ec6f3b9955"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=mail %{
    set-option buffer tree_sitter_lang "mail"
    set-option buffer tree_sitter_source "https://codeberg.org/ficd/tree-sitter-mail"
    set-option buffer tree_sitter_rev "5eddbfdbec4c893182c79047499901c196332e78"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=make %{
    set-option buffer tree_sitter_lang "make"
    set-option buffer tree_sitter_source "https://github.com/alemuller/tree-sitter-make"
    set-option buffer tree_sitter_rev "a4b9187417d6be349ee5fd4b6e77b4172c6827dd"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=markdoc %{
    set-option buffer tree_sitter_lang "markdoc"
    set-option buffer tree_sitter_source "https://github.com/markdoc-extra/tree-sitter-markdoc"
    set-option buffer tree_sitter_rev "5ffe71b29e8a3f94823913ea9cea51fcfa7e3bf8"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:markdown|markdown-rustdoc|quarto|rmarkdown) %{
    set-option buffer tree_sitter_lang "markdown"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-markdown"
    set-option buffer tree_sitter_rev "2dfd57f547f06ca5631a80f601e129d73fc8e9f0"
    set-option buffer tree_sitter_subpath "tree-sitter-markdown"
}

hook global BufSetOption filetype=markdown.inline %{
    set-option buffer tree_sitter_lang "markdown_inline"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-markdown"
    set-option buffer tree_sitter_rev "2dfd57f547f06ca5631a80f601e129d73fc8e9f0"
    set-option buffer tree_sitter_subpath "tree-sitter-markdown-inline"
}

hook global BufSetOption filetype=matlab %{
    set-option buffer tree_sitter_lang "matlab"
    set-option buffer tree_sitter_source "https://github.com/acristoffers/tree-sitter-matlab"
    set-option buffer tree_sitter_rev "55d89cb322c9bc49bfd953b2a9d29ff03b072eac"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=mermaid %{
    set-option buffer tree_sitter_lang "mermaid"
    set-option buffer tree_sitter_source "https://github.com/monaqa/tree-sitter-mermaid"
    set-option buffer tree_sitter_rev "d787c66276e7e95899230539f556e8b83ee16f6d"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=meson %{
    set-option buffer tree_sitter_lang "meson"
    set-option buffer tree_sitter_source "https://github.com/staysail/tree-sitter-meson"
    set-option buffer tree_sitter_rev "32a83e8f200c347232fa795636cfe60dde22957a"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=mojo %{
    set-option buffer tree_sitter_lang "mojo"
    set-option buffer tree_sitter_source "https://github.com/lsh/tree-sitter-mojo"
    set-option buffer tree_sitter_rev "3d7c53b8038f9ebbb57cd2e61296180aa5c1cf64"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=move %{
    set-option buffer tree_sitter_lang "move"
    set-option buffer tree_sitter_source "https://github.com/tzakian/tree-sitter-move"
    set-option buffer tree_sitter_rev "8bc0d1692caa8763fef54d48068238d9bf3c0264"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=nasm %{
    set-option buffer tree_sitter_lang "nasm"
    set-option buffer tree_sitter_source "https://github.com/naclsn/tree-sitter-nasm"
    set-option buffer tree_sitter_rev "570f3d7be01fffc751237f4cfcf52d04e20532d1"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=nearley %{
    set-option buffer tree_sitter_lang "nearley"
    set-option buffer tree_sitter_source "https://github.com/mi2ebi/tree-sitter-nearley"
    set-option buffer tree_sitter_rev "12d01113e194c8e83f6341aab8c2a5f21db9cac9"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=nginx %{
    set-option buffer tree_sitter_lang "nginx"
    set-option buffer tree_sitter_source "https://gitlab.com/joncoole/tree-sitter-nginx"
    set-option buffer tree_sitter_rev "b4b61db443602b69410ab469c122c01b1e685aa0"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=nickel %{
    set-option buffer tree_sitter_lang "nickel"
    set-option buffer tree_sitter_source "https://github.com/nickel-lang/tree-sitter-nickel"
    set-option buffer tree_sitter_rev "88d836a24b3b11c8720874a1a9286b8ae838d30a"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=nim %{
    set-option buffer tree_sitter_lang "nim"
    set-option buffer tree_sitter_source "https://github.com/alaviss/tree-sitter-nim"
    set-option buffer tree_sitter_rev "c5f0ce3b65222f5dbb1a12f9fe894524881ad590"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=nix %{
    set-option buffer tree_sitter_lang "nix"
    set-option buffer tree_sitter_source "https://github.com/nix-community/tree-sitter-nix"
    set-option buffer tree_sitter_rev "1b69cf1fa92366eefbe6863c184e5d2ece5f187d"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=nu %{
    set-option buffer tree_sitter_lang "nu"
    set-option buffer tree_sitter_source "https://github.com/nushell/tree-sitter-nu"
    set-option buffer tree_sitter_rev "cc4624fbc6ec3563d98fbe8f215a8b8e10b16f32"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ocaml %{
    set-option buffer tree_sitter_lang "ocaml"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-ocaml"
    set-option buffer tree_sitter_rev "9965d208337d88bbf1a38ad0b0fe49e5f5ec9677"
    set-option buffer tree_sitter_subpath "ocaml"
}

hook global BufSetOption filetype=ocaml-interface %{
    set-option buffer tree_sitter_lang "ocaml-interface"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-ocaml"
    set-option buffer tree_sitter_rev "9965d208337d88bbf1a38ad0b0fe49e5f5ec9677"
    set-option buffer tree_sitter_subpath "interface"
}

hook global BufSetOption filetype=odin %{
    set-option buffer tree_sitter_lang "odin"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-odin"
    set-option buffer tree_sitter_rev "6c6b07e354a52f8f2a9bc776cbc262a74e74fd26"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ohm %{
    set-option buffer tree_sitter_lang "ohm"
    set-option buffer tree_sitter_source "https://github.com/novusnota/tree-sitter-ohm"
    set-option buffer tree_sitter_rev "80f14f0e477ddacc1e137d5ed8e830329e3fb7a3"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=opencl %{
    set-option buffer tree_sitter_lang "opencl"
    set-option buffer tree_sitter_source "https://github.com/lefp/tree-sitter-opencl"
    set-option buffer tree_sitter_rev "8e1d24a57066b3cd1bb9685bbc1ca9de5c1b78fb"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=openscad %{
    set-option buffer tree_sitter_lang "openscad"
    set-option buffer tree_sitter_source "https://github.com/openscad/tree-sitter-openscad"
    set-option buffer tree_sitter_rev "acc196e969a169cadd8b7f8d9f81ff2d30e3e253"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=org %{
    set-option buffer tree_sitter_lang "org"
    set-option buffer tree_sitter_source "https://github.com/milisims/tree-sitter-org"
    set-option buffer tree_sitter_rev "698bb1a34331e68f83fc24bdd1b6f97016bb30de"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=pascal %{
    set-option buffer tree_sitter_lang "pascal"
    set-option buffer tree_sitter_source "https://github.com/Isopod/tree-sitter-pascal"
    set-option buffer tree_sitter_rev "2fd40f477d3e2794af152618ccfac8d92eb72a66"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=passwd %{
    set-option buffer tree_sitter_lang "passwd"
    set-option buffer tree_sitter_source "https://github.com/ath3/tree-sitter-passwd"
    set-option buffer tree_sitter_rev "20239395eacdc2e0923a7e5683ad3605aee7b716"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=pem %{
    set-option buffer tree_sitter_lang "pem"
    set-option buffer tree_sitter_source "https://github.com/mtoohey31/tree-sitter-pem"
    set-option buffer tree_sitter_rev "be67a4330a1aa507c7297bc322204f936ec1132c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=penrose %{
    set-option buffer tree_sitter_lang "penrose"
    set-option buffer tree_sitter_source "https://github.com/klukaszek/tree-sitter-penrose"
    set-option buffer tree_sitter_rev "d9368ff7f743b2ac2145a3342db13b1ad950cba5"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=perl %{
    set-option buffer tree_sitter_lang "perl"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-perl/tree-sitter-perl"
    set-option buffer tree_sitter_rev "72a08a496a23212f23802490ef6f4700d68cfd0e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=pest %{
    set-option buffer tree_sitter_lang "pest"
    set-option buffer tree_sitter_source "https://github.com/pest-parser/tree-sitter-pest"
    set-option buffer tree_sitter_rev "c19629a0c50e6ca2485c3b154b1dde841a08d169"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=php %{
    set-option buffer tree_sitter_lang "php"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-php"
    set-option buffer tree_sitter_rev "f860e598194f4a71747f91789bf536b393ad4a56"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=php-only %{
    set-option buffer tree_sitter_lang "php-only"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-php"
    set-option buffer tree_sitter_rev "cf1f4a0f1c01c705c1d6cf992b104028d5df0b53"
    set-option buffer tree_sitter_subpath "php_only"
}

hook global BufSetOption filetype=picat %{
    set-option buffer tree_sitter_lang "picat"
    set-option buffer tree_sitter_source "https://github.com/dlr-ft/tree-sitter-picat"
    set-option buffer tree_sitter_rev "ecb6b07d280f2cef8214bc4b999ad83ac1d9205c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=pkl %{
    set-option buffer tree_sitter_lang "pkl"
    set-option buffer tree_sitter_source "https://github.com/apple/tree-sitter-pkl"
    set-option buffer tree_sitter_rev "c03f04a313b712f8ab00a2d862c10b37318699ae"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=po %{
    set-option buffer tree_sitter_lang "po"
    set-option buffer tree_sitter_source "https://github.com/erasin/tree-sitter-po"
    set-option buffer tree_sitter_rev "417cee9abb2053ed26b19e7de972398f2da9b29e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=pod %{
    set-option buffer tree_sitter_lang "pod"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-perl/tree-sitter-pod"
    set-option buffer tree_sitter_rev "0bf8387987c21bf2f8ed41d2575a8f22b139687f"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ponylang %{
    set-option buffer tree_sitter_lang "ponylang"
    set-option buffer tree_sitter_source "https://github.com/mfelsche/tree-sitter-ponylang"
    set-option buffer tree_sitter_rev "ef66b151bc2604f431b5668fcec4747db4290e11"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=powershell %{
    set-option buffer tree_sitter_lang "powershell"
    set-option buffer tree_sitter_source "https://github.com/airbus-cert/tree-sitter-powershell"
    set-option buffer tree_sitter_rev "c9316be0faca5d5b9fd3b57350de650755f42dc0"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=prisma %{
    set-option buffer tree_sitter_lang "prisma"
    set-option buffer tree_sitter_source "https://github.com/victorhqc/tree-sitter-prisma"
    set-option buffer tree_sitter_rev "eca2596a355b1a9952b4f80f8f9caed300a272b5"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=prolog %{
    set-option buffer tree_sitter_lang "prolog"
    set-option buffer tree_sitter_source "https://codeberg.org/foxy/tree-sitter-prolog"
    set-option buffer tree_sitter_rev "d8d415f6a1cf80ca138524bcc395810b176d40fa"
    set-option buffer tree_sitter_subpath "grammars/prolog"
}

hook global BufSetOption filetype=properties %{
    set-option buffer tree_sitter_lang "properties"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-properties"
    set-option buffer tree_sitter_rev "579b62f5ad8d96c2bb331f07d1408c92767531d9"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=protobuf %{
    set-option buffer tree_sitter_lang "proto"
    set-option buffer tree_sitter_source "https://github.com/sdoerner/tree-sitter-proto"
    set-option buffer tree_sitter_rev "778ab6ed18a7fcf82c83805a87d63376c51e80bc"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=proverif %{
    set-option buffer tree_sitter_lang "proverif"
    set-option buffer tree_sitter_source "https://codeberg.org/maribu/tree-sitter-proverif"
    set-option buffer tree_sitter_rev "7741807092c4009c1fe4c3648da60ca72b1b80f1"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=prql %{
    set-option buffer tree_sitter_lang "prql"
    set-option buffer tree_sitter_source "https://github.com/PRQL/tree-sitter-prql"
    set-option buffer tree_sitter_rev "09e158cd3650581c0af4c49c2e5b10c4834c8646"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ptx %{
    set-option buffer tree_sitter_lang "ptx"
    set-option buffer tree_sitter_source "https://codeberg.org/jer-gremlin/tree-sitter-ptx"
    set-option buffer tree_sitter_rev "3dfa6758d4c15832d051f933101992b9e01d6611"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=pug %{
    set-option buffer tree_sitter_lang "pug"
    set-option buffer tree_sitter_source "https://github.com/zealot128/tree-sitter-pug"
    set-option buffer tree_sitter_rev "13e9195370172c86a8b88184cc358b23b677cc46"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=purescript %{
    set-option buffer tree_sitter_lang "purescript"
    set-option buffer tree_sitter_source "https://github.com/postsolar/tree-sitter-purescript"
    set-option buffer tree_sitter_rev "f541f95ffd6852fbbe88636317c613285bc105af"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:python|sage|starlark|tilt) %{
    set-option buffer tree_sitter_lang "python"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-python"
    set-option buffer tree_sitter_rev "293fdc02038ee2bf0e2e206711b69c90ac0d413f"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=codeql %{
    set-option buffer tree_sitter_lang "ql"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-ql"
    set-option buffer tree_sitter_rev "1fd627a4e8bff8c24c11987474bd33112bead857"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=qml %{
    set-option buffer tree_sitter_lang "qmljs"
    set-option buffer tree_sitter_source "https://github.com/yuja/tree-sitter-qmljs"
    set-option buffer tree_sitter_rev "0b2b25bcaa7d4925d5f0dda16f6a99c588a437f1"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=tsq %{
    set-option buffer tree_sitter_lang "query"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-query"
    set-option buffer tree_sitter_rev "a6674e279b14958625d7a530cabe06119c7a1532"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=quint %{
    set-option buffer tree_sitter_lang "quint"
    set-option buffer tree_sitter_source "https://github.com/gruhn/tree-sitter-quint"
    set-option buffer tree_sitter_rev "eebbd01edfeff6404778c92efe5554e42e506a18"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=r %{
    set-option buffer tree_sitter_lang "r"
    set-option buffer tree_sitter_source "https://github.com/r-lib/tree-sitter-r"
    set-option buffer tree_sitter_rev "cc04302e1bff76fa02e129f332f44636813b0c3c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=regex %{
    set-option buffer tree_sitter_lang "regex"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-regex"
    set-option buffer tree_sitter_rev "e1cfca3c79896ff79842f057ea13e529b66af636"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=rego %{
    set-option buffer tree_sitter_lang "rego"
    set-option buffer tree_sitter_source "https://github.com/FallenAngel97/tree-sitter-rego"
    set-option buffer tree_sitter_rev "ddd39af81fe8b0288102a7cb97959dfce723e0f3"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=pip-requirements %{
    set-option buffer tree_sitter_lang "requirements"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-requirements"
    set-option buffer tree_sitter_rev "caeb2ba854dea55931f76034978de1fd79362939"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=rescript %{
    set-option buffer tree_sitter_lang "rescript"
    set-option buffer tree_sitter_source "https://github.com/rescript-lang/tree-sitter-rescript"
    set-option buffer tree_sitter_rev "5e2a44a9d886b0a509f5bfd0437d33b4871fbac5"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ripple %{
    set-option buffer tree_sitter_lang "ripple"
    set-option buffer tree_sitter_source "https://github.com/Ripple-TS/ripple"
    set-option buffer tree_sitter_rev "49762f0a63de0d1845fcd2e6632639c095995336"
    set-option buffer tree_sitter_subpath "packages/tree-sitter"
}

hook global BufSetOption filetype=robot %{
    set-option buffer tree_sitter_lang "robot"
    set-option buffer tree_sitter_source "https://github.com/Hubro/tree-sitter-robot"
    set-option buffer tree_sitter_rev "322e4cc65754d2b3fdef4f2f8a71e0762e3d13af"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=robots.txt %{
    set-option buffer tree_sitter_lang "robots"
    set-option buffer tree_sitter_source "https://github.com/opa-oz/tree-sitter-robots-txt"
    set-option buffer tree_sitter_rev "8e3a4205b76236bb6dbebdbee5afc262ce38bb62"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ron %{
    set-option buffer tree_sitter_lang "ron"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-ron"
    set-option buffer tree_sitter_rev "78938553b93075e638035f624973083451b29055"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=rpmspec %{
    set-option buffer tree_sitter_lang "rpmspec"
    set-option buffer tree_sitter_source "https://gitlab.com/cryptomilk/tree-sitter-rpmspec"
    set-option buffer tree_sitter_rev "d0275cd1316d9b7a2e0fae028ce95a5a18741e1d"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=rshtml %{
    set-option buffer tree_sitter_lang "rshtml"
    set-option buffer tree_sitter_source "https://github.com/rshtml/tree-sitter-rshtml"
    set-option buffer tree_sitter_rev "89ae0f3a5e221a83aad243def85e822616d3b3c2"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=rst %{
    set-option buffer tree_sitter_lang "rst"
    set-option buffer tree_sitter_source "https://github.com/stsewd/tree-sitter-rst"
    set-option buffer tree_sitter_rev "ab09cab886a947c62a8c6fa94d3ad375f3f6a73d"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ruby %{
    set-option buffer tree_sitter_lang "ruby"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-ruby"
    set-option buffer tree_sitter_rev "206c7077164372c596ffa8eaadb9435c28941364"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:rust|rust-format-args-macro) %{
    set-option buffer tree_sitter_lang "rust"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-rust"
    set-option buffer tree_sitter_rev "261b20226c04ef601adbdf185a800512a5f66291"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=rust-format-args %{
    set-option buffer tree_sitter_lang "rust-format-args"
    set-option buffer tree_sitter_source "https://github.com/nik-rev/tree-sitter-rust-format-args"
    set-option buffer tree_sitter_rev "84ffe550e261cf5ea40a0ec31849ba2443bae99f"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=scala %{
    set-option buffer tree_sitter_lang "scala"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-scala"
    set-option buffer tree_sitter_rev "2d55e74b0485fe05058ffe5e8155506c9710c767"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=scfg %{
    set-option buffer tree_sitter_lang "scfg"
    set-option buffer tree_sitter_source "https://github.com/rockorager/tree-sitter-scfg"
    set-option buffer tree_sitter_rev "d850fd470445d73de318a21d734d1e09e29b773c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:dune|hy|racket|scheme) %{
    set-option buffer tree_sitter_lang "scheme"
    set-option buffer tree_sitter_source "https://github.com/6cdh/tree-sitter-scheme"
    set-option buffer tree_sitter_rev "af3af6c9356b936f8a515a1e449c32e804c2b1a8"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=scss %{
    set-option buffer tree_sitter_lang "scss"
    set-option buffer tree_sitter_source "https://github.com/serenadeai/tree-sitter-scss"
    set-option buffer tree_sitter_rev "c478c6868648eff49eb04a4df90d703dc45b312a"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=shellcheckrc %{
    set-option buffer tree_sitter_lang "shellcheckrc"
    set-option buffer tree_sitter_source "https://codeberg.org/kpbaks/tree-sitter-shellcheckrc"
    set-option buffer tree_sitter_rev "ad3da4e8f7fd72dcc5e93a6b89822c59a7cd10ff"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=slang %{
    set-option buffer tree_sitter_lang "slang"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-slang"
    set-option buffer tree_sitter_rev "327b1b821c255867a4fb724c8eee48887e3d014b"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=slint %{
    set-option buffer tree_sitter_lang "slint"
    set-option buffer tree_sitter_source "https://github.com/slint-ui/tree-sitter-slint"
    set-option buffer tree_sitter_rev "f11da7e62051ba8b9d4faa299c26de8aeedfc1cd"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=slisp %{
    set-option buffer tree_sitter_lang "slisp"
    set-option buffer tree_sitter_source "https://github.com/xenogenics/tree-sitter-slisp"
    set-option buffer tree_sitter_rev "29f9c6707ce9dfc2fc915d175ec720b207f179f3"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=smali %{
    set-option buffer tree_sitter_lang "smali"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-smali"
    set-option buffer tree_sitter_rev "fdfa6a1febc43c7467aa7e937b87b607956f2346"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=smithy %{
    set-option buffer tree_sitter_lang "smithy"
    set-option buffer tree_sitter_source "https://github.com/indoorvivants/tree-sitter-smithy"
    set-option buffer tree_sitter_rev "8327eb84d55639ffbe08c9dc82da7fff72a1ad07"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=sml %{
    set-option buffer tree_sitter_lang "sml"
    set-option buffer tree_sitter_source "https://github.com/Giorbo/tree-sitter-sml"
    set-option buffer tree_sitter_rev "bd4055d5554614520d4a0706b34dc0c317c6b608"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=snakemake %{
    set-option buffer tree_sitter_lang "snakemake"
    set-option buffer tree_sitter_source "https://github.com/osthomas/tree-sitter-snakemake"
    set-option buffer tree_sitter_rev "e909815acdbe37e69440261ebb1091ed52e1dec6"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=solidity %{
    set-option buffer tree_sitter_lang "solidity"
    set-option buffer tree_sitter_source "https://github.com/JoranHonig/tree-sitter-solidity"
    set-option buffer tree_sitter_rev "f7f5251a3f5b1d04f0799b3571b12918af177fc8"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=sourcepawn %{
    set-option buffer tree_sitter_lang "sourcepawn"
    set-option buffer tree_sitter_source "https://github.com/nilshelmig/tree-sitter-sourcepawn"
    set-option buffer tree_sitter_rev "f2af8d0dc14c6790130cceb2a20027eb41a8297c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=spade %{
    set-option buffer tree_sitter_lang "spade"
    set-option buffer tree_sitter_source "https://codeberg.org/spade-lang/tree-sitter-spade"
    set-option buffer tree_sitter_rev "996aaabea9a9fbe498ae0876356028a78e6ef8db"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=spicedb %{
    set-option buffer tree_sitter_lang "spicedb"
    set-option buffer tree_sitter_source "https://github.com/jzelinskie/tree-sitter-spicedb"
    set-option buffer tree_sitter_rev "a4e4645651f86d6684c15dfa9931b7841dc52a66"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=sql %{
    set-option buffer tree_sitter_lang "sql"
    set-option buffer tree_sitter_source "https://github.com/DerekStride/tree-sitter-sql"
    set-option buffer tree_sitter_rev "86e3d03837d282544439620eb74d224586074b8b"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=sshclientconfig %{
    set-option buffer tree_sitter_lang "sshclientconfig"
    set-option buffer tree_sitter_source "https://github.com/metio/tree-sitter-ssh-client-config"
    set-option buffer tree_sitter_rev "e45c6d5c71657344d4ecaf87dafae7736f776c57"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=strace %{
    set-option buffer tree_sitter_lang "strace"
    set-option buffer tree_sitter_source "https://github.com/sigmaSd/tree-sitter-strace"
    set-option buffer tree_sitter_rev "2b18fdf9a01e7ec292cc6006724942c81beb7fd5"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=strictdoc %{
    set-option buffer tree_sitter_lang "strictdoc"
    set-option buffer tree_sitter_source "https://github.com/manueldiagostino/tree-sitter-strictdoc"
    set-option buffer tree_sitter_rev "070edcf23f7c85af355437706048f73833e0ea10"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=styx %{
    set-option buffer tree_sitter_lang "styx"
    set-option buffer tree_sitter_source "https://github.com/bearcove/styx"
    set-option buffer tree_sitter_rev "ff7f49629a20a308111810a0c5e6228617133ea7"
    set-option buffer tree_sitter_subpath "crates/tree-sitter-styx"
}

hook global BufSetOption filetype=supercollider %{
    set-option buffer tree_sitter_lang "supercollider"
    set-option buffer tree_sitter_source "https://github.com/madskjeldgaard/tree-sitter-supercollider"
    set-option buffer tree_sitter_rev "3b35bd0fded4423c8fb30e9585c7bacbcd0e8095"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=svelte %{
    set-option buffer tree_sitter_lang "svelte"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-svelte"
    set-option buffer tree_sitter_rev "ae5199db47757f785e43a14b332118a5474de1a2"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=sway %{
    set-option buffer tree_sitter_lang "sway"
    set-option buffer tree_sitter_source "https://github.com/FuelLabs/tree-sitter-sway"
    set-option buffer tree_sitter_rev "e491a005ee1d310f4c138bf215afd44cfebf959c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=swift %{
    set-option buffer tree_sitter_lang "swift"
    set-option buffer tree_sitter_source "https://github.com/alex-pinkus/tree-sitter-swift"
    set-option buffer tree_sitter_rev "57c1c6d6ffa1c44b330182d41717e6fe37430704"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=systemverilog %{
    set-option buffer tree_sitter_lang "systemverilog"
    set-option buffer tree_sitter_source "https://github.com/gmlarumbe/tree-sitter-systemverilog"
    set-option buffer tree_sitter_rev "3bd2c5d2f60ed7b07c2177b34e2976ad9a87c659"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=t32 %{
    set-option buffer tree_sitter_lang "t32"
    set-option buffer tree_sitter_source "https://gitlab.com/xasc/tree-sitter-t32"
    set-option buffer tree_sitter_rev "6da5e3cbabd376b566d04282005e52ffe67ef74a"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=tablegen %{
    set-option buffer tree_sitter_lang "tablegen"
    set-option buffer tree_sitter_source "https://github.com/Flakebi/tree-sitter-tablegen"
    set-option buffer tree_sitter_rev "3e9c4822ab5cdcccf4f8aa9dcd42117f736d51d9"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=tact %{
    set-option buffer tree_sitter_lang "tact"
    set-option buffer tree_sitter_source "https://github.com/tact-lang/tree-sitter-tact"
    set-option buffer tree_sitter_rev "ec57ab29c86d632639726631fb2bb178d23e1c91"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=task %{
    set-option buffer tree_sitter_lang "task"
    set-option buffer tree_sitter_source "https://github.com/alexanderbrevig/tree-sitter-task"
    set-option buffer tree_sitter_rev "f2cb435c5dbf3ee19493e224485d977cb2d36d8b"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=tcl %{
    set-option buffer tree_sitter_lang "tcl"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-tcl"
    set-option buffer tree_sitter_rev "56ad1fa6a34ba800e5495d1025a9b0fda338d5b8"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=teal %{
    set-option buffer tree_sitter_lang "teal"
    set-option buffer tree_sitter_source "https://github.com/euclidianAce/tree-sitter-teal"
    set-option buffer tree_sitter_rev "3db655924b2ff1c54fdf6371b5425ea6b5dccefe"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=templ %{
    set-option buffer tree_sitter_lang "templ"
    set-option buffer tree_sitter_source "https://github.com/vrischmann/tree-sitter-templ"
    set-option buffer tree_sitter_rev "47594c5cbef941e6a3ccf4ddb934a68cf4c68075"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=tera %{
    set-option buffer tree_sitter_lang "tera"
    set-option buffer tree_sitter_source "https://github.com/uncenter/tree-sitter-tera"
    set-option buffer tree_sitter_rev "e8d679a29c03e64656463a892a30da626e19ed8e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=textproto %{
    set-option buffer tree_sitter_lang "textproto"
    set-option buffer tree_sitter_source "https://github.com/PorterAtGoogle/tree-sitter-textproto"
    set-option buffer tree_sitter_rev "568471b80fd8793d37ed01865d8c2208a9fefd1b"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=thrift %{
    set-option buffer tree_sitter_lang "thrift"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-thrift"
    set-option buffer tree_sitter_rev "68fd0d80943a828d9e6f49c58a74be1e9ca142cf"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=tlaplus %{
    set-option buffer tree_sitter_lang "tlaplus"
    set-option buffer tree_sitter_source "https://github.com/tlaplus-community/tree-sitter-tlaplus"
    set-option buffer tree_sitter_rev "4ba91b07b97741a67f61221d0d50e6d962e4987e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=todotxt %{
    set-option buffer tree_sitter_lang "todotxt"
    set-option buffer tree_sitter_source "https://github.com/arnarg/tree-sitter-todotxt"
    set-option buffer tree_sitter_rev "3937c5cd105ec4127448651a21aef45f52d19609"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:cross-config|git-cliff-config|jjconfig|miseconfig|toml) %{
    set-option buffer tree_sitter_lang "toml"
    set-option buffer tree_sitter_source "https://github.com/ikatyang/tree-sitter-toml"
    set-option buffer tree_sitter_rev "7cff70bbcbbc62001b465603ca1ea88edd668704"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=tql %{
    set-option buffer tree_sitter_lang "tql"
    set-option buffer tree_sitter_source "https://github.com/tenzir/tree-sitter-tql"
    set-option buffer tree_sitter_rev "d3b3b2699bc09fd0c63e2c13f15f6e474665c62e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=tsx %{
    set-option buffer tree_sitter_lang "tsx"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-typescript"
    set-option buffer tree_sitter_rev "75b3874edb2dc714fb1fd77a32013d0f8699989f"
    set-option buffer tree_sitter_subpath "tsx"
}

hook global BufSetOption filetype=twig %{
    set-option buffer tree_sitter_lang "twig"
    set-option buffer tree_sitter_source "https://github.com/gbprod/tree-sitter-twig"
    set-option buffer tree_sitter_rev "085648e01d1422163a1702a44e72303b4e2a0bd1"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=typescript %{
    set-option buffer tree_sitter_lang "typescript"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-typescript"
    set-option buffer tree_sitter_rev "75b3874edb2dc714fb1fd77a32013d0f8699989f"
    set-option buffer tree_sitter_subpath "typescript"
}

hook global BufSetOption filetype=typespec %{
    set-option buffer tree_sitter_lang "typespec"
    set-option buffer tree_sitter_source "https://github.com/happenslol/tree-sitter-typespec"
    set-option buffer tree_sitter_rev "0ee05546d73d8eb64635ed8125de6f35c77759fe"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=typst %{
    set-option buffer tree_sitter_lang "typst"
    set-option buffer tree_sitter_source "https://github.com/uben0/tree-sitter-typst"
    set-option buffer tree_sitter_rev "13863ddcbaa7b68ee6221cea2e3143415e64aea4"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=ungrammar %{
    set-option buffer tree_sitter_lang "ungrammar"
    set-option buffer tree_sitter_source "https://github.com/Philipp-M/tree-sitter-ungrammar"
    set-option buffer tree_sitter_rev "a7e104629cff5a8b7367187610631e8f5eb7c6ea"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=unison %{
    set-option buffer tree_sitter_lang "unison"
    set-option buffer tree_sitter_source "https://github.com/kylegoetz/tree-sitter-unison"
    set-option buffer tree_sitter_rev "3c97db76d3cdbd002dfba493620c2d5df2fd6fa9"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=uxntal %{
    set-option buffer tree_sitter_lang "uxntal"
    set-option buffer tree_sitter_source "https://github.com/Jummit/tree-sitter-uxntal"
    set-option buffer tree_sitter_rev "d68406066648cd6db4c6a2f11ec305af02079884"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=v %{
    set-option buffer tree_sitter_lang "v"
    set-option buffer tree_sitter_source "https://github.com/vlang/v-analyzer"
    set-option buffer tree_sitter_rev "59a8889d84a293d7c0366d14c8dbb0eec24fe889"
    set-option buffer tree_sitter_subpath "tree_sitter_v"
}

hook global BufSetOption filetype=vala %{
    set-option buffer tree_sitter_lang "vala"
    set-option buffer tree_sitter_source "https://github.com/vala-lang/tree-sitter-vala"
    set-option buffer tree_sitter_rev "c9eea93ba2ec4ec1485392db11945819779745b3"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=vento %{
    set-option buffer tree_sitter_lang "vento"
    set-option buffer tree_sitter_source "https://github.com/ventojs/tree-sitter-vento"
    set-option buffer tree_sitter_rev "3b32474bc29584ea214e4e84b47102408263fe0e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=verilog %{
    set-option buffer tree_sitter_lang "verilog"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter/tree-sitter-verilog"
    set-option buffer tree_sitter_rev "4457145e795b363f072463e697dfe2f6973c9a52"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=vhdl %{
    set-option buffer tree_sitter_lang "vhdl"
    set-option buffer tree_sitter_source "https://github.com/jpt13653903/tree-sitter-vhdl"
    set-option buffer tree_sitter_rev "32d3e3daa745bf9f1665676f323be968444619e1"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=vhs %{
    set-option buffer tree_sitter_lang "vhs"
    set-option buffer tree_sitter_source "https://github.com/charmbracelet/tree-sitter-vhs"
    set-option buffer tree_sitter_rev "9534865e614c95eb9418e5e73f061c32fa4d9540"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=vim %{
    set-option buffer tree_sitter_lang "vim"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-vim"
    set-option buffer tree_sitter_rev "f3cd62d8bd043ef20507e84bb6b4b53731ccf3a7"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=vue %{
    set-option buffer tree_sitter_lang "vue"
    set-option buffer tree_sitter_source "https://github.com/ikatyang/tree-sitter-vue"
    set-option buffer tree_sitter_rev "91fe2754796cd8fba5f229505a23fa08f3546c06"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=wast %{
    set-option buffer tree_sitter_lang "wast"
    set-option buffer tree_sitter_source "https://github.com/wasm-lsp/tree-sitter-wasm"
    set-option buffer tree_sitter_rev "2ca28a9f9d709847bf7a3de0942a84e912f59088"
    set-option buffer tree_sitter_subpath "wast"
}

hook global BufSetOption filetype=wat %{
    set-option buffer tree_sitter_lang "wat"
    set-option buffer tree_sitter_source "https://github.com/wasm-lsp/tree-sitter-wasm"
    set-option buffer tree_sitter_rev "2ca28a9f9d709847bf7a3de0942a84e912f59088"
    set-option buffer tree_sitter_subpath "wat"
}

hook global BufSetOption filetype=werk %{
    set-option buffer tree_sitter_lang "werk"
    set-option buffer tree_sitter_source "https://github.com/little-bonsai/tree-sitter-werk"
    set-option buffer tree_sitter_rev "92b0f7fe98465c4c435794a58e961306193d1c1e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=wesl %{
    set-option buffer tree_sitter_lang "wesl"
    set-option buffer tree_sitter_source "https://github.com/wgsl-tooling-wg/tree-sitter-wesl"
    set-option buffer tree_sitter_rev "94ee6122680ef8ce2173853ca7c99f7aaeeda8ce"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=wgsl %{
    set-option buffer tree_sitter_lang "wgsl"
    set-option buffer tree_sitter_source "https://github.com/szebniok/tree-sitter-wgsl"
    set-option buffer tree_sitter_rev "272e89ef2aeac74178edb9db4a83c1ffef80a463"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=wikitext %{
    set-option buffer tree_sitter_lang "wikitext"
    set-option buffer tree_sitter_source "https://github.com/santhoshtr/tree-sitter-wikitext"
    set-option buffer tree_sitter_rev "444214b31695e9dd4d32fb06247397fb8778a9d2"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=wit %{
    set-option buffer tree_sitter_lang "wit"
    set-option buffer tree_sitter_source "https://github.com/hh9527/tree-sitter-wit"
    set-option buffer tree_sitter_rev "c917790ab9aec50c5fd664cbfad8dd45110cfff3"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=wren %{
    set-option buffer tree_sitter_lang "wren"
    set-option buffer tree_sitter_source "https://git.sr.ht/~jummit/tree-sitter-wren"
    set-option buffer tree_sitter_rev "6748694be32f11e7ec6b5faeb1b48ca6156d4e06"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=xit %{
    set-option buffer tree_sitter_lang "xit"
    set-option buffer tree_sitter_source "https://github.com/synaptiko/tree-sitter-xit"
    set-option buffer tree_sitter_rev "7d7902456061bc2ad21c64c44054f67b5515734c"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:msbuild|xml) %{
    set-option buffer tree_sitter_lang "xml"
    set-option buffer tree_sitter_source "https://github.com/RenjiSann/tree-sitter-xml"
    set-option buffer tree_sitter_rev "48a7c2b6fb9d515577e115e6788937e837815651"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=xtc %{
    set-option buffer tree_sitter_lang "xtc"
    set-option buffer tree_sitter_source "https://github.com/Alexis-Lapierre/tree-sitter-xtc"
    set-option buffer tree_sitter_rev "7bc11b736250c45e25cfb0215db2f8393779957e"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=(?:docker-compose|github-action|gitlab-ci|nestedtext|woodpecker-ci|yaml) %{
    set-option buffer tree_sitter_lang "yaml"
    set-option buffer tree_sitter_source "https://github.com/ikatyang/tree-sitter-yaml"
    set-option buffer tree_sitter_rev "0e36bed171768908f331ff7dff9d956bae016efb"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=yara %{
    set-option buffer tree_sitter_lang "yara"
    set-option buffer tree_sitter_source "https://github.com/egibs/tree-sitter-yara"
    set-option buffer tree_sitter_rev "eb3ede203275c38000177f72ec0f9965312806ef"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=yuck %{
    set-option buffer tree_sitter_lang "yuck"
    set-option buffer tree_sitter_source "https://github.com/Philipp-M/tree-sitter-yuck"
    set-option buffer tree_sitter_rev "e3d91a3c65decdea467adebe4127b8366fa47919"
    set-option buffer tree_sitter_subpath ""
}

hook global BufSetOption filetype=zig %{
    set-option buffer tree_sitter_lang "zig"
    set-option buffer tree_sitter_source "https://github.com/tree-sitter-grammars/tree-sitter-zig"
    set-option buffer tree_sitter_rev "6479aa13f32f701c383083d8b28360ebd682fb7d"
    set-option buffer tree_sitter_subpath ""
}
