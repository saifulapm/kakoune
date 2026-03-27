#ifndef splitjoin_hh_INCLUDED
#define splitjoin_hh_INCLUDED

#include "string.hh"
#include "vector.hh"

namespace Kakoune
{

enum class SJPreset { Args, List, Dict, Statement, Default };

struct SJRule
{
    StringView node_type;
    bool is_redirect = false;

    // Direct rule:
    SJPreset preset = SJPreset::Args;
    bool last_separator = false;
    bool space_in_brackets = false;
    StringView separator = ",";
    StringView force_insert = "";  // appended on join (e.g. ";")

    // Redirect:
    Vector<StringView> targets;
};

ConstArrayView<SJRule> get_splitjoin_rules(StringView language);

} // namespace Kakoune

#endif
