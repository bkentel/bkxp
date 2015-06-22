#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "view.hpp"

TEST_CASE("view <=> world transforms", "[math]") {
    int const window_w = 640;
    int const window_h = 480;
    int const tile_w   = 18;
    int const tile_h   = 18;

    bkrl::view v {window_w, window_h, tile_w, tile_h};

    SECTION("no zoom, center on 0,0") {
        v.center_on_world(0, 0);

        auto const o = v.origin();

        REQUIRE(x(o) == (window_w / 2 - tile_w / 2));
        REQUIRE(y(o) == (window_h / 2 - tile_h / 2));
    }

    SECTION("no zoom, center on 0,0") {
        auto const zoom = 2.0;

        v.zoom_to(zoom);
        v.center_on_world(0, 0);

        auto const o = v.origin();

        REQUIRE(x(o) == (window_w / 2 - tile_w * zoom / 2));
        REQUIRE(y(o) == (window_h / 2 - tile_h * zoom / 2));
    }

    SECTION("no zoom, no scroll") {
        for (int xi = 0; xi < tile_w; ++xi) {
            for (int yi = 0; yi < tile_h; ++yi) {
                auto const xp = 100 + xi;
                auto const yp = 100 + yi;

                auto const p = v.screen_to_world(xp, yp);
                REQUIRE(x(p) == (xp / tile_w));
                REQUIRE(y(p) == (yp / tile_h));
            }
        }
    }

    SECTION("zoom, no scroll") {
        auto const zoom = 2.0;
        v.zoom_to(zoom);

        for (int xi = 0; xi < tile_w; ++xi) {
            for (int yi = 0; yi < tile_h; ++yi) {
                auto const xp = 100 + xi;
                auto const yp = 100 + yi;

                auto const p = v.screen_to_world(xp, yp);
                REQUIRE(x(p) == static_cast<int>(std::trunc(xp / tile_w / zoom)));
                REQUIRE(y(p) == static_cast<int>(std::trunc(yp / tile_h / zoom)));
            }
        }
    }

    SECTION("window rect, no scroll, no zoom") {
        auto const r = v.screen_to_world(bklib::irect {0, 0, window_w, window_h});
        REQUIRE(r.left   == 0);
        REQUIRE(r.right  == bklib::round_to<int>(static_cast<double>(window_w) / tile_w));
        REQUIRE(r.top    == 0);
        REQUIRE(r.bottom == bklib::round_to<int>(static_cast<double>(window_h) / tile_h));
    }

    SECTION("window rect") {
        v.center_on_world(0, 0);
        auto const r = v.screen_to_world();

        REQUIRE(r.left   == bklib::floor_to<int>((-window_w / 2.0 + tile_w / 2.0) / tile_w));
        REQUIRE(r.right  == bklib::ceil_to<int>( ( window_w / 2.0 + tile_w / 2.0) / tile_w));
        REQUIRE(r.top    == bklib::floor_to<int>((-window_h / 2.0 + tile_h / 2.0) / tile_h));
        REQUIRE(r.bottom == bklib::ceil_to<int>( ( window_h / 2.0 + tile_h / 2.0) / tile_h));
    }

}

#endif // BK_NO_UNIT_TESTS
