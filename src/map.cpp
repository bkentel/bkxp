#include "map.hpp"
#include "renderer.hpp"
#include "view.hpp"

#include "bklib/algorithm.hpp"

namespace {

inline decltype(auto) find_creature_by_id(bkrl::creature_instance_id const id) noexcept {
    return [id](bkrl::creature const& c) noexcept {
        return c.id() == id;
    };
}

inline decltype(auto) find_by_pos(bklib::ipoint2 const p) noexcept {
    auto const x_pos = x(p);
    auto const y_pos = y(p);

    return [x_pos, y_pos](auto const& data) noexcept {
        return (data.x == x_pos) && (data.y == y_pos);
    };
}

} // namespace

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

class bkrl::map::render_data_t {
public:
    using point_t = bklib::ipoint2;

    void draw(renderer& render, bklib::irect const bounds, view const& v) const {
        auto const r = intersection(bounds, v.screen_to_world());
        if (!r) {
            return;
        }

        for (auto y = r.top; y < r.bottom; ++y) {
            for (auto x = r.left; x < r.right; ++x) {
                auto const& cell = terrain_data_.cell_at(x, y);

                if (cell.base_index) {
                    render.draw_cell(x, y, cell.base_index);
                }

            }
        }

        for (auto const& i : item_data_) {
            render.draw_cell(i.x, i.y, i.base_index);
        }

        for (auto const& c : creature_data_) {
            render.draw_cell(c.x, c.y, c.base_index);
        }
    }

    void update_creature_pos(point_t const from, point_t const to) {
        update_pos_(creature_data_, from, to);
    }

    void update_item_pos(point_t const from, point_t const to) {
        update_pos_(item_data_, from, to);
    }

    void update_or_add(item_def const& idef, point_t const p) {
        item_render_data_t data {
            static_cast<int16_t>(x(p))
          , static_cast<int16_t>(y(p))
          , static_cast<uint16_t>(idef.symbol[0])
          , {255, 255, 255, 255}
        };

        update_or_add_(item_data_, p, std::move(data));
    }

    void update_or_add(creature_def const& cdef, point_t const p) {
        creature_render_data_t data {
            static_cast<int16_t>(x(p))
          , static_cast<int16_t>(y(p))
          , static_cast<uint16_t>(cdef.symbol[0])
          , {255, 255, 255, 255}
        };

        update_or_add_(creature_data_, p, std::move(data));
    }

