#include "creature.hpp"
#include "map.hpp"
#include "context.hpp"
#include "output.hpp"

#include "bklib/dictionary.hpp"

#include <functional>

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
    return true;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::can_get_item(item const& i) const
{
    return true;
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
bkrl::creature::creature(
    instance_id_t<tag_creature> const  id
  , creature_def                const& def
  , bklib::ipoint2              const  p
) : id_  {id}
  , def_ {get_id(def)}
  , pos_ {p}
  , stats_ {}
  , items_ {}
  , flags_ {def.flags}
{
    stats_.hp_val.base = 5;
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
        break;
    }

    return 0;
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
    random_t&            random
  , creature_def const&  def
  , bklib::ipoint2 const p
) {
    return creature {instance_id_t<tag_creature> {++next_id_}, def, p};
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
    auto const att_info = ctx.data.find(att.def());
    auto const def_info = ctx.data.find(def.def());

    def.modify(stat_type::health, -1);

    auto const att_name = att_info ? att_info->name.c_str() : "player";
    auto const def_name = def_info ? def_info->name.c_str() : "player";
    ctx.out.write("The %s attacks the %s.", att_name, def_name);

    if (def.is_dead()) {
        ctx.out.write("The %s dies.", def_name);
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::advance(context& ctx, map& m, creature& c)
{
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
        [](bklib::ipoint2 const, creature const& c) {
            return c.is_dead();
        }
      , [&](bklib::ipoint2 const p, creature const&) {
            m.remove_creature_at(p);
        }
    );
}
