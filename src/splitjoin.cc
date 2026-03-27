#include "splitjoin.hh"

namespace Kakoune
{

static const SJRule sj_bash[] = {
    {"statement", false, SJPreset::Statement, false, true, "", ";", {}},
    {"array", false, SJPreset::List, false, false, "", "", {}},
    {"compound_statement", false, SJPreset::Statement, false, true, "", ";", {}},
    {"do_group", false, SJPreset::Statement, false, true, "", ";", {}},
    {"if_statement", false, SJPreset::Statement, false, true, "", ";", {}},
    {"variable_assignment", true, SJPreset::Args, false, false, ",", "", {"array"}},
    {"for_statement", true, SJPreset::Args, false, false, ",", "", {"do_group", "compound_statement"}},
};

static const SJRule sj_c[] = {
    {"parameter_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"argument_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"initializer_list", false, SJPreset::List, true, true, ",", "", {}},
    {"compound_statement", false, SJPreset::Statement, false, true, "", ";", {}},
    {"enumerator_list", false, SJPreset::List, true, true, ",", "", {}},
    {"if_statement", true, SJPreset::Args, false, false, ",", "", {"compound_statement"}},
    {"declaration", true, SJPreset::Args, false, false, ",", "", {"parameter_list", "argument_list", "initializer_list"}},
    {"call_expression", true, SJPreset::Args, false, false, ",", "", {"argument_list"}},
    {"enum_specifier", true, SJPreset::Args, false, false, ",", "", {"enumerator_list"}},
};

static const SJRule sj_css[] = {
    {"block", false, SJPreset::Statement, false, true, "", "", {}},
    {"keyframe_block_list", false, SJPreset::Statement, false, true, "", "", {}},
    {"arguments", false, SJPreset::Args, false, false, ",", "", {}},
    {"call_expression", true, SJPreset::Args, false, false, ",", "", {"arguments"}},
    {"rule_set", true, SJPreset::Args, false, false, ",", "", {"block"}},
    {"media_statement", true, SJPreset::Args, false, false, ",", "", {"block"}},
    {"keyframes_statement", true, SJPreset::Args, false, false, ",", "", {"keyframe_block_list"}},
    {"supports_statement", true, SJPreset::Args, false, false, ",", "", {"block"}},
    {"at_rule", true, SJPreset::Args, false, false, ",", "", {"block"}},
};

static const SJRule sj_dart[] = {
    {"list_literal", false, SJPreset::List, true, true, ",", "", {}},
    {"set_or_map_literal", false, SJPreset::Dict, true, true, ",", "", {}},
    {"block", false, SJPreset::Statement, false, true, "", ";", {}},
    {"arguments", false, SJPreset::Args, false, false, ",", "", {}},
    {"formal_parameter_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"static_final_declaration", true, SJPreset::Args, false, false, ",", "", {"list_literal", "set_or_map_literal"}},
    {"initialized_identifier", true, SJPreset::Args, false, false, ",", "", {"list_literal", "set_or_map_literal"}},
};

static const SJRule sj_elixir[] = {
    {"list", false, SJPreset::List, false, false, ",", "", {}},
    {"map_content", false, SJPreset::List, false, false, ",", "", {}},
    {"keywords", false, SJPreset::List, false, false, ",", "", {}},
    {"arguments", false, SJPreset::Args, false, false, ",", "", {}},
    {"tuple", false, SJPreset::List, false, false, ",", "", {}},
    {"map", true, SJPreset::Args, false, false, ",", "", {"map_content"}},
    {"binary_operator", true, SJPreset::Args, false, false, ",", "", {"list", "map", "tuple"}},
};

static const SJRule sj_go[] = {
    {"literal_value", false, SJPreset::List, true, true, ",", "", {}},
    {"parameter_list", false, SJPreset::Args, true, false, ",", "", {}},
    {"argument_list", false, SJPreset::Args, true, false, ",", "", {}},
    {"type_arguments", false, SJPreset::Args, true, false, ",", "", {}},
    {"block", false, SJPreset::Statement, false, true, "", ";", {}},
    {"import_spec", false, SJPreset::Args, false, false, ",", "", {}},
    {"import_spec_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"import_declaration", true, SJPreset::Args, false, false, ",", "", {"import_spec", "import_spec_list"}},
    {"function_declaration", true, SJPreset::Args, false, false, ",", "", {"block"}},
    {"if_statement", true, SJPreset::Args, false, false, ",", "", {"block"}},
    {"short_var_declaration", true, SJPreset::Args, false, false, ",", "", {"literal_value"}},
    {"var_declaration", true, SJPreset::Args, false, false, ",", "", {"literal_value"}},
};

static const SJRule sj_html[] = {
    {"start_tag", false, SJPreset::Default, false, false, "", "", {}},
    {"self_closing_tag", false, SJPreset::Default, false, false, "", "", {}},
    {"element", false, SJPreset::Default, false, false, "", "", {}},
};

static const SJRule sj_java[] = {
    {"argument_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"formal_parameters", false, SJPreset::Args, false, false, ",", "", {}},
    {"block", false, SJPreset::Statement, false, true, "", ";", {}},
    {"constructor_body", false, SJPreset::Statement, false, true, "", ";", {}},
    {"array_initializer", false, SJPreset::List, true, true, ",", "", {}},
    {"annotation_argument_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"enum_body", false, SJPreset::Dict, true, true, ",", "", {}},
    {"enum_declaration", true, SJPreset::Args, false, false, ",", "", {"enum_body"}},
    {"if_statement", true, SJPreset::Args, false, false, ",", "", {"block"}},
    {"annotation", true, SJPreset::Args, false, false, ",", "", {"annotation_argument_list"}},
    {"method_declaration", true, SJPreset::Args, false, false, ",", "", {"block"}},
    {"variable_declarator", true, SJPreset::Args, false, false, ",", "", {"array_initializer"}},
    {"constructor_declaration", true, SJPreset::Args, false, false, ",", "", {"constructor_body"}},
};

static const SJRule sj_javascript[] = {
    {"object", false, SJPreset::Dict, true, true, ",", "", {}},
    {"object_pattern", false, SJPreset::Dict, true, true, ",", "", {}},
    {"array", false, SJPreset::List, true, true, ",", "", {}},
    {"array_pattern", false, SJPreset::List, true, true, ",", "", {}},
    {"formal_parameters", false, SJPreset::Args, false, false, ",", "", {}},
    {"arguments", false, SJPreset::Args, false, false, ",", "", {}},
    {"named_imports", false, SJPreset::Dict, true, true, ",", "", {}},
    {"export_clause", false, SJPreset::Dict, true, true, ",", "", {}},
    {"statement_block", false, SJPreset::Statement, false, true, "", ";", {}},
    {"body", false, SJPreset::Statement, false, true, "", ";", {}},
    {"jsx_opening_element", false, SJPreset::Default, false, false, "", "", {}},
    {"jsx_element", false, SJPreset::Default, false, false, "", "", {}},
    {"jsx_self_closing_element", false, SJPreset::Default, false, false, "", "", {}},
    {"arrow_function", true, SJPreset::Args, false, false, ",", "", {"body"}},
    {"lexical_declaration", true, SJPreset::Args, false, false, ",", "", {"array", "object"}},
    {"pair", true, SJPreset::Args, false, false, ",", "", {"array", "object"}},
    {"variable_declaration", true, SJPreset::Args, false, false, ",", "", {"array", "object"}},
    {"assignment_expression", true, SJPreset::Args, false, false, ",", "", {"array", "object"}},
    {"try_statement", true, SJPreset::Args, false, false, ",", "", {"statement_block"}},
    {"function_declaration", true, SJPreset::Args, false, false, ",", "", {"statement_block"}},
    {"catch_clause", true, SJPreset::Args, false, false, ",", "", {"statement_block"}},
    {"finally_clause", true, SJPreset::Args, false, false, ",", "", {"statement_block"}},
    {"export_statement", true, SJPreset::Args, false, false, ",", "", {"export_clause", "object"}},
    {"import_statement", true, SJPreset::Args, false, false, ",", "", {"named_imports", "object"}},
    {"if_statement", true, SJPreset::Args, false, false, ",", "", {"statement_block", "object"}},
};

static const SJRule sj_json[] = {
    {"object", false, SJPreset::Dict, false, true, ",", "", {}},
    {"array", false, SJPreset::List, false, true, ",", "", {}},
    {"pair", true, SJPreset::Args, false, false, ",", "", {"object", "array"}},
};

static const SJRule sj_julia[] = {
    {"argument_list", false, SJPreset::Args, true, false, ",", "", {}},
    {"vector_expression", false, SJPreset::List, true, false, ",", "", {}},
    {"matrix_expression", false, SJPreset::Statement, false, false, "", ";", {}},
    {"tuple_expression", false, SJPreset::List, true, false, ",", "", {}},
    {"comprehension_expression", false, SJPreset::List, true, false, "", "", {}},
    {"open_tuple", false, SJPreset::Args, true, false, ",", "", {}},
    {"call_expression", true, SJPreset::Args, false, false, ",", "", {"argument_list"}},
};

static const SJRule sj_kotlin[] = {
    {"collection_literal", false, SJPreset::Default, false, false, ",", "", {}},
    {"value_arguments", false, SJPreset::Default, false, false, ",", "", {}},
    {"statements", false, SJPreset::Statement, false, true, "", ";", {}},
    {"function_value_parameters", false, SJPreset::Args, false, false, ",", "", {}},
    {"lambda_literal", true, SJPreset::Args, false, false, ",", "", {"statements"}},
    {"function_body", true, SJPreset::Args, false, false, ",", "", {"statements"}},
    {"property_declaration", true, SJPreset::Args, false, false, ",", "", {"collection_literal", "value_arguments"}},
};

static const SJRule sj_lua[] = {
    {"table_constructor", false, SJPreset::Dict, true, true, ",", "", {}},
    {"arguments", false, SJPreset::Args, false, false, ",", "", {}},
    {"parameters", false, SJPreset::Args, false, false, ",", "", {}},
    {"block", false, SJPreset::Statement, false, true, "", "", {}},
    {"variable_declaration", true, SJPreset::Args, false, false, ",", "", {"table_constructor", "block"}},
    {"assignment_statement", true, SJPreset::Args, false, false, ",", "", {"table_constructor", "block"}},
    {"if_statement", true, SJPreset::Args, false, false, ",", "", {"block"}},
    {"else_statement", true, SJPreset::Args, false, false, ",", "", {"block"}},
    {"function_declaration", true, SJPreset::Args, false, false, ",", "", {"block"}},
    {"function_definition", true, SJPreset::Args, false, false, ",", "", {"block"}},
};

static const SJRule sj_nix[] = {
    {"list_expression", false, SJPreset::List, true, true, "", "", {}},
    {"binding_set", false, SJPreset::Dict, true, true, "", ";", {}},
    {"formals", false, SJPreset::Args, false, true, ",", "", {}},
    {"let_expression", false, SJPreset::Default, false, true, "", "", {}},
    {"attrset_expression", true, SJPreset::Args, false, false, ",", "", {"binding_set"}},
};

static const SJRule sj_perl[] = {
    {"list_expression", false, SJPreset::Statement, false, false, "", "", {}},
    {"block", false, SJPreset::Statement, false, true, "", ";", {}},
    {"array", false, SJPreset::Args, false, false, ",", "", {}},
    {"hash_ref", false, SJPreset::Dict, true, true, ",", "", {}},
    {"array_ref", false, SJPreset::Dict, true, true, ",", "", {}},
    {"anonymous_array_expression", true, SJPreset::Args, false, false, ",", "", {"list_expression"}},
    {"anonymous_hash_expression", true, SJPreset::Args, false, false, ",", "", {"list_expression"}},
    {"variable_declaration", true, SJPreset::Args, false, false, ",", "", {"array", "array_ref", "hash_ref"}},
};

static const SJRule sj_php[] = {
    {"array_creation_expression", false, SJPreset::Dict, true, false, ",", "", {}},
    {"arguments", false, SJPreset::Args, true, false, ",", "", {}},
    {"formal_parameters", false, SJPreset::Args, true, false, ",", "", {}},
    {"compound_statement", false, SJPreset::Statement, false, true, "", ";", {}},
    {"assignment_expression", true, SJPreset::Args, false, false, ",", "", {"array_creation_expression", "arguments"}},
    {"if_statement", true, SJPreset::Args, false, false, ",", "", {"compound_statement"}},
    {"else_clause", true, SJPreset::Args, false, false, ",", "", {"compound_statement"}},
    {"try_statement", true, SJPreset::Args, false, false, ",", "", {"compound_statement"}},
    {"catch_clause", true, SJPreset::Args, false, false, ",", "", {"compound_statement"}},
    {"anonymous_function_creation_expression", true, SJPreset::Args, false, false, ",", "", {"compound_statement"}},
    {"function_definition", true, SJPreset::Args, false, false, ",", "", {"compound_statement"}},
    {"method_declaration", true, SJPreset::Args, false, false, ",", "", {"compound_statement"}},
    {"arrow_function", true, SJPreset::Args, false, false, ",", "", {"formal_parameters"}},
    {"function_call_expression", true, SJPreset::Args, false, false, ",", "", {"arguments"}},
    {"scoped_call_expression", true, SJPreset::Args, false, false, ",", "", {"arguments"}},
    {"member_call_expression", true, SJPreset::Args, false, false, ",", "", {"arguments"}},
    {"object_creation_expression", true, SJPreset::Args, false, false, ",", "", {"arguments"}},
};

static const SJRule sj_pug[] = {
    {"attributes", false, SJPreset::Default, false, false, "", "", {}},
    {"tag", true, SJPreset::Args, false, false, ",", "", {"attributes"}},
};

static const SJRule sj_python[] = {
    {"no_space_in_brackets", false, SJPreset::List, true, false, ",", "", {}},
    {"pattern_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"tuple_pattern", false, SJPreset::List, false, true, ",", "", {}},
    {"tuple", false, SJPreset::List, true, false, ",", "", {}},
    {"import_from_statement", false, SJPreset::Args, false, false, ",", "", {}},
    {"argument_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"parameters", false, SJPreset::Args, true, false, ",", "", {}},
    {"parenthesized_expression", false, SJPreset::Args, false, false, ",", "", {}},
    {"list_comprehension", false, SJPreset::Args, false, false, ",", "", {}},
    {"set_comprehension", false, SJPreset::Args, false, false, ",", "", {}},
    {"dictionary_comprehension", false, SJPreset::Args, false, false, ",", "", {}},
    {"dictionary", false, SJPreset::List, true, true, ",", "", {}},
    {"list", false, SJPreset::List, true, true, ",", "", {}},
    {"set", false, SJPreset::List, true, true, ",", "", {}},
    {"assignment", true, SJPreset::Args, false, false, ",", "", {"tuple", "list", "dictionary", "set", "argument_list", "list_comprehension", "set_comprehension", "dictionary_comprehension"}},
    {"decorator", true, SJPreset::Args, false, false, ",", "", {"argument_list"}},
    {"raise_statement", true, SJPreset::Args, false, false, ",", "", {"argument_list"}},
    {"call", true, SJPreset::Args, false, false, ",", "", {"argument_list"}},
    {"function_definition", true, SJPreset::Args, false, false, ",", "", {"parameters"}},
};

static const SJRule sj_r[] = {
    {"arguments", false, SJPreset::Args, false, false, ",", "", {}},
    {"parameters", false, SJPreset::Args, false, false, ",", "", {}},
    {"left_assignment", true, SJPreset::Args, false, false, ",", "", {"arguments", "parameters"}},
    {"super_assignment", true, SJPreset::Args, false, false, ",", "", {"arguments", "parameters"}},
    {"right_assignment", true, SJPreset::Args, false, false, ",", "", {"arguments", "parameters"}},
    {"super_right_assignment", true, SJPreset::Args, false, false, ",", "", {"arguments", "parameters"}},
    {"equals_assignment", true, SJPreset::Args, false, false, ",", "", {"arguments", "parameters"}},
    {"function_definition", true, SJPreset::Args, false, false, ",", "", {"parameters"}},
    {"call", true, SJPreset::Args, false, false, ",", "", {"arguments"}},
    {"binary_operator", true, SJPreset::Args, false, false, ",", "", {"arguments", "parameters"}},
    {"pipe", true, SJPreset::Args, false, false, ",", "", {"arguments"}},
};

static const SJRule sj_ruby[] = {
    {"array", false, SJPreset::List, true, true, ",", "", {}},
    {"hash", false, SJPreset::List, true, true, ",", "", {}},
    {"method_parameters", false, SJPreset::Args, false, false, ",", "", {}},
    {"argument_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"block", false, SJPreset::Dict, true, true, "", "", {}},
    {"string_array", false, SJPreset::List, false, true, ",", "", {}},
    {"body_statement", false, SJPreset::Statement, false, true, "", ";", {}},
    {"if_modifier", false, SJPreset::Default, false, false, "", "", {}},
    {"unless_modifier", false, SJPreset::Default, false, false, "", "", {}},
    {"conditional", false, SJPreset::Default, false, false, "", "", {}},
    {"when", false, SJPreset::Default, false, true, "", "", {}},
    {"right", false, SJPreset::Default, false, false, "", "", {}},
    {"operator_assignment", true, SJPreset::Args, false, false, ",", "", {"right"}},
    {"method", true, SJPreset::Args, false, false, ",", "", {"body_statement"}},
    {"assignment", true, SJPreset::Args, false, false, ",", "", {"array", "hash"}},
    {"call", true, SJPreset::Args, false, false, ",", "", {"block", "do_block"}},
};

static const SJRule sj_rust[] = {
    {"field_declaration_list", false, SJPreset::Dict, true, true, ",", "", {}},
    {"declaration_list", false, SJPreset::Statement, false, true, "", "", {}},
    {"field_initializer_list", false, SJPreset::Dict, true, true, ",", "", {}},
    {"struct_pattern", false, SJPreset::Dict, true, true, ",", "", {}},
    {"parameters", false, SJPreset::Args, true, false, ",", "", {}},
    {"arguments", false, SJPreset::Args, true, false, ",", "", {}},
    {"tuple_type", false, SJPreset::Args, true, false, ",", "", {}},
    {"enum_variant_list", false, SJPreset::List, true, true, ",", "", {}},
    {"tuple_expression", false, SJPreset::Args, true, false, ",", "", {}},
    {"block", false, SJPreset::Statement, false, true, "", "", {}},
    {"value", false, SJPreset::Statement, false, true, "", "", {}},
    {"use_list", false, SJPreset::List, true, true, ",", "", {}},
    {"array_expression", false, SJPreset::List, true, true, ",", "", {}},
    {"parenthesized_expression", false, SJPreset::Args, false, false, ",", "", {}},
    {"match_arm", true, SJPreset::Args, false, false, ",", "", {"value"}},
    {"closure_expression", true, SJPreset::Args, false, false, ",", "", {"body", "value"}},
    {"let_declaration", true, SJPreset::Args, false, false, ",", "", {"field_declaration_list", "field_initializer_list", "array_expression"}},
    {"function_item", true, SJPreset::Args, false, false, ",", "", {"parameters"}},
    {"enum_item", true, SJPreset::Args, false, false, ",", "", {"enum_variant_list"}},
    {"use_declaration", true, SJPreset::Args, false, false, ",", "", {"use_list"}},
    {"trait_item", true, SJPreset::Args, false, false, ",", "", {"declaration_list"}},
};

static const SJRule sj_sql[] = {
    {"column_definitions", false, SJPreset::Args, false, false, ",", "", {}},
    {"ordered_columns", false, SJPreset::Args, false, false, ",", "", {}},
    {"list", false, SJPreset::Args, false, false, ",", "", {}},
};

static const SJRule sj_starlark[] = {
    {"assignment", true, SJPreset::Args, false, false, ",", "", {"tuple", "list", "dictionary", "argument_list", "list_comprehension", "dictionary_comprehension"}},
    {"call", true, SJPreset::Args, false, false, ",", "", {"argument_list"}},
};

static const SJRule sj_terraform[] = {
    {"tuple", false, SJPreset::List, true, false, ",", "", {}},
    {"object", false, SJPreset::Dict, true, false, "", "", {}},
    {"function_arguments", false, SJPreset::Args, false, false, ",", "", {}},
    {"function_call", true, SJPreset::Args, false, false, ",", "", {"function_arguments"}},
};

static const SJRule sj_toml[] = {
    {"array", false, SJPreset::List, false, true, ",", "", {}},
};

static const SJRule sj_typst[] = {
    {"content", false, SJPreset::Default, false, false, "", "", {}},
    {"group", false, SJPreset::Default, true, false, ",", "", {}},
};

static const SJRule sj_zig[] = {
    {"parameters", false, SJPreset::Args, true, false, ",", "", {}},
    {"arguments", false, SJPreset::Args, false, false, ",", "", {}},
    {"initializer_list", false, SJPreset::List, true, true, ",", "", {}},
    {"struct_declaration", false, SJPreset::List, true, true, ",", "", {}},
    {"enum_declaration", false, SJPreset::List, true, true, ",", "", {}},
    {"call_expression", false, SJPreset::Args, true, false, ",", "", {}},
};

// C++ extends C with template support
static const SJRule sj_cpp[] = {
    // All C rules
    {"parameter_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"argument_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"initializer_list", false, SJPreset::List, true, true, ",", "", {}},
    {"compound_statement", false, SJPreset::Statement, false, true, "", ";", {}},
    {"enumerator_list", false, SJPreset::List, true, true, ",", "", {}},
    {"if_statement", true, SJPreset::Args, false, false, ",", "", {"compound_statement"}},
    {"declaration", true, SJPreset::Args, false, false, ",", "", {"parameter_list", "argument_list", "initializer_list"}},
    {"call_expression", true, SJPreset::Args, false, false, ",", "", {"argument_list"}},
    {"enum_specifier", true, SJPreset::Args, false, false, ",", "", {"enumerator_list"}},
    // C++ extras
    {"template_argument_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"template_parameter_list", false, SJPreset::Args, false, false, ",", "", {}},
    {"template_declaration", true, SJPreset::Args, false, false, ",", "", {"template_parameter_list"}},
    {"template_type", true, SJPreset::Args, false, false, ",", "", {"template_argument_list"}},
};

static const SJRule sj_haskell[] = {
    {"list", false, SJPreset::Args, false, false, ",", "", {}},
};

static const SJRule sj_yaml[] = {
    {"flow_sequence", false, SJPreset::List, false, true, ",", "", {}},
    {"flow_mapping", false, SJPreset::Dict, true, true, ",", "", {}},
    {"block_sequence", false, SJPreset::List, true, true, ",", "", {}},
    {"block_mapping", false, SJPreset::Dict, true, true, ",", "", {}},
};

ConstArrayView<SJRule> get_splitjoin_rules(StringView language)
{
    if (language == "bash") return sj_bash;
    if (language == "c") return sj_c;
    if (language == "cpp") return sj_cpp;
    if (language == "css") return sj_css;
    if (language == "dart") return sj_dart;
    if (language == "elixir") return sj_elixir;
    if (language == "go") return sj_go;
    if (language == "haskell") return sj_haskell;
    if (language == "html") return sj_html;
    if (language == "java") return sj_java;
    if (language == "javascript") return sj_javascript;
    if (language == "json") return sj_json;
    if (language == "julia") return sj_julia;
    if (language == "kotlin") return sj_kotlin;
    if (language == "lua") return sj_lua;
    if (language == "nix") return sj_nix;
    if (language == "perl") return sj_perl;
    if (language == "php") return sj_php;
    if (language == "pug") return sj_pug;
    if (language == "python") return sj_python;
    if (language == "r") return sj_r;
    if (language == "ruby") return sj_ruby;
    if (language == "rust") return sj_rust;
    if (language == "sql") return sj_sql;
    if (language == "starlark") return sj_starlark;
    if (language == "terraform") return sj_terraform;
    if (language == "toml") return sj_toml;
    if (language == "typst") return sj_typst;
    if (language == "yaml") return sj_yaml;
    if (language == "zig") return sj_zig;
    // Aliases — languages that share rules with another language
    if (language == "json5") return sj_json;
    if (language == "jsonc") return sj_json;
    if (language == "php_only") return sj_php;
    if (language == "scss") return sj_css;
    if (language == "svelte") return sj_html;
    if (language == "vue") return sj_html;
    if (language == "tsx") return sj_javascript;
    if (language == "typescript") return sj_javascript;
    if (language == "zsh") return sj_bash;
    return {};
}

} // namespace Kakoune
