#include "map.hpp"
#include "renderer.hpp"
#include "view.hpp"

#include "bklib/algorithm.hpp"

namespace {

inline decltype(auto) find_creature_by_id(bkrl::creature_instance_id const id) {
    return [id](bkrl::creature const& c) {
        return c.id() == id;
    };
}

} // namespace

//--------------------------------------------------------------------------------------------------
void bkrl::map::draw(renderer& render, view const& v) const
{
    auto const r = intersection(bounds(), v.screen_to_world());
    if (!r) {
        return;
    }

    for (auto y = r.top; y < r.bottom; ++y) {
        for (auto x = r.left; x < r.right; ++x) {
            auto const& cell = terrain_render_data_.cell_at(x, y);

            if (cell.base_index) {
                render.draw_cell(x, y, cell.base_index);
            }

        }
    }

    items_.for_each_at(r, [&](bklib::ipoint2 const& p, item_pile const& pile) {
        if (!pile.empty()) {
            pile.begin()->draw(render, p);
        }
    });

    for (auto const& c : creature_render_data_) {
        render.draw_cell(c.x, c.y, c.base_index);
    }

    //creatures_.for_each_at(r, [&](bklib::ipoint2 const& p, creature const& c) {
    //    c.draw(render);
    //});
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::advance(random_state& random)
{
    creatures_.for_each_data([&](creature& c) {
        c.advance(random, *this);
    });
}

//--------------------------------------------------------------------------------------------------
bool bkrl::map::move_creature_by(creature& c, bklib::ivec2 const v)
{
    BK_ASSERT(std::abs(x(v)) <= 1);
    BK_ASSERT(std::abs(y(v)) <= 1);

    auto const p = c.position();
    auto const q = c.position() + v;
    if (!intersects(bounds(), q)) {
        return false;
    }

    auto const& ter = at(q);
    
    switch (ter.type) {
    case terrain_type::empty:
    case terrain_type::floor:
        break;
    case terrain_type::door:
        if (!door {ter}.is_open()) {
            return false;
        }
        break;
    default:
        return false;
    }

    if (creatures_.at(q)) {
        return false;
    }

    if (!c.move_by(v)) {
        return false;
    }

    // TODO update this to take the player into consideration
    creatures_.relocate(p, q, c);

    auto const x_pos = x(p);
    auto const y_pos = y(p);

    if (auto const ptr = bklib::find_maybe(
        creature_render_data_, [=](creature_render_data_t const& data) {
            return (data.x == x_pos) && (data.y == y_pos);
        }
    )) {
        ptr->x = static_cast<int16_t>(x(q));
        ptr->y = static_cast<int16_t>(y(q));
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::map::move_creature_to(creature& c, bklib::ipoint2 const p)
{
    return move_creature_by(c, p - c.position());
}

//--------------------------------------------------------------------------------------------------
bool bkrl::map::move_creature_by(
    creature_instance_id const id
  , bklib::ivec2         const v
) {
    auto const ptr = creatures_.find(find_creature_by_id(id));
    return ptr ? move_creature_by(*ptr, v) : false;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::map::move_creature_to(
    creature_instance_id const id
  , bklib::ipoint2       const p
) {
    auto const ptr = creatures_.find(find_creature_by_id(id));
    return ptr ? move_creature_to(*ptr, p) : false;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::map::generate_creature(
    random_state&        random
  , creature_factory&    factory
  , creature_def const&  def
  , bklib::ipoint2 const p
) {
    if (!intersects(p, bounds())) {
        return false;
    }
    
    if (creatures_.at(p)) {
        return false;
    }

    creatures_.insert(p, factory.create(random, def, p));

    creature_render_data_.push_back (creature_render_data_t {
        static_cast<int16_t>(x(p))
      , static_cast<int16_t>(y(p))
      , static_cast<uint16_t>(def.symbol[0])
      , {255, 255, 255, 255}
    });

    return true;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::map::generate_creature(
    random_state&       random
  , creature_factory&   factory
  , creature_def const& def
) {
    auto& rnd = random[random_stream::creature];

    for (int i = 0; i < 10; ++i) {
        bklib::ipoint2 const p {
            random_range(rnd, 0, 50)
          , random_range(rnd, 0, 50)
        };

        if (generate_creature(random, factory, def, p)) {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::map::generate_item(
    random_state& random
  , item_factory& factory
  , item_def const& def
) {
    auto& rnd = random[random_stream::item];

    bklib::ipoint2 const p {
        random_range(rnd, 0, 50)
      , random_range(rnd, 0, 50)
    };

    auto& pile = [&]() -> decltype(auto) {
        if (auto const existing = items_.at(p)) {
            return *existing;
        }

        return items_.insert(p, item_pile {});
    }();

    pile.insert(factory.create(random, def));

    return true;
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::fill(bklib::irect const r, terrain_type const value)
{
    for (int y = r.top; y < r.bottom; ++y) {
        for (int x = r.left; x < r.right; ++x) {
            auto& cell = terrain_entries_.block_at(x, y).cell_at(x, y);
            cell.type = value;
            cell.variant = 0;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::fill(bklib::irect r, terrain_type const value, terrain_type const border)
{
    for (int y = r.top; y < r.bottom; ++y) {
        for (int x = r.left; x < r.right; ++x) {
            auto& cell = at(x, y);
            
            if (y == r.top  || y == r.bottom - 1
             || x == r.left || x == r.right - 1
            ) {
                cell.type = border;
            } else {
                cell.type = value;
            }

            cell.variant = 0;
        }
    }
}


//--------------------------------------------------------------------------------------------------
void bkrl::map::update_render_data(bklib::ipoint2 const p)
{
    update_render_data(x(p), y(p));
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::update_render_data(int const x, int const y)
{
    auto& index = terrain_render_data_.block_at(x, y).cell_at(x, y).base_index;

    auto const& ter = terrain_entries_.block_at(x, y).cell_at(x, y);

    switch (ter.type) {
    default :
    case terrain_type::empty : index = 0;   break;
    case terrain_type::rock  : index = '*'; break;
    case terrain_type::stair : index = '>'; break;
    case terrain_type::floor : index = '.'; break;
    case terrain_type::wall  : index = '#'; break;
    case terrain_type::door:
        index = (door {ter}.is_open()) ? '\\' : '+';
        break;
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::update_render_data()
{
    for (int y = 0; y < size_chunk; ++y) {
        for (int x = 0; x < size_chunk; ++x) {
            update_render_data(x, y);
        }
    }
}
