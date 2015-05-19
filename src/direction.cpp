#include "direction.hpp"
#include "commands.hpp"
#include "bklib/assert.hpp"

bklib::ivec3 bkrl::direction_vector(command_type const cmd) {
    switch (cmd) {
    case command_type::dir_here:   return vec_here;
    case command_type::dir_north:  return vec_north;
    case command_type::dir_south:  return vec_south;
    case command_type::dir_east:   return vec_east;
    case command_type::dir_west:   return vec_west;
    case command_type::dir_n_west: return vec_n_west;
    case command_type::dir_n_east: return vec_n_east;
    case command_type::dir_s_west: return vec_s_west;
    case command_type::dir_s_east: return vec_s_east;
    case command_type::dir_up:     return vec_up;
    case command_type::dir_down:   return vec_down;
    default:
        BK_PRECONDITION(false);
        break;
    }

    return vec_here;
}
