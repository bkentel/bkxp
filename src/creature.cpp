#include "creature.hpp"
#include "map.hpp"
#include "context.hpp"
#include "output.hpp"

#include "bklib/dictionary.hpp"

#include <functional>

void bkrl::process_tags(creature_def&)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::creature
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::is_player() const noexcept
{
    return flags_.test(creature_flag::is_player);
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::is_dead() const noexcept
{
    return stats_.hp_val.value() <= 0;
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::move_by(bklib::ivec2 const v) noexcept
{
    pos_ += v;
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::move_to(bklib::ipoint2 const p) noexcept
{
    pos_ = p;
}

//--------------------------------------------------------------------------------------------------
bklib::ipoint2 bkrl::creature::position() const noexcept
{
    return pos_;
}

//--------------------------------------------------------------------------------------------------
bkrl::instance_id_t<bkrl::tag_creature> bkrl::creature::id() const noexcept
{
    return id_;
}

//--------------------------------------------------------------------------------------------------
bkrl::def_id_t<bkrl::tag_creature> bkrl::creature::def() const noexcept
{
    return def_;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::can_get_items(item_pile const& ip) const
{
    auto const total_weight = std::accumulate(
        std::begin(ip), std::end(ip), 0
      , [](auto const n, item const& i) noexcept { return n + i.weight(); }
    );

    //TODO

    return total_weight < 1000;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::can_get_item(item const& i) const
{
    return i.weight() < 1000;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::can_enter_terrain(terrain_entry const& ter) const
{
    switch (ter.type) {
    case terrain_type::stair: BK_FALLTHROUGH
    case terrain_type::empty: BK_FALLTHROUGH
    case terrain_type::floor:
        break;
    case terrain_type::door:
        if (!door {ter}.is_open()) {
            return false;
        }
        break;
    case terrain_type::wall: BK_FALLTHROUGH
    case terrain_type::rock: BK_FALLTHROUGH
    default:
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::get_item(item&& i)
{
    items_.insert(std::move(i));
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::get_items(item_pile&& ip)
{
    items_.insert(std::move(ip));
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::drop_item(item_pile& dst, int const i)
{
    move_item(items_, dst, i);
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::drop_item(item_pile& dst, item_pile::iterator const it)
{
    move_item(items_, dst, it);
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::drop_items(item_pile& dst)
{
    move_items(items_, dst);
}

//--------------------------------------------------------------------------------------------------
bkrl::equipment::result_t bkrl::creature::equip_item(item& i)
{
    BK_PRECONDITION(has_item(i));
    return equip_list().equip(i);
}

//--------------------------------------------------------------------------------------------------
bkrl::equipment::result_t bkrl::creature::equip_item(int const index)
{
    BK_PRECONDITION(!!item_list().checked_advance(index));
    return equip_list().equip(item_list().advance(index));
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::has_item(item const& i) const
{
    return !!bklib::find_maybe(items_, find_by_id(i.id()));
}

//--------------------------------------------------------------------------------------------------
bkrl::creature::creature(
    instance_id_t<tag_creature> const  id
  , creature_def                const& def
  , bklib::ipoint2              const  p
) : id_  {id}
  , def_ {get_id(def)}
  , pos_ (p)
  , stats_ {}
  , items_ {}
  , flags_ (def.flags)
{
}

//--------------------------------------------------------------------------------------------------
int bkrl::creature::modify(stat_type const stat, int const mod)
{
    using st = bkrl::stat_type;

    auto const set_new_mod = [mod](auto& s) {
        s.modify(mod);
        return s.value();
    };

    switch (stat) {
    case st::health:
        return set_new_mod(stats_.hp_val);
    case st::stamina:
        return set_new_mod(stats_.sp_val);
    case st::mana:
        return set_new_mod(stats_.mp_val);
    case st::strength:
        return set_new_mod(stats_.str_val);
    case st::constitution:
        return set_new_mod(stats_.con_val);
    case st::dexterity:
        return set_new_mod(stats_.dex_val);
    case st::intelligence:
        return set_new_mod(stats_.int_val);
    case st::wisdom:
        return set_new_mod(stats_.wis_val);
    case st::charisma:
        return set_new_mod(stats_.cha_val);
    case st::luck:
        return set_new_mod(stats_.luc_val);
    default:
        BK_UNREACHABLE;
    }

    BK_UNREACHABLE;
}
//--------------------------------------------------------------------------------------------------
int bkrl::creature::current(stat_type stat) const noexcept
{
    using st = bkrl::stat_type;

    switch (stat) {
    case st::health:       return stats_.hp_val.value();
    case st::stamina:      return stats_.sp_val.value();
    case st::mana:         return stats_.mp_val.value();
    case st::strength:     return stats_.str_val.value();
    case st::constitution: return stats_.con_val.value();
    case st::dexterity:    return stats_.dex_val.value();
    case st::intelligence: return stats_.int_val.value();
    case st::wisdom:       return stats_.wis_val.value();
    case st::charisma:     return stats_.cha_val.value();
    case st::luck:         return stats_.luc_val.value();
    default:               BK_UNREACHABLE;
    }

    BK_UNREACHABLE;
}

//--------------------------------------------------------------------------------------------------
bklib::utf8_string bkrl::creature::friendly_name(context const& ctx) const
{
    auto const id  = def();
    auto const def = ctx.data.find(id);

    if (!def) {
        return to_string(id);
    }

    if (def->name.empty()) {
        return def->id_string;
    }

    return def->name;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::creature_factory
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bkrl::creature_factory::creature_factory()
  : next_id_ {0}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_factory::~creature_factory() noexcept = default;

//--------------------------------------------------------------------------------------------------
bkrl::creature bkrl::creature_factory::create(
    random_t&             random
  , creature_def   const& def
  , bklib::ipoint2 const  p
) {
    creature result {instance_id_t<tag_creature> {++next_id_}, def, p};

    result.stats_.hp_val.base = bklib::clamp_to<int16_t>(def.stat_hp.generate(random));
    if (result.stats_.hp_val.base <= 0) {
        result.stats_.hp_val.base = 1;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bool bkrl::move_by(context& ctx, creature& c, map& m, bklib::ivec2 const v)
{
    BK_ASSERT(std::abs(x(v)) <= 1);
    BK_ASSERT(std::abs(y(v)) <= 1);

    auto const from = c.position();
    auto const to   = c.position() + v;

    if (from == to) {
        return true;
    }

    if (!intersects(m.bounds(), to)) {
        return false;
    }

    auto const& ter = m.at(to);
    if (!c.can_enter_terrain(ter)) {
        return false;
    }

    if (auto const other = m.creature_at(to)) {
        attack(ctx, m, c, *other);
        return true;
    }

    m.move_creature_to(c, to);

    return true;
}

//--------------------------------------------------------------------------------------------------
void bkrl::attack(context& ctx, map& m, creature& att, creature& def)
{
    //TODO consider removing the m parameter

    auto const att_info = ctx.data.find(att.def());
    auto const def_info = ctx.data.find(def.def());

    def.modify(stat_type::health, -1);

    auto const att_name = att_info ? att_info->name.c_str() : "player";
    auto const def_name = def_info ? def_info->name.c_str() : "player";
    ctx.out.write("The {} attacks the {}.", att_name, def_name);
}

//--------------------------------------------------------------------------------------------------
void bkrl::advance(context& ctx, map& m, creature& c)
{
    if (c.is_dead()) {
        return;
    }

    if (c.is_player()) {
        return;
    }

    auto& random = ctx.random[random_stream::creature];

    if (!x_in_y_chance(random, 1, 3)) {
        return;
    }

    bklib::ivec2 const v {
        random_range(random, -1, 1)
      , random_range(random, -1, 1)
    };

    move_by(ctx, c, m, v);
}

//--------------------------------------------------------------------------------------------------
void bkrl::advance(context& ctx, map& m, creature_map& cmap)
{
    cmap.for_each_data([&](creature& c) {
        advance(ctx, m, c);
    });

    cmap.remove_if(
        [ ](bklib::ipoint2, creature const& c) { return c.is_dead(); }
      , [&](bklib::ipoint2, creature&       c) { kill(ctx, m, c); }
    );
}

//--------------------------------------------------------------------------------------------------
void bkrl::kill(context& ctx, map& m, creature& c)
{
    if (!c.is_dead()) {
        BK_ASSERT(false); //TODO
    }

    auto const& name = c.friendly_name(ctx);
    ctx.out.write("The {} dies.", name);

    if (!make_corpse(ctx, m, c)) {
        ctx.out.write("The {} evaporates into nothingness.", name);
    }

    if (!drop_all(ctx, m, c)) {
        BK_DEBUG_BREAK; //TODO
    }

    m.remove_creature_at(c.position());
}

//--------------------------------------------------------------------------------------------------
bkrl::item_pile* bkrl::drop_all(context& ctx, map& m, creature& c)
{
    return bkrl::with_pile_at(ctx.data, m, c.position(), [&](item_pile& pile) {
        c.drop_items(pile);
    });
}

//--------------------------------------------------------------------------------------------------
bkrl::item_pile* bkrl::make_corpse(context& ctx, map& m, creature const& c)
{
    using namespace bklib::literals;

    if (has_tag(ctx, c, make_tag("NO_CORPSE"_hash))) {
        return nullptr;
    }

    return bkrl::with_pile_at(ctx.data, m, c.position(), [&](item_pile& pile) {
        auto const corpse_def = ctx.data.find(def_id_t<tag_item> {"CORPSE"});
        if (!corpse_def) {
            BK_ASSERT(false); //TODO
        }

        auto corpse = ctx.ifactory.create(ctx.random[random_stream::item], ctx.data.items(), *corpse_def);
        corpse.data() = item_data_t {
            item_data_type::corpse
          , static_cast<uint32_t>(c.def())
        };
        corpse.flags().set(item_flag::is_corpse);

        pile.insert(std::move(corpse));
    });
}

//--------------------------------------------------------------------------------------------------
bool bkrl::has_tag(creature_def const& def, def_id_t<tag_string_tag> const tag)
{
    return has_tag(def.tags, tag);
}

//--------------------------------------------------------------------------------------------------
bool bkrl::has_tag(creature const& c, creature_dictionary const& defs, def_id_t<tag_string_tag> const tag)
{
    if (auto const def = defs.find(c.def())) {
        return has_tag(*def, tag);
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::has_tag(context const& ctx, creature const& c, def_id_t<tag_string_tag> const tag)
{
    return has_tag(c, ctx.data.creatures(), tag);
}
