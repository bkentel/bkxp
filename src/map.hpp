#pragma once

#include "terrain.hpp"
#include "creature.hpp"
#include "item.hpp"
#include "identifier.hpp"
#include "random.hpp"

#include "bklib/math.hpp"
#include "bklib/spatial_map.hpp"

#include <array>
#include <bitset>
#include <vector>
#include <functional>
#include <cstdint>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

struct context;
class  renderer;
class  view;
struct color_def;

constexpr size_t size_block = 16;
constexpr size_t size_chunk = size_block * size_block;

//--------------------------------------------------------------------------------------------------
//! Base map data block 16 x 16 currently (see size_block)
//--------------------------------------------------------------------------------------------------
template <typename T>
struct block_t {
    T& cell_at(int const x, int const y) noexcept {
        auto const yi = static_cast<size_t>(y) % size_block;
        auto const xi = static_cast<size_t>(x) % size_block;

        return data[yi * size_block + xi];
    }

    T const& cell_at(int const x, int const y) const noexcept {
        return const_cast<block_t*>(this)->cell_at(x, y);
    }

    template <typename Function>
    void for_each_cell(Function&& f, int const x0, int const y0) const {
        for (auto i = 0u; i < data.size(); ++i) {
            auto const yi = i / size_block;
            auto const xi = i % size_block;
            f(x0 + xi, y0 + yi, data[i]);
        }
    }

    std::array<T, size_block * size_block> data;
};

//--------------------------------------------------------------------------------------------------
//! Map "chunk" consisting of 16 x 16 block_t currently (see size_chunk)
//--------------------------------------------------------------------------------------------------
template <typename T>
struct chunk_t {
    chunk_t() {
        data.resize(size_block * size_block);
    }

    block_t<T>& block_at(int const x, int const y) noexcept {
        auto const yi = static_cast<size_t>(y) / size_block;
        auto const xi = static_cast<size_t>(x) / size_block;
        return data[yi * size_block + xi];
    }

    T& cell_at(int const x, int const y) noexcept {
        auto const yb = static_cast<size_t>(y) / size_block;
        auto const yi = static_cast<size_t>(y) % size_block;
        auto const xb = static_cast<size_t>(x) / size_block;
        auto const xi = static_cast<size_t>(x) % size_block;

        return data[yb * size_block + xb].data[yi * size_block + xi];
    }

    T const& cell_at(int const x, int const y) const noexcept {
        return const_cast<chunk_t*>(this)->cell_at(x, y);
    }

    block_t<T> const& block_at(int const x, int const y) const noexcept {
        return const_cast<chunk_t*>(this)->block_at(x, y);
    }

    template <typename Function>
    void for_each_cell(Function&& f, int const x0, int const y0) const {
        for (auto i = 0u; i < data.size(); ++i) {
            auto const yi = i / size_block;
            auto const xi = i % size_block;
            data[i].for_each_cell(std::forward<Function>(f), x0 + xi * size_block, y0 + yi * size_block);
        }
    }

    std::vector<block_t<T>> data;
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename T, typename Function>
void for_each_cell(T&& block, Function&& f) {
    block.for_each_cell(std::forward<Function>(f), 0, 0);
}

template <typename T, typename Function>
void for_each_cell(T&& block, int const x, int const y, Function&& f) {
    block.for_each_cell(std::forward<Function>(f), x, y);
}

//----------------------------------------------------------------------------------------------
struct placement_result_t {
    constexpr placement_result_t() noexcept = default;
    constexpr placement_result_t(bklib::ipoint2 const p, bool const ok) noexcept
      : where (p), success {ok}
    {
    }

    constexpr explicit operator bool() const noexcept { return success; }

    operator bklib::ipoint2() const noexcept {
        BK_ASSERT(success);
        return where;
    }

    bklib::ipoint2 where   = bklib::ipoint2 {0, 0};
    bool           success {false};
};

inline bool operator==(placement_result_t const lhs, bklib::ipoint2 const rhs) noexcept {
    return lhs.where == rhs;
}

inline bool operator==(bklib::ipoint2 const lhs, placement_result_t const rhs) noexcept {
    return rhs == lhs;
}

inline bool operator!=(placement_result_t const lhs, bklib::ipoint2 const rhs) noexcept {
    return !(lhs == rhs);
}

inline bool operator!=(bklib::ipoint2 const lhs, placement_result_t const rhs) noexcept {
    return !(rhs == lhs);
}

struct room_data_t {
    bklib::irect region;
    uint64_t     data;
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
class map {
public:
    map();
    explicit map(context& ctx);
    ~map();

