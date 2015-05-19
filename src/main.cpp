#include "bklib/assert.hpp"
#include "bklib/string.hpp"
#include "bklib/utility.hpp"
#include "bklib/math.hpp"
#include "bklib/exception.hpp"
#include "identifier.hpp"
#include "map.hpp"
#include "system.hpp"
#include "renderer.hpp"
#include "random.hpp"
#include "bsp_layout.hpp"
#include "commands.hpp"
#include "creature.hpp"

#include "game.hpp"

namespace bkrl {

////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////

using weight_value_t = bklib::tagged_value<int, struct tag_weight_value_t>;
using worth_value_t = bklib::tagged_value<int, struct tag_worth_value_t>;


} //namespace bkrl

namespace std {
//template <> struct hash<bkrl::texture_def> : bkrl::id_hash_base<bkrl::texture_def> {};
//template <> struct hash<bkrl::terrain_def> : bkrl::id_hash_base<bkrl::terrain_def> {};
}


int run_unit_tests();

void main() try {
    run_unit_tests();

    bkrl::game game;

    return;
} catch (bklib::exception_base const&) {
} catch (boost::exception const&) {
} catch (std::exception const&) {
} catch (...) {
}
