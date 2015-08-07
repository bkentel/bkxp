#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "text.hpp"

TEST_CASE("text rendering", "[text][graphics]") {
    bkrl::text_renderer trender;
    auto const line_spacing = trender.line_spacing();

    using size_type = bkrl::text_layout::size_type;

    constexpr size_type const left   = 5;
    constexpr size_type const top    = 10;
    constexpr size_type const width  = 120;
    constexpr size_type const height = 100;
    constexpr auto const text = bklib::make_string_view("This_is_a_test_string.");

    bkrl::text_layout const layout {trender, text, left, top, width, height};

    auto const extent = layout.extent();

    REQUIRE(extent.left == left);
    REQUIRE(extent.top  == top);

    constexpr auto const glyph_width = 18;

    auto const expected_w = width - (width  % glyph_width);
    REQUIRE(extent.width() == expected_w);

    auto const d = std::div(text.size() * glyph_width, expected_w);
    auto const expected_h = (d.quot + (d.rem ? 1 : 0)) * line_spacing;
    REQUIRE(extent.height() == expected_h);
}

#endif // BK_NO_UNIT_TESTS
