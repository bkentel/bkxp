#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "terrain.hpp"

TEST_CASE("default constructed door", "[door][bkrl]") {
    bkrl::door d;

    REQUIRE(!d.is_open());
    REQUIRE(d.is_closed());
}

TEST_CASE("door from terrain", "[door][bkrl]") {
    bkrl::terrain_entry const e {1, bkrl::terrain_flags::none, bkrl::terrain_type::door, 0};

    bkrl::door d {e};
    
    SECTION("initial state") {
        REQUIRE(d.is_open());
        REQUIRE(!d.is_closed());
    }

    SECTION("close and open") {
        REQUIRE((d.close() && d.is_closed()));
        REQUIRE((d.open()  && d.is_open()));

        REQUIRE((d.set_open_close(bkrl::door::state::closed) && d.is_closed()));
        REQUIRE((d.set_open_close(bkrl::door::state::open)   && d.is_open()));
    }
}

#endif // BK_NO_UNIT_TESTS
