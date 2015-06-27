#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "bklib/scope_guard.hpp"

TEST_CASE("scope_guard", "[scope_guard][bklib]") {
    bool flag = false;

    SECTION("trigger") {
        REQUIRE(!flag);
        {
            BK_SCOPE_EXIT { flag = true; };
        }
        REQUIRE(flag);
    }

    SECTION("dismiss") {
        REQUIRE(!flag);
        {
            BK_NAMED_SCOPE_EXIT(guard) { flag = true; };
            guard.dismiss();

        }
        REQUIRE(!flag);
    }
}

#endif // BK_NO_UNIT_TESTS
