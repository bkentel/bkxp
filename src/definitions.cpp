#include "definitions.hpp"

#include "item.hpp"
#include "creature.hpp"
#include "color.hpp"

#include "bklib/dictionary.hpp"

bkrl::creature_def const*
bkrl::definitions::find(def_id_t<tag_creature> const id) const
{
    return creature_defs_ ? creature_defs_->find(id) : nullptr;
}

bkrl::item_def const*
bkrl::definitions::find(def_id_t<tag_item> const id) const
{
    return item_defs_ ? item_defs_->find(id) : nullptr;
}

bkrl::color_def const*
bkrl::definitions::find(def_id_t<tag_color> const id) const
{
    return color_defs_ ? color_defs_->find(id) : nullptr;
}
