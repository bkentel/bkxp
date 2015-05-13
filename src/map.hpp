#pragma once

#include "creature.hpp"

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
    T const& cell_at(int const x, int const y) const noexcept {
        return data[yi * size_block + xi];
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
    block_t<T> const& block_at(int const x, int const y) const noexcept {
        auto const yi = y / size_block;
        auto const xi = x / size_block;
        return data[yi * size_block + xi];
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
struct map_terrain_t {
    uint16_t base_type = 0; // floor
    uint16_t variation = 0; // dirty floor
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
struct map_object_t {
    uint16_t base_type = 0; // door
    uint16_t variation = 0; // rusty door
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
struct map_cell_t {
    map_terrain_t ter;
    map_object_t  obj;
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
struct map_light_t {
    float light = 0.0f;
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
class map {
public:
    void move_by(creature& critter, int dx, int dy);
    void draw(renderer& render);
private:
    chunk_t<map_cell_t>  base_;
    chunk_t<map_light_t> light_;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
