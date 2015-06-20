#pragma once

#include <bklib/math.hpp>

#include "terrain.hpp"
#include "creature.hpp"
#include "item.hpp"
#include "identifier.hpp"
#include "random.hpp"

#include <array>
#include <bitset>
#include <vector>
#include <functional>
#include <cstdint>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

class renderer;
class view;

constexpr size_t size_block = 16;
constexpr size_t size_chunk = size_block * size_block;

//--------------------------------------------------------------------------------------------------
//! Base map data block 16 x 16 currently (see size_block)
//--------------------------------------------------------------------------------------------------
template <typename T>
struct block_t {
    T& cell_at(int const x, int const y) noexcept {
        return data[(y % size_block) * size_block + (x % size_block)];
    }

    T const& cell_at(int const x, int const y) const noexcept {
        return const_cast<block_t*>(this)->cell_at(x, y);
    }

    template <typename Function>
    void for_each_cell(Function&& f, int const x0, int const y0) const {
        for (size_t i = 0; i < data.size(); ++i) {
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
        auto const yi = y / size_block;
        auto const xi = x / size_block;
        return data[yi * size_block + xi];
    }

    T& cell_at(int const x, int const y) noexcept {
        auto const yb = y / size_block;
        auto const yi = y % size_block;
        auto const xb = x / size_block;
        auto const xi = x % size_block;

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
        for (size_t i = 0; i < data.size(); ++i) {
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
void for_each_cell(T&& block, Function&& f, int const x = 0, int const y = 0) {
    block.for_each_cell(std::forward<Function>(f), x, y);
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
class map {
public:
    map();
    ~map();

    void draw(renderer& render, view const& v) const;
    void advance(random_state& random);

    void move_creature_to(creature& c, bklib::ipoint2 p);
    void move_creature_to(creature_instance_id id, bklib::ipoint2 p);

    creature const* find_creature(std::function<bool (creature const&)> const& predicate) const;
    creature* find_creature(std::function<bool (creature const&)> const& predicate);

    //----------------------------------------------------------------------------------------------
    //! @pre @p p must be a valid map position.
    //----------------------------------------------------------------------------------------------
    void place_item_at(item&& itm, item_def const& def, bklib::ipoint2 p);
    void place_item_at(random_state& random, item_def const& def, item_factory& factory, bklib::ipoint2 p);

    void place_items_at(item_dictionary const& dic, item_pile&& pile, bklib::ipoint2 p);

    template <typename Function>
    void with_pile_at(bklib::ipoint2 const p, Function&& f) {
        item_pile pile;
        f(pile);
        place_items_at(std::move(pile), p);
    }

    //----------------------------------------------------------------------------------------------
    //! @pre @p p must be a valid map position.
    //! @pre a creature must not already exist at @p p.
    //----------------------------------------------------------------------------------------------
    void place_creature_at(creature&& c, creature_def const& def, bklib::ipoint2 p);
    void place_creature_at(random_state& random, creature_def const& def, creature_factory& factory, bklib::ipoint2 p);

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
    //! @pre a creature must exist at @p p.
    //----------------------------------------------------------------------------------------------
    creature remove_creature_at(bklib::ipoint2 p);

    item_pile*       items_at(bklib::ipoint2 p);
    item_pile const* items_at(bklib::ipoint2 p) const;

    creature*       creature_at(bklib::ipoint2 p);
    creature const* creature_at(bklib::ipoint2 p) const;

    bklib::irect bounds() const noexcept {
        return {0, 0, size_chunk, size_chunk};
    }

    terrain_entry& at(int x, int y) {
        return terrain_entries_.block_at(x, y).cell_at(x, y);
    }

    terrain_entry const& at(int x, int y) const {
        return terrain_entries_.block_at(x, y).cell_at(x, y);
    }

    terrain_entry const& at(bklib::ipoint2 const p) const { return at(x(p), y(p)); }
    terrain_entry&       at(bklib::ipoint2 const p)       { return at(x(p), y(p)); }

    void fill(bklib::irect r, terrain_type value);
    void fill(bklib::irect r, terrain_type value, terrain_type border);

    struct find_around_result {
        int count;
        int x;
        int y;
        std::bitset<9> valid;
    };

    template <typename Predicate>
    find_around_result find_around(bklib::ipoint2 const& p, Predicate&& pred) const {
        constexpr int const dx[] = {-1,  0,  1, -1,  0,  1, -1,  0,  1};
        constexpr int const dy[] = {-1, -1, -1,  0,  0,  0,  1,  1,  1};

        find_around_result result {};

        auto const x0 = x(p);
        auto const y0 = y(p);

        for (int i = 0; i < 9; ++i) {
            auto const x1 = x0 + dx[i];
            auto const y1 = y0 + dy[i];

            if (x1 < 0 || x1 > size_chunk
             || y1 < 0 || y1 > size_chunk
             || !pred(at(x1, y1))
            ) {
                continue;
            }

            ++result.count;
            result.x = x1;
            result.y = y1;
            result.valid.set(i);
        }

        return result;
    }
private:
    class render_data_t;
    std::unique_ptr<render_data_t> render_data_;

    chunk_t<terrain_entry> terrain_entries_;

    creature_map creatures_;
    item_map     items_;
};

template <typename T>
using base_type_of_t = std::remove_const_t<
    std::remove_reference_t<
        std::remove_all_extents_t<T>
    >
>;

//----------------------------------------------------------------------------------------------
//! Call f(item_pile&) creating a new item_pile at @p in the map @m if it doesn't already exist.
//! @pre @p p is a valid position in @p m.
//! @pre @p dic has valid enties for any new items added.
//----------------------------------------------------------------------------------------------
template <typename Map, typename Function>
void with_pile_at(item_dictionary const& dic, Map&& m, bklib::ipoint2 const p, Function&& f)
{
    static_assert(std::is_same<map, base_type_of_t<Map>>::value, "");

    if (auto const maybe_pile = m.items_at(p)) {
        f(*maybe_pile);
        m.update_render_data(p);
    } else {
        item_pile pile;
        f(pile);
        m.place_items_at(dic, std::move(pile), p);
    }
}

void advance(random_state& random, map& m);

//----------------------------------------------------------------------------------------------
//! @pre @p p lies within the bounds of the map @p m
//! @return true if generation succeeded, false otherwise.
//----------------------------------------------------------------------------------------------
bool generate_creature(random_state& random, map& m, creature_factory& factory, creature_def_id def, bklib::ipoint2 p);
bool generate_creature(random_state& random, map& m, creature_factory& factory, creature_def_id def);
bool generate_creature(random_state& random, map& m, creature_factory& factory, creature_def const& def, bklib::ipoint2 p);
bool generate_creature(random_state& random, map& m, creature_factory& factory, creature_def const& def);

//----------------------------------------------------------------------------------------------
//! @pre @p p lies within the bounds of the map @p m.
//! @return true if generation succeeded, false otherwise.
//----------------------------------------------------------------------------------------------
bool generate_item(random_state& random, map& m, item_factory& factory, item_def_id def, bklib::ipoint2 p);
bool generate_item(random_state& random, map& m, item_factory& factory, item_def_id def);
bool generate_item(random_state& random, map& m, item_factory& factory, item_def const& def, bklib::ipoint2 p);
bool generate_item(random_state& random, map& m, item_factory& factory, item_def const& def);

////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
