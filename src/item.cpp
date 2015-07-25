#include "item.hpp"
#include "terrain.hpp"
#include "context.hpp"
#include "map.hpp"
#include "bklib/dictionary.hpp"
#include "external/format.h"

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
  : data_  {}
  , flags_ {}
  , id_    {id}
  , def_   {get_id(def)}
{
    using namespace bklib::literals;

    if (has_tag(def.tags, make_tag("CORPSE"_hash))) {
        flags().set(item_flag::is_corpse);
    }
}

//--------------------------------------------------------------------------------------------------
bklib::utf8_string bkrl::item::friendly_name(definitions const& defs) const
{
    auto const id  = def();
    auto const def = defs.find(id);

    if (!def) {
        return to_string(id);
    }

    if (def->name.empty()) {
        return def->id_string;
    }

    if (flags().test(item_flag::is_corpse)) {
        auto const cid = def_id_t<tag_creature>(static_cast<uint32_t>(data()));
        if (!cid) {
            return fmt::sprintf("unknown remains");
        }

        if (auto const cdef = defs.find(cid)) {
            if (cdef->name.empty()) {
                return fmt::sprintf("the remains of a nameless entity");
            }

            return fmt::sprintf("the remains of a %s", cdef->name);
        }

        return fmt::sprintf("the remains of a %s", to_string(cid));
    }

    return def->name;
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

//--------------------------------------------------------------------------------------------------
bool bkrl::has_tag(item_def const& def, def_id_t<tag_string_tag> const tag)
{
    return has_tag(def.tags, tag);
}

//--------------------------------------------------------------------------------------------------
bool bkrl::has_tag(item const& c, item_dictionary const& defs, def_id_t<tag_string_tag> const tag)
{
    if (auto const def = defs.find(c.def())) {
        return has_tag(*def, tag);
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::has_tag(context const& ctx, item const& c, def_id_t<tag_string_tag> const tag)
{
    return has_tag(c, ctx.data.items(), tag);
}
