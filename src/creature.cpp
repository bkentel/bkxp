#include "creature.hpp"
#include "map.hpp"
#include "context.hpp"

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
bool bkrl::move_by(creature& c, map& m, bklib::ivec2 const v)
{
    BK_ASSERT(std::abs(x(v)) <= 1);
    BK_ASSERT(std::abs(y(v)) <= 1);

    auto const from = c.position();
    auto const to   = c.position() + v;

    if (!intersects(m.bounds(), to)) {
        return false;
    }

    auto const& ter = m.at(to);
    if (!c.can_enter_terrain(ter)) {
        return false;
    }

    if (m.creature_at(to)) {
        return false;
    }

    m.move_creature_to(c, to);

    return true;
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

    move_by(c, m, v);
}

//--------------------------------------------------------------------------------------------------
void bkrl::advance(context& ctx, map& m, creature_map& cmap)
{
    cmap.for_each_data([&](creature& c) {
        advance(ctx, m, c);
    });
}
