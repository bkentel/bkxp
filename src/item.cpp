#include "item.hpp"
#include "terrain.hpp"
#include "context.hpp"
#include "map.hpp"
#include "bklib/dictionary.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::item
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bool bkrl::item::can_place_on(terrain_entry const&) const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
bkrl::item::item(
    instance_id_t<tag_item> const  id
  , item_def                const& def
)
  : id_  {id}
  , def_ {get_id(def)}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::item bkrl::item_factory::create(random_t& random, item_def const& def)
{
    return item {instance_id_t<tag_item> {++next_id_}, def};
}

//--------------------------------------------------------------------------------------------------
void bkrl::advance(context& ctx, map& m, item& item)
{
}

//--------------------------------------------------------------------------------------------------
void bkrl::advance(context& ctx, map& m, item_pile& items)
{
    for (auto& itm : items) {
        advance(ctx, m, itm);
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::advance(context& ctx, map& m, item_map& imap)
{
    imap.for_each_data([&](item_pile& items) {
        advance(ctx, m, items);
    });
}
