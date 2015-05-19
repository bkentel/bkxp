#pragma once

#include "bklib/math.hpp"
#include <cstdint>

namespace bkrl {

enum class command_type : int32_t;

constexpr bklib::ivec3 vec_here   { 0,  0,  0};
constexpr bklib::ivec3 vec_north  { 0, -1,  0};
constexpr bklib::ivec3 vec_south  { 0,  1,  0};
constexpr bklib::ivec3 vec_east   { 1,  0,  0};
constexpr bklib::ivec3 vec_west   {-1,  0,  0};
constexpr bklib::ivec3 vec_n_east { 1, -1,  0};
constexpr bklib::ivec3 vec_n_west {-1, -1,  0};
constexpr bklib::ivec3 vec_s_east { 1,  1,  0};
constexpr bklib::ivec3 vec_s_west {-1,  1,  0};
constexpr bklib::ivec3 vec_up     { 0,  0, -1};
constexpr bklib::ivec3 vec_down   { 0,  0,  1};

//--------------------------------------------------------------------------------------------------
// @return a 3-vector corresponding to the direction @p cmd represents.
// @pre @p cmd must specify a command type that represents a valid direction.
//--------------------------------------------------------------------------------------------------
bklib::ivec3 direction_vector(command_type cmd);

} //namespace bkrl
