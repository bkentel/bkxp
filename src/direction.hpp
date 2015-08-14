#pragma once

#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include <cstdint>

namespace bkrl {

enum class command_type : uint32_t;

static constexpr int const x_off[] = {-1,  0,  1, -1,  1, -1,  0,  1, 0};
static constexpr int const y_off[] = {-1, -1, -1,  0,  0,  1,  1,  1, 0};

constexpr bklib::ivec2 index_to_offset(size_t const i) noexcept {
    return {x_off[i], y_off[i]};
}

constexpr size_t offset_to_index(int const xi, int const yi) noexcept {
    return (xi == x_off[0]) && (yi == y_off[0]) ? 0
         : (xi == x_off[1]) && (yi == y_off[1]) ? 1
         : (xi == x_off[2]) && (yi == y_off[2]) ? 2
         : (xi == x_off[3]) && (yi == y_off[3]) ? 3
         : (xi == x_off[4]) && (yi == y_off[4]) ? 4
         : (xi == x_off[5]) && (yi == y_off[5]) ? 5
         : (xi == x_off[6]) && (yi == y_off[6]) ? 6
         : (xi == x_off[7]) && (yi == y_off[7]) ? 7
         : (xi == x_off[8]) && (yi == y_off[8]) ? 8
         : 8;
}

constexpr size_t offset_to_index(bklib::ivec2 const v) noexcept {
    return offset_to_index(x(v), y(v));
}

static constexpr bklib::ivec3 const vec_here   { 0,  0,  0};
static constexpr bklib::ivec3 const vec_north  { 0, -1,  0};
static constexpr bklib::ivec3 const vec_south  { 0,  1,  0};
static constexpr bklib::ivec3 const vec_east   { 1,  0,  0};
static constexpr bklib::ivec3 const vec_west   {-1,  0,  0};
static constexpr bklib::ivec3 const vec_n_east { 1, -1,  0};
static constexpr bklib::ivec3 const vec_n_west {-1, -1,  0};
static constexpr bklib::ivec3 const vec_s_east { 1,  1,  0};
static constexpr bklib::ivec3 const vec_s_west {-1,  1,  0};
static constexpr bklib::ivec3 const vec_up     { 0,  0, -1};
static constexpr bklib::ivec3 const vec_down   { 0,  0,  1};

namespace detail {
inline constexpr bklib::ivec3 direction_vector(uint32_t const cmd) noexcept {
    return cmd == bklib::static_djb2_hash("dir_here")   ? vec_here
         : cmd == bklib::static_djb2_hash("dir_north")  ? vec_north
         : cmd == bklib::static_djb2_hash("dir_south")  ? vec_south
         : cmd == bklib::static_djb2_hash("dir_east")   ? vec_east
         : cmd == bklib::static_djb2_hash("dir_west")   ? vec_west
         : cmd == bklib::static_djb2_hash("dir_n_west") ? vec_n_west
         : cmd == bklib::static_djb2_hash("dir_n_east") ? vec_n_east
         : cmd == bklib::static_djb2_hash("dir_s_west") ? vec_s_west
         : cmd == bklib::static_djb2_hash("dir_s_east") ? vec_s_east
         : cmd == bklib::static_djb2_hash("dir_up")     ? vec_up
         : cmd == bklib::static_djb2_hash("dir_down")   ? vec_down
         : vec_here;
}
} //namespace detail

//--------------------------------------------------------------------------------------------------
// @return a 3-vector corresponding to the direction @p cmd represents.
// @pre @p cmd must specify a command type that represents a valid direction.
//--------------------------------------------------------------------------------------------------
inline constexpr bklib::ivec3 direction_vector(command_type const cmd) noexcept {
    return detail::direction_vector(static_cast<uint32_t>(cmd));
}

} //namespace bkrl
