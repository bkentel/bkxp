#pragma once

#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include <cstdint>

namespace bkrl {

enum class command_type : uint32_t;

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