    void set_draw_colors(bklib::dictionary<color_def> const& colors);
    void draw(renderer& render, view const& v) const;
    void advance(context& ctx);

    void move_creature_to(creature& c, bklib::ipoint2 p);
    void move_creature_to(instance_id_t<tag_creature> id, bklib::ipoint2 p);

    creature const* find_creature(std::function<bool (creature const&)> const& predicate) const;
    creature* find_creature(std::function<bool (creature const&)> const& predicate);

    //----------------------------------------------------------------------------------------------
    //! @pre @p p must be a valid map position.
    //----------------------------------------------------------------------------------------------
    item_pile* place_item_at(item&& itm, item_def const& def, bklib::ipoint2 p);
    item_pile* place_items_at(definitions const& defs, item_pile&& pile, bklib::ipoint2 p);

    //----------------------------------------------------------------------------------------------
    //! @pre @p p must be a valid map position.
    //! @pre a creature must not already exist at @p p.
    //----------------------------------------------------------------------------------------------
    void place_creature_at(creature&& c, creature_def const& def, bklib::ipoint2 p);

    void update_render_data(bklib::ipoint2 p);
    void update_render_data(int x, int y);
    void update_render_data();

    //----------------------------------------------------------------------------------------------
    //! @pre @p p must be a valid map position.
    //! @pre an item must exist at @p p.
    //----------------------------------------------------------------------------------------------
    item_pile remove_items_at(bklib::ipoint2 p);

    //----------------------------------------------------------------------------------------------
    //! @pre @p p must be a valid map position.
    //! @pre the item must exist at @p p.
    //----------------------------------------------------------------------------------------------
    item remove_item_at(bklib::ipoint2 p, int index);

    //----------------------------------------------------------------------------------------------
    //! @pre @p p must be a valid map position.
    //! @pre a creature must exist at @p p.
    //----------------------------------------------------------------------------------------------
    creature remove_creature_at(bklib::ipoint2 p);

    item_pile*       items_at(bklib::ipoint2 p);
    item_pile const* items_at(bklib::ipoint2 p) const;

    bool can_place_item_at(bklib::ipoint2 p) const;

    creature*       creature_at(bklib::ipoint2 p);
    creature const* creature_at(bklib::ipoint2 p) const;

    bool can_place_creature_at(bklib::ipoint2 p) const;

    bklib::irect bounds() const noexcept {
        return {0, 0, static_cast<int>(size_chunk), static_cast<int>(size_chunk)};
    }

    terrain_entry& at(int const x, int const y) noexcept {
        return terrain_entries_.block_at(x, y).cell_at(x, y);
    }

    terrain_entry const& at(int const x, int const y) const noexcept {
        return const_cast<map*>(this)->at(x, y);
    }

    terrain_entry const& at(bklib::ipoint2 const p) const noexcept { return const_cast<map*>(this)->at(p); }
    terrain_entry&       at(bklib::ipoint2 const p)       noexcept { return at(x(p), y(p)); }

    void fill(bklib::irect r, terrain_type value);
    void fill(bklib::irect r, terrain_type value, terrain_type border);

    void add_room(room_data_t data) {
        rooms_.push_back(std::move(data));
    }
private:
    class render_data_t;
    std::unique_ptr<render_data_t> render_data_;

    chunk_t<terrain_entry> terrain_entries_;

    creature_map creatures_;
    item_map     items_;
    std::vector<room_data_t> rooms_;
};

template <typename T>
using base_type_of_t = std::remove_const_t<
    std::remove_reference_t<
        std::remove_all_extents_t<T>
    >
>;

static constexpr int const x_off[] = {-1,  0,  1, -1,  1, -1,  0,  1, 0};
static constexpr int const y_off[] = {-1, -1, -1,  0,  0,  1,  1,  1, 0};

struct find_around_result {
    explicit operator bool() const noexcept { return count > 0; }

    int                 count; //!< number of results found.
    bklib::ipoint2      p;     //!< position of the last match.
    std::array<bool, 9> valid; //!<

