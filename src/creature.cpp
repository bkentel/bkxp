#include "creature.hpp"
#include "renderer.hpp"
#include "map.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::creature
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
void bkrl::creature::draw(renderer& render) const
{
    render.draw_cell(x(pos_), y(pos_), 1);
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::advance(random_state& random, map& m)
{
    auto& rnd = random[random_stream::creature];
    
    if (!x_in_y_chance(rnd, 1, 3)) {
        return;
    }

    int const dx = random_range(rnd, -1, 1);
    int const dy = random_range(rnd, -1, 1);

    m.move_creature_by(*this, bklib::ivec2 {dx, dy});
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::is_player() const noexcept
{
    return true;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::move_by(bklib::ivec2 const v)
{
    pos_ += v;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::move_by(int const dx, int const dy)
{
    return move_by(bklib::ivec2 {dx, dy});
}

//--------------------------------------------------------------------------------------------------
bklib::ipoint2 bkrl::creature::position() const noexcept
{
    return pos_;
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_instance_id bkrl::creature::id() const noexcept
{
    return id_;
}

//--------------------------------------------------------------------------------------------------
bkrl::creature::creature(
    creature_instance_id const  id
  , creature_def         const& def
  , bklib::ipoint2       const  p
) : id_  {id}
  , def_ {idof(def)}
  , pos_ {p}
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::creature_factory
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bkrl::creature bkrl::creature_factory::create(
    random_state&        random
  , creature_def const&  def
  , bklib::ipoint2 const p
) {
    return creature {creature_instance_id {++next_id_}, def, p};
}