    void update_terrain(terrain_entry const& ter, point_t const p) {
        auto const x_pos = x(p);
        auto const y_pos = y(p);

        auto& index = terrain_data_.block_at(x_pos, y_pos).cell_at(x_pos, y_pos).base_index;

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

    void clear_item_at(point_t const p) {
        clear_at_(item_data_, p);
    }

    void clear_creature_at(point_t const p) {
        clear_at_(creature_data_, p);
    }
private:
    template <typename Container>
    static void clear_at_(Container& c, point_t const p) {
        auto const it = bklib::find_if(c, find_by_pos(p));
        BK_PRECONDITION(it != std::end(c));
        c.erase(it);
    }

    template <typename Container, typename T>
    static void update_or_add_(Container& c, point_t const p, T&& value) {
        if (auto const maybe = bklib::find_maybe(c, find_by_pos(p))) {
            *maybe = std::forward<T>(value);
        } else {
            c.push_back(std::forward<T>(value));
        }
    }

    template <typename Container>
    static void update_pos_(Container& c, point_t const from, point_t const to) {
        auto const ptr = bklib::find_maybe(c, find_by_pos(from));
        BK_ASSERT(ptr);

        ptr->x = static_cast<int16_t>(x(to));
        ptr->y = static_cast<int16_t>(y(to));
    }

    chunk_t<terrain_render_data_t>      terrain_data_;
    std::vector<creature_render_data_t> creature_data_;
    std::vector<item_render_data_t>     item_data_;
};

//--------------------------------------------------------------------------------------------------
bkrl::map::map()
  : render_data_ {std::make_unique<render_data_t>()}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::map::~map() = default;

//--------------------------------------------------------------------------------------------------
void bkrl::map::draw(renderer& render, view const& v) const
{
    render_data_->draw(render, bounds(), v);
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::advance(random_state& random)
{
    bkrl::advance(random, *this, creatures_);
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::move_creature_to(creature& c, bklib::ipoint2 const to)
{
    auto const from = c.position();

    if (!creatures_.relocate(from, to, c)) {
        BK_ASSERT(false);
    }

    c.move_to(to);
    render_data_->update_creature_pos(from, to);
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::move_creature_to(
    creature_instance_id const id
  , bklib::ipoint2       const p
) {
    auto const ptr = creatures_.find(find_creature_by_id(id));
    BK_ASSERT(ptr);

    move_creature_to(*ptr, p);
}

//--------------------------------------------------------------------------------------------------
bkrl::creature const* bkrl::map::find_creature(
    std::function<bool (creature const&)> const& predicate
) const {
    return const_cast<map*>(this)->find_creature(predicate);
}

//--------------------------------------------------------------------------------------------------
bkrl::creature* bkrl::map::find_creature(
    std::function<bool (creature const&)> const& predicate
) {
    return creatures_.find(predicate);
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::place_item_at(
    item&&               itm
  , item_def const&      def
  , bklib::ipoint2 const p
) {
    BK_PRECONDITION(intersects(p, bounds()));

    auto& pile = [&]() -> decltype(auto) {
        if (auto const existing = items_.at(p)) {
            return *existing;
        }

        return items_.insert(p, item_pile {});
    }();

    pile.insert(std::move(itm));
    render_data_->update_or_add(def, p);
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::place_item_at(
    random_state& random
  , item_def const& def
  , item_factory& factory
  , bklib::ipoint2 const p
) {
    place_item_at(factory.create(random, def), def, p);
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::place_items_at(item_dictionary const& dic, item_pile&& pile, bklib::ipoint2 const p)
{
    if (auto const maybe_items = items_at(p)) {
        move_items(pile, *maybe_items);
        return;
    }

    if (pile.empty()) {
        return;
    }

    auto const maybe_idef = dic.find(pile.begin()->def());
    BK_PRECONDITION(maybe_idef);

    render_data_->update_or_add(*maybe_idef, p);
    items_.insert(p, std::move(pile));
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::place_creature_at(
    creature&&           c
  , creature_def const&  def
  , bklib::ipoint2 const p
) {
    BK_PRECONDITION(intersects(p, bounds()));
    BK_PRECONDITION(!creature_at(p));

    creatures_.insert(p, std::move(c));
    render_data_->update_or_add(def, p);
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::place_creature_at(
    random_state& random
  , creature_def const& def
  , creature_factory& factory
  , bklib::ipoint2 p
) {
    place_creature_at(factory.create(random, def, p), def, p);
}

//--------------------------------------------------------------------------------------------------
bkrl::creature* bkrl::map::creature_at(bklib::ipoint2 const p)
{
    return creatures_.at(p);
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::fill(bklib::irect const r, terrain_type const value)
{
    fill(r, value, value);
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
    auto const& ter = terrain_entries_.block_at(x, y).cell_at(x, y);
    render_data_->update_terrain(ter, bklib::ipoint2 {x, y});
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::update_render_data()
{
    for (auto y = 0; y < static_cast<int>(size_chunk); ++y) {
        for (auto x = 0; x < static_cast<int>(size_chunk); ++x) {
            update_render_data(x, y);
        }
    }
}

//--------------------------------------------------------------------------------------------------
bkrl::item_pile bkrl::map::remove_items_at(bklib::ipoint2 const p)
{
    render_data_->clear_item_at(p);
    return items_.remove(p);
}

//--------------------------------------------------------------------------------------------------
bkrl::creature bkrl::map::remove_creature_at(bklib::ipoint2 const p)
{
    render_data_->clear_creature_at(p);
    return creatures_.remove(p);
}

//--------------------------------------------------------------------------------------------------
bkrl::item_pile* bkrl::map::items_at(bklib::ipoint2 const p)
{
    return items_.at(p);
}

//--------------------------------------------------------------------------------------------------
bkrl::item_pile const* bkrl::map::items_at(bklib::ipoint2 const p) const
{
    return items_.at(p);
}

//--------------------------------------------------------------------------------------------------
bkrl::creature const* bkrl::map::creature_at(bklib::ipoint2 const p) const
{
    return creatures_.at(p);
}

//--------------------------------------------------------------------------------------------------
bool bkrl::generate_creature(
    random_state&        random
  , map&                 m
  , creature_factory&    factory
  , creature_def const&  def
  , bklib::ipoint2 const p
) {
    if (m.creature_at(p)) {
        return false;
    }

    m.place_creature_at(random, def, factory, p);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::generate_creature(
    random_state&       random
  , map&                m
  , creature_factory&   factory
  , creature_def const& def
) {
    auto& rnd = random[random_stream::creature];

    for (int i = 0; i < 10; ++i) {
        bklib::ipoint2 const p {
            random_range(rnd, 0, 50)
          , random_range(rnd, 0, 50)
        };

        if (generate_creature(random, m, factory, def, p)) {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::generate_creature(
    random_state&         random
  , map&                  m
  , creature_factory&     factory
  , creature_def_id const def
  , bklib::ipoint2 const  p
) {
    if (auto const maybe_def = factory.dictionary().find(def)) {
        return generate_creature(random, m, factory, *maybe_def, p);
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::generate_creature(
    random_state&         random
  , map&                  m
  , creature_factory&     factory
  , creature_def_id const def
) {
    if (auto const maybe_def = factory.dictionary().find(def)) {
        return generate_creature(random, m, factory, *maybe_def);
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::generate_item(
    random_state&        random
  , map&                 m
  , item_factory&        factory
  , item_def const&      def
  , bklib::ipoint2 const p
) {
    m.place_item_at(random, def, factory, p);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::generate_item(
    random_state&   random
  , map&            m
  , item_factory&   factory
  , item_def const& def
) {
    auto& rnd = random[random_stream::item];

    bklib::ipoint2 const p {
        random_range(rnd, 0, 50)
      , random_range(rnd, 0, 50)
    };

    return generate_item(random, m, factory, def, p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
void bkrl::advance(random_state& random, map& m)
{
    m.advance(random);
}