    static int index_of(bklib::ivec2 const v) noexcept {
        auto const xi = x(v);
        auto const yi = y(v);

        for (auto i = 0u; i < 9u; ++i) {
            if (x_off[i] == xi && y_off[i] == yi) {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

    bool is_valid(bklib::ivec2 const v) const noexcept {
        auto const i = index_of(v);
        return i > 0 ? valid[static_cast<size_t>(i)] : false;
    }
};

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
template <typename Map, typename Predicate>
find_around_result find_around(Map&& m, bklib::ipoint2 const p, Predicate&& pred)
{
    find_around_result result {};

    auto const bounds = m.bounds();

    for (auto i = 0u; i < 9u; ++i) {
        auto const q = p + bklib::ivec2 {x_off[i], y_off[i]};
        if (!bklib::intersects(q, bounds) || !pred(m.at(q))) {
            continue;
        }

        ++result.count;
        result.p = q;
        result.valid[i] = true;
    }

    return result;
}

template <typename Map, typename Predicate>
find_around_result find_items_around(Map&& m, bklib::ipoint2 const p, Predicate&& pred)
{
    find_around_result result {};

    auto const bounds = m.bounds();

    for (auto i = 0u; i < 9u; ++i) {
        auto const q = p + bklib::ivec2 {x_off[i], y_off[i]};

        if (!bklib::intersects(q, bounds)) {
            continue;
        }

        auto* const pile = m.items_at(q);
        if (!pile) {
            continue;
        }

        if (!pred(*pile)) {
            continue;
        }

        ++result.count;
        result.p = q;
        result.valid[i] = true;
    }

    return result;
}

//----------------------------------------------------------------------------------------------
//! Predicate(bklib::point2)
//----------------------------------------------------------------------------------------------
template <typename Map, typename Predicate>
placement_result_t find_first_around(Map&& m, bklib::ipoint2 const p, Predicate&& pred)
{
    auto const bounds = m.bounds();

    for (auto i = 0u; i < 9u; ++i) {
        auto const q = p + bklib::ivec2 {x_off[i], y_off[i]};
        if (!bklib::intersects(q, bounds)) {
            continue;
        }

        if (pred(q)) {
            return placement_result_t {q, true};
        }
    }

    return placement_result_t {p, false};
}

bool can_place_at(map const& m, bklib::ipoint2 const p, creature const& c);
bool can_place_at(map const& m, bklib::ipoint2 const p, item const& c);

//----------------------------------------------------------------------------------------------
//! Call f(item_pile&) creating a new item_pile at @p in the map @p m if it doesn't already exist.
//! @pre @p p is a valid position in @p m.
//! @pre @p dic has valid entries for any new items added.
//----------------------------------------------------------------------------------------------
template <typename Map, typename Function>
item_pile* with_pile_at(definitions const& defs, Map&& m, bklib::ipoint2 const p, Function&& f)
{
    static_assert(std::is_same<map, base_type_of_t<Map>>::value, "");

    if (!m.can_place_item_at(p)) {
        return nullptr;
    }

    if (auto const maybe_pile = m.items_at(p)) {
        f(*maybe_pile);
        m.update_render_data(p);
        return maybe_pile;
    }

    item_pile pile;
    f(pile);
    return m.place_items_at(defs, std::move(pile), p);
}

//--------------------------------------------------------------------------------------------------
//! Update the state of a door at p.
//--------------------------------------------------------------------------------------------------
bool set_door_state(map& m, bklib::ipoint2 p, door::state state);

//----------------------------------------------------------------------------------------------
//! @pre @p p lies within the bounds of the map @p m
//! @return true if generation succeeded, false otherwise.
//----------------------------------------------------------------------------------------------
placement_result_t generate_creature(context& ctx, map& m, creature_def const& def, bklib::ipoint2 p);
placement_result_t generate_creature(context& ctx, map& m, creature_def const& def);

//----------------------------------------------------------------------------------------------
//! @pre @p p lies within the bounds of the map @p m.
//! @return true if generation succeeded, false otherwise.
//----------------------------------------------------------------------------------------------
placement_result_t generate_item(context& ctx, map& m, item_def const& def, bklib::ipoint2 p);
placement_result_t generate_item(context& ctx, map& m, item_def const& def);

void advance(context& ctx, map& m);

////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
