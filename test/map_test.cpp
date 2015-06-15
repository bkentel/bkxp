#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "map.hpp"

TEST_CASE("map generate_creature", "[map][creature][bkrl]") {
    bkrl::random_state random;
    bkrl::creature_factory factory;
    bkrl::creature_def const cdef {"test"};
    bkrl::map map;

    SECTION("generate at same location") {
        bklib::ipoint2 const p {10, 10};

        REQUIRE(map.generate_creature(random, factory, cdef, p));
        REQUIRE(!map.generate_creature(random, factory, cdef, p));
    }

    SECTION("generate outside bounds") {
        auto const bounds = map.bounds();

        bklib::ipoint2 const p0 {bounds.left - 1, bounds.top - 1};
        bklib::ipoint2 const p1 {bounds.right, bounds.bottom};

        REQUIRE(!map.generate_creature(random, factory, cdef, p0));
        REQUIRE(!map.generate_creature(random, factory, cdef, p1));
    }
}

#endif // BK_NO_UNIT_TESTS
