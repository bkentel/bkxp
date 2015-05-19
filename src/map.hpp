#pragma once

#include <bklib/math.hpp>

#include "terrain.hpp"

#include <array>
#include <cstdint>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

class renderer;

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
    block_t<T>& block_at(int const x, int const y) noexcept {
        auto const yi = y / size_block;
        auto const xi = x / size_block;
        return data[yi * size_block + xi];
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

    std::array<block_t<T>, size_block * size_block> data;
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
    void draw(renderer& render);

    void update_render_data(int x, int y);
    void update_render_data();

    bklib::irect bounds() const noexcept {
        return {0, 0, size_chunk, size_chunk};
    }

    terrain_entry& at(int x, int y) {
        return terrain_entries_.block_at(x, y).cell_at(x, y);
    }

    terrain_entry const& at(int x, int y) const {
        return terrain_entries_.block_at(x, y).cell_at(x, y);
    }

    void fill(bklib::irect r, terrain_type value);
    void fill(bklib::irect r, terrain_type value, terrain_type border);
private:
    chunk_t<terrain_entry>       terrain_entries_;
    chunk_t<terrain_render_data> terrain_render_data_;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
