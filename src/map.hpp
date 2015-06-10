#pragma once

#include <bklib/math.hpp>

#include "terrain.hpp"
#include "creature.hpp"
#include "item.hpp"
#include "identifier.hpp"
#include "random.hpp"

#include <array>
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
    void draw(renderer& render, view const& v) const;
    void advance(random_state& random);

    bool move_creature_by(creature& c, bklib::ivec2 v);
    bool move_creature_to(creature& c, bklib::ipoint2 p);
    bool move_creature_by(creature_instance_id id, bklib::ivec2 v);
    bool move_creature_to(creature_instance_id id, bklib::ipoint2 p);

    void generate_creature(random_state& random, creature_factory& factory, creature_def const& def);
    void generate_item(random_state& random, item_factory& factory, item_def const& def);

    void update_render_data(int x, int y);
    void update_render_data();

    item_pile remove_items_at(bklib::ipoint2 const p) {
        return items_.remove(p);
    }

    item_pile* items_at(bklib::ipoint2 const p) {
        return items_.at(p);
    }

    item_pile const* items_at(bklib::ipoint2 const p) const {
        return items_.at(p);
    }

    creature const* creature_at(bklib::ipoint2 const p) const {
        return creatures_.at(p);
    }

    creature* creature_at(bklib::ipoint2 const p) {
        return creatures_.at(p);
    }

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
private:
    chunk_t<terrain_entry>       terrain_entries_;
    chunk_t<terrain_render_data> terrain_render_data_;

    creature_map creatures_;
    item_map     items_;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
