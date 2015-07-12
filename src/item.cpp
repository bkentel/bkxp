#include "item.hpp"
#include "terrain.hpp"

#include "json.hpp"

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
    item_instance_id const  id
  , item_def         const& def
)
  : id_  {id}
  , def_ {get_id(def)}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::item bkrl::item_factory::create(random_t& random, item_def const& def)
{
    return item {item_instance_id {++next_id_}, def};
}

//--------------------------------------------------------------------------------------------------
void bkrl::load_definitions(item_dictionary& dic, bklib::utf8_string_view const data, detail::load_from_string_t)
{
    load_definitions<item_def>(data, [&](item_def const& def) {
        dic.insert_or_replace(def); // TODO duplicates
        return true;
    });
}
