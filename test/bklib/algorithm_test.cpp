#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "bklib/algorithm.hpp"
#include <vector>

TEST_CASE("find_maybe", "[algorithm][bklib]") {
    std::vector<int> const v {10, 20, 30, 40, 50};

    auto const find_value = [](int const value) {
        return [value](int const n) { return n == value; };
    };

    SECTION("valid") {
        for (auto const& value : v) {
            auto const ptr = bklib::find_maybe(v, find_value(value));
            REQUIRE((ptr && ptr == &value));
        }
    }

    SECTION("invalid") {
        auto const ptr = bklib::find_maybe(v, find_value(0));
        REQUIRE(!ptr);
    }
}

#endif // BK_NO_UNIT_TESTS
