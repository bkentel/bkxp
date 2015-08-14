#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "color.hpp"
#include "text.hpp"
#include "bklib/dictionary.hpp"

TEST_CASE("text rendering metrics", "[text][graphics][bkrl]") {
    auto text_render = bkrl::make_text_renderer();
    bkrl::text_renderer& trender = *text_render;

    using size_type = bkrl::text_renderer::size_type;
    size_type const w = 18;
    size_type const h = 18;

    REQUIRE(trender.bbox() == (bkrl::text_renderer::point_t {w, h}));
    REQUIRE(trender.line_spacing() == h);
}

TEST_CASE("text layout metrics", "[text][graphics][bkrl]") {
    auto text_render = bkrl::make_text_renderer();
    bkrl::text_renderer& trender = *text_render;
    bkrl::text_layout layout {trender, "", 10, 20, 100, 200};

    auto const extent = layout.extent();
    auto const bounds = layout.bounds();

    REQUIRE(extent.left == 10);
    REQUIRE(extent.left == bounds.left);

    REQUIRE(extent.top == 20);
    REQUIRE(extent.top == bounds.top);

    REQUIRE(bounds.width()  == 100);
    REQUIRE(bounds.height() == 200);

    REQUIRE(extent.width()  == 0);
    REQUIRE(extent.height() == trender.line_spacing());

    layout.set_position(5, 10);
    REQUIRE(layout.bounds().left == 5);
    REQUIRE(layout.bounds().top  == 10);

    layout.clip_to(50, 60);
    REQUIRE(layout.bounds().width()  == 50);
    REQUIRE(layout.bounds().height() == 60);
}

TEST_CASE("text layout default", "[text][graphics][bkrl]") {
    using size_type = bkrl::text_layout::size_type;

    auto text_render = bkrl::make_text_renderer();
    bkrl::text_renderer& trender = *text_render;
    bkrl::text_layout layout {trender, ""};

    auto const extent = layout.extent();
    auto const bounds = layout.bounds();

    REQUIRE(extent.left == 0);
    REQUIRE(extent.left == bounds.left);

    REQUIRE(extent.top == 0);
    REQUIRE(extent.top == bounds.top);

    REQUIRE(bounds.right  == std::numeric_limits<size_type>::max());
    REQUIRE(bounds.bottom == std::numeric_limits<size_type>::max());

    REQUIRE(extent.width()  == 0);
    REQUIRE(extent.height() == trender.line_spacing());
}

TEST_CASE("text rendering colors", "[text][graphics][bkrl]") {
    bkrl::color_dictionary cdic;
    bkrl::color_def col0 {"color0"};
    col0.short_name = "0";
    col0.color = bkrl::make_color(10, 20, 30, 40);

    auto text_render = bkrl::make_text_renderer();
    bkrl::text_renderer& trender = *text_render;

    auto const fallback_color =  bkrl::make_color(255, 255, 255);

    SECTION("no colors") {
        REQUIRE(trender.get_color("0") == fallback_color);
    }

    SECTION("definition present") {
        cdic.insert_or_discard(col0);
        trender.set_colors(&cdic);

        SECTION("missing") {
            REQUIRE(trender.get_color("1") == fallback_color);
        }

        SECTION("present")  {
            REQUIRE(trender.get_color("0") == col0.color);
        }

        SECTION("clear color") {
            trender.set_colors();
            REQUIRE(trender.get_color("0") == fallback_color);
        }
    }
}

TEST_CASE("text rendering - simple", "[text][graphics][bkrl]") {
    auto text_render = bkrl::make_text_renderer();
    bkrl::text_renderer& trender = *text_render;
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

    auto const glyph_width = x(trender.bbox());

    auto const expected_w = width - (width  % glyph_width);
    REQUIRE(extent.width() == expected_w);

    auto const d = std::div(static_cast<int>(text.size()) * glyph_width, expected_w);
    auto const expected_h = (d.quot + (d.rem ? 1 : 0)) * line_spacing;
    REQUIRE(extent.height() == expected_h);
}

