#include "map.hpp"
#include "renderer.hpp"
#include "view.hpp"
#include "color.hpp"
#include "context.hpp"
#include "bsp_layout.hpp"

#include "bklib/algorithm.hpp"
#include "bklib/dictionary.hpp"

namespace {

inline decltype(auto)
find_creature_by_id(bkrl::instance_id_t<bkrl::tag_creature> const id) noexcept {
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
namespace {

template <typename T, typename Function>
void for_each_block_in(bkrl::chunk_t<T> const& chunk, bklib::irect const r, Function&& f)
{
    constexpr auto const w = static_cast<int>(bkrl::size_block);
    constexpr auto const h = static_cast<int>(bkrl::size_block);

    auto const rw = r.width();
    auto const blocks_x = rw / w + (r.left % w ? 1 : 0) + (rw % w ? 1 : 0);

    auto const rh = r.height();
    auto const blocks_y = rh / h + (r.top % h ? 1 : 0) + (rh % h ? 1 : 0);

    auto const x0 = w * (r.left / w);
    auto const y0 = h * (r.top  / h);

    for (auto yi = 0; (yi < blocks_y * h) && (yi < h * h - r.top); yi += h) {
        for (auto xi = 0; (xi < blocks_x * w) && (xi < w * w - r.left); xi += w) {
            f(chunk.block_at(r.left + xi, r.top + yi), xi + x0, yi + y0);
        }
    }
}
} //namespace

class bkrl::map::render_data_t {
public:
    using point_t = bklib::ipoint2;

    void set_draw_colors(color_dictionary const& colors) {
        colors_ = &colors;
    }

    void draw(renderer& render, bklib::irect const bounds, view const& v) const {
        auto const r = intersection(bounds, v.screen_to_world());
        if (!r) {
            return;
        }

        for_each_block_in(terrain_data_, r, [&](auto const& block, int const x, int const y) {
            auto const base = block.data.data();
            constexpr auto const off_texture = offsetof(terrain_render_data_t, base_index);
            constexpr auto const siz_texture = sizeof(terrain_render_data_t::base_index);
            constexpr auto const stride = sizeof(terrain_render_data_t);

            render.draw_cells(x, y, size_block, size_block, base, off_texture, siz_texture, stride);
        });

        for (auto const& i : item_data_) {
            render.draw_cell(i.x, i.y, i.base_index, i.color);
        }

        for (auto const& c : creature_data_) {
            render.draw_cell(c.x, c.y, c.base_index, c.color);
        }
    }

    void update_creature_pos(point_t const from, point_t const to) {
        update_pos_(creature_data_, from, to);
    }

    void update_item_pos(point_t const from, point_t const to) {
        update_pos_(item_data_, from, to);
    }

    void update_or_add(item_def const* idef, point_t const p) {
        constexpr uint16_t const fallback_symbol = '?';
        color4 const fallback_color = make_color(255, 0, 255);

        item_render_data_t data {
            static_cast<int16_t>(x(p))
          , static_cast<int16_t>(y(p))
          , static_cast<uint16_t>(idef ? idef->symbol[0] : fallback_symbol)
          , idef ? get_color_(*idef) : fallback_color
        };

        update_or_add_(item_data_, p, std::move(data));
    }

    void update_or_add(creature_def const* cdef, point_t const p) {
        constexpr uint16_t const fallback_symbol = '?';
        color4 const fallback_color = make_color(255, 0, 255);

        creature_render_data_t data {
            static_cast<int16_t>(x(p))
          , static_cast<int16_t>(y(p))
          , static_cast<uint16_t>(cdef ? cdef->symbol[0] : fallback_symbol)
          , cdef ? get_color_(*cdef) : fallback_color
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
    template <typename T>
    color4 get_color_(T const& def) const {
        auto default_color = color4 {255, 255, 255, 255};

        if (!colors_) {
            return default_color;
        }

        if (auto const cdef = colors_->find(def.symbol_color)) {
            return cdef->color;
        } else {
            // TODO log this
        }

        return default_color;
    }

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

    color_dictionary const* colors_ = nullptr;
};

//--------------------------------------------------------------------------------------------------
bkrl::map::map()
  : render_data_ {std::make_unique<render_data_t>()}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::map::map(context& ctx)
  : map {}
{
    set_draw_colors(ctx.data.colors());

    auto& random = ctx.random[random_stream::substantive];

    //
    // use slightly smaller bounds
    //
    auto const map_bounds = [&] {
        auto result = bounds();

        BK_ASSERT(result.width() > 1 && result.height() > 1);

        result.right  -= 1;
        result.bottom -= 1;

        return result;
    }();

    //
    // generate rooms
    //
    bsp_layout layout {map_bounds};
    layout.generate(random, [&](bklib::irect const region) {
        if (bkrl::x_in_y_chance(random, 7, 10)) {
            return;
        }

        bklib::irect const r {region.left, region.top, region.right + 1, region.bottom + 1};

        fill(r, terrain_type::floor, terrain_type::wall);
        add_room(room_data_t {region, uint64_t {}});
    });

    //
    // add items, creatures, and features
    //
    for (auto const& room : rooms_) {
        // add a random door
        auto const& region = room.region;
        bklib::irect const r {region.left, region.top, region.right + 1, region.bottom + 1};
        at(random_point_border(random, r)).type = terrain_type::door;

        if (bkrl::x_in_y_chance(random, 7, 10)) {
            auto const def = ctx.data.random_creature(ctx.random, random_stream::substantive);
            if (!def) {
                continue;
            }

            generate_creature(ctx, *this, *def, random_point(random, r));
        }

        if (bkrl::x_in_y_chance(random, 7, 10)) {
            auto const def = ctx.data.random_item(ctx.random, random_stream::substantive);
            if (!def) {
                continue;
            }

            generate_item(ctx, *this, *def, random_point(random, r));
        }
    }

    update_render_data();
}

//--------------------------------------------------------------------------------------------------
bkrl::map::~map() = default;

//--------------------------------------------------------------------------------------------------
void  bkrl::map::set_draw_colors(color_dictionary const& colors)
{
    render_data_->set_draw_colors(colors);
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::draw(renderer& render, view const& v) const
{
    render_data_->draw(render, bounds(), v);
}

//--------------------------------------------------------------------------------------------------
void bkrl::map::advance(context& ctx)
{
    bkrl::advance(ctx, *this, items_);
    bkrl::advance(ctx, *this, creatures_);
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
    instance_id_t<tag_creature> const id
  , bklib::ipoint2              const p
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
bkrl::item_pile* bkrl::map::place_item_at(
    item&&               itm
  , item_def const&      def
  , bklib::ipoint2 const p
) {
    BK_PRECONDITION(intersects(p, bounds()));
    BK_PRECONDITION(can_place_item_at(p));

    auto& pile = [&]() -> decltype(auto) {
        if (auto const existing = items_.at(p)) {
            return *existing;
        }

        return items_.insert(p, item_pile {});
    }();

    pile.insert(std::move(itm));
    render_data_->update_or_add(&def, p);

    return &pile;
}

//--------------------------------------------------------------------------------------------------
bkrl::item_pile* bkrl::map::place_items_at(definitions const& defs, item_pile&& pile, bklib::ipoint2 const p)
{
    if (auto const maybe_items = items_at(p)) {
        move_items(pile, *maybe_items);
        return maybe_items;
    }

    if (pile.empty()) {
        return nullptr;
    }

    auto const maybe_idef = defs.find(pile.begin()->def());

    render_data_->update_or_add(maybe_idef, p);
    return &items_.insert(p, std::move(pile));
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
    render_data_->update_or_add(&def, p);
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
bkrl::item bkrl::map::remove_item_at(bklib::ipoint2 const p, int const index)
{
    auto const pile = items_.at(p);
    BK_PRECONDITION(pile);

    auto result = pile->remove(index);

    if (pile->empty()) {
        items_.remove(p);
        render_data_->clear_item_at(p); // TODO should update symbol
    }

    return result;
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
bool bkrl::map::can_place_item_at(bklib::ipoint2 const p) const
{
    switch (at(p).type) {
    case terrain_type::floor: BK_FALLTHROUGH
    case terrain_type::stair:
        break;
    case terrain_type::empty: BK_FALLTHROUGH
    case terrain_type::rock: BK_FALLTHROUGH
    case terrain_type::wall: BK_FALLTHROUGH
    case terrain_type::door: BK_FALLTHROUGH
    default:
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bkrl::creature const* bkrl::map::creature_at(bklib::ipoint2 const p) const
{
    return creatures_.at(p);
}

//--------------------------------------------------------------------------------------------------
bool bkrl::map::can_place_creature_at(bklib::ipoint2 const p) const
{
    switch (at(p).type) {
    case terrain_type::floor: BK_FALLTHROUGH
    case terrain_type::stair:
        break;
    case terrain_type::empty: BK_FALLTHROUGH
    case terrain_type::rock: BK_FALLTHROUGH
    case terrain_type::wall: BK_FALLTHROUGH
    case terrain_type::door: BK_FALLTHROUGH
    default:
        return false;
    }

    return !creature_at(p);
}

//--------------------------------------------------------------------------------------------------
bkrl::placement_result_t
bkrl::generate_creature(context& ctx, map& m, creature_def const& def, bklib::ipoint2 const p) {
    auto& random = ctx.random[random_stream::creature];

    auto c = ctx.cfactory.create(random, def, p);

    auto const result = find_first_around(m, p, [&](bklib::ipoint2 const q) {
        return can_place_at(m, q, c);
    });

    if (!result) {
        return result;
    }

    c.move_to(result);

    if (auto const idef = ctx.data.random_item(ctx.random, random_stream::creature)) {
        auto itm = ctx.ifactory.create(random, ctx.data.items(), *idef);
        c.get_item(std::move(itm));
    }

    m.place_creature_at(std::move(c), def, result);

    return result;
}

//--------------------------------------------------------------------------------------------------
bkrl::placement_result_t
bkrl::generate_creature(context& ctx, map& m, creature_def const& def)
{
    return generate_creature(ctx, m, def, random_point(ctx.random[random_stream::substantive], m.bounds()));
}

//--------------------------------------------------------------------------------------------------
bkrl::placement_result_t
bkrl::generate_item(context& ctx, map& m, item_def const& def, bklib::ipoint2 const p) {
    auto i = ctx.ifactory.create(ctx.random[random_stream::substantive], ctx.data.items(), def);

    auto const result = find_first_around(m, p, [&](bklib::ipoint2 const q) {
        return can_place_at(m, q, i);
    });

    if (!!result) {
        BK_ASSERT(can_place_at(m, result, i));
        m.place_item_at(std::move(i), def, result);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
bkrl::placement_result_t
bkrl::generate_item(context& ctx, map& m, item_def const& def)
{
    return generate_item(ctx, m, def, random_point(ctx.random[random_stream::substantive], m.bounds()));
}

//--------------------------------------------------------------------------------------------------
bool bkrl::can_place_at(map const& m, bklib::ipoint2 const p, creature const& c)
{
    return m.can_place_creature_at(p) && c.can_enter_terrain(m.at(p));
}

//--------------------------------------------------------------------------------------------------
bool bkrl::can_place_at(map const& m, bklib::ipoint2 const p, item const& i)
{
    return m.can_place_item_at(p) && i.can_place_on(m.at(p));
}

//--------------------------------------------------------------------------------------------------
void bkrl::advance(context& ctx, map& m)
{
    m.advance(ctx);
}

//--------------------------------------------------------------------------------------------------
bool bkrl::set_door_state(map& m, bklib::ipoint2 const p, door::state const state)
{
    auto& ter = m.at(p);
    door d {ter};

    if (!d.set_open_close(state)) {
        return false;
    }

    ter = d;
    m.update_render_data(p);

    return true;
}
