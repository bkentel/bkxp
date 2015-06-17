#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "bklib/math.hpp"

TEST_CASE("transform_float", "[bklib][math]") {
    using namespace bklib;

    REQUIRE(1 == transform_float<transform_float_none>(1));
    REQUIRE(1 == transform_float<transform_float_floor>(1));
    REQUIRE(1 == transform_float<transform_float_ceil>(1));
    REQUIRE(1 == transform_float<transform_float_round>(1));

    REQUIRE(1.1 == transform_float<transform_float_none>(1.1));
    REQUIRE(1.0 == transform_float<transform_float_floor>(1.1));
    REQUIRE(2.0 == transform_float<transform_float_ceil>(1.1));
    REQUIRE(1.0 == transform_float<transform_float_round>(1.1));
}

TEST_CASE("intersections", "[bklib][math]") {
    using bklib::irect;

    constexpr int const left   = 0;
    constexpr int const top    = 10;
    constexpr int const right  = 110;
    constexpr int const bottom = 100;

    irect const a {left, top, right, bottom};

    SECTION("a == b") {
        irect const b = a;
        auto const result = intersection(a, b);
        REQUIRE(result == a);
    }

    SECTION("a contains b") {
        irect const b {left + 10, top + 10, right - 10, bottom - 10};
        auto const result = intersection(a, b);
        REQUIRE(result == b);
    }

    SECTION("disjoint a and b (bottom right)") {
        irect const b {bottom, right, right + 10, right + 10};
        auto const result = intersection(a, b);
        REQUIRE(!result);
    }

    //TODO more tests
}

#endif //BK_NO_UNIT_TESTS