TEST_CASE("text rendering", "[text][graphics][bkrl]") {
    auto text_render = bkrl::make_text_renderer();
    bkrl::text_renderer& trender = *text_render;
    auto const bbox = trender.bbox();

    using size_type = bkrl::text_renderer::size_type;

    SECTION("escaped text") {
        bkrl::text_layout layout {trender, R"(\<not a tag\>)"};
        REQUIRE((layout.extent().width() / x(bbox)) == 11);
        REQUIRE(layout.glyphs_at_line(0) == 11);
    }

    SECTION("colored text") {
        bkrl::text_layout layout {trender, "<color=y>text</color>"};
        REQUIRE((layout.extent().width() / x(bbox)) == 4);
        REQUIRE(layout.glyphs_at_line(0) == 4);
    }

    SECTION("text with embedded newline") {
        auto const line_w = static_cast<size_type>(x(bbox) * 5);
        bkrl::text_layout layout {trender, "line0\nline1\nline2"};

        auto const lines = layout.extent().height() / trender.line_spacing();
        REQUIRE(lines == 3);
        REQUIRE(layout.glyphs_at_line(0) == 5);
        REQUIRE(layout.glyphs_at_line(1) == 5);
        REQUIRE(layout.glyphs_at_line(2) == 5);
    }

    SECTION("wrapped text before space") {
        auto const line_w = static_cast<size_type>(x(bbox) * 5);
        bkrl::text_layout layout {trender, "line0 line1 line2", 0, 0, line_w};

        auto const lines = layout.extent().height() / trender.line_spacing();
        REQUIRE(lines == 3);
        REQUIRE(layout.glyphs_at_line(0) == 5);
        REQUIRE(layout.glyphs_at_line(1) == 5);
        REQUIRE(layout.glyphs_at_line(2) == 5);
    }

    SECTION("wrapped text on space") {
        auto const line_w = static_cast<size_type>(x(bbox) * 6);
        bkrl::text_layout layout {trender, "line0 line1 line2", 0, 0, line_w};

        auto const lines = layout.extent().height() / trender.line_spacing();
        REQUIRE(lines == 3);
        REQUIRE(layout.glyphs_at_line(0) == 6);
        REQUIRE(layout.glyphs_at_line(1) == 6);
        REQUIRE(layout.glyphs_at_line(2) == 5);
    }

    SECTION("wrapped text after space") {
        auto const line_w = static_cast<size_type>(x(bbox) * 7);
        bkrl::text_layout layout {trender, "line0 line1 line2", 0, 0, line_w};

        auto const lines = layout.extent().height() / trender.line_spacing();
        REQUIRE(lines == 3);
        REQUIRE(layout.glyphs_at_line(0) == 6);
        REQUIRE(layout.glyphs_at_line(1) == 6);
        REQUIRE(layout.glyphs_at_line(2) == 5);
    }

    SECTION("wrapped text after space, mid-tag") {
        auto const line_w = static_cast<size_type>(x(bbox) * 7);
        bkrl::text_layout layout {trender, "line0 <color=r>line1</color> line2", 0, 0, line_w};

        auto const lines = layout.extent().height() / trender.line_spacing();
        REQUIRE(lines == 3);
    }

    SECTION("height-restricted text") {
        auto const line_w = static_cast<size_type>(x(bbox) * 5);
        bkrl::text_layout layout {trender, "line0\nline1 line2", 0, 0, line_w, trender.line_spacing()};

        auto const lines = layout.extent().height() / trender.line_spacing();
        REQUIRE(lines == 1);
    }

    SECTION("complex wrapping (bug related case)") {
        constexpr auto const str = bklib::make_string_view(
            "You already have the <color=gy>torn rags</color> equipped. Your <color=r>torso</color> is occupied by a <color=gy>torn rags</color>."
        );

        bkrl::text_layout layout {trender, str, 0, 0, 640, 180};

        auto const lines = layout.extent().height() / trender.line_spacing();
        REQUIRE(lines == 3);
        REQUIRE(layout.glyphs_at_line(0) == 31);
        REQUIRE(layout.glyphs_at_line(1) == 35);
        REQUIRE(layout.glyphs_at_line(2) == 12);
    }
}

#endif // BK_NO_UNIT_TESTS
