#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "text.hpp"

TEST_CASE("text rendering", "[text][graphics]") {
    bkrl::text_renderer trender;

    bkrl::text_layout layout {trender, "This is a test string.", 10, 10, 100, 100};
}

#endif // BK_NO_UNIT_TESTS
