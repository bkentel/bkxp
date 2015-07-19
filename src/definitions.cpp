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

namespace {

template <typename T>
T const* random_definition(
    bkrl::random_state&               rnd
  , bkrl::random_stream         const stream
  , bklib::dictionary<T> const* const dic
) {
    return dic ? std::addressof(bkrl::random_element(rnd[stream], *dic)) : nullptr;
}

} //namespace

bkrl::creature_def const*
bkrl::definitions::random_creature(random_state& rnd, random_stream const stream) const
{
    return random_definition(rnd, stream, creature_defs_);
}

bkrl::item_def const*
bkrl::definitions::random_item(random_state& rnd, random_stream const stream) const
{
    return random_definition(rnd, stream, item_defs_);
}

bkrl::color_def const*
bkrl::definitions::random_color(random_state& rnd, random_stream const stream) const
{
    return random_definition(rnd, stream, color_defs_);
}
