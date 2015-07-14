#pragma once

#include "identifier.hpp"

#include "bklib/string.hpp"
#include "bklib/utility.hpp"

#include <vector>

namespace bklib { template <typename T> class dictionary; }

namespace bkrl {

struct creature_def;
struct item_def;
struct color_def;

class definitions {
public:
    definitions(bklib::dictionary<creature_def>* const creatures
              , bklib::dictionary<item_def>*     const items
              , bklib::dictionary<color_def>*    const colors
    ) : creature_defs_ {creatures}
      , item_defs_     {items}
      , color_defs_    {colors}
    {
    }

    creature_def const* find(def_id_t<tag_creature> id) const;
    item_def     const* find(def_id_t<tag_item>     id) const;
    color_def    const* find(def_id_t<tag_color>    id) const;
private:
    bklib::dictionary<creature_def>* creature_defs_;
    bklib::dictionary<item_def>*     item_defs_;
    bklib::dictionary<color_def>*    color_defs_;
};

using tag_list = std::vector<bkrl::def_id_t<tag_string_tag>>;

//--------------------------------------------------------------------------------------------------
struct definition_base {
    explicit definition_base(bklib::utf8_string&& def_id_string)
      : id_string {std::move(def_id_string)}
    {
    }

    bklib::utf8_string  id_string;
    bklib::utf8_string  name;
    bklib::utf8_string  description;
    bklib::utf8_string  symbol;
    def_id_t<tag_color> symbol_color;
    tag_list            tags;
};

} //namespace bkrl
