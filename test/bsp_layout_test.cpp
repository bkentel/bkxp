#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "bsp_layout.hpp"

TEST_CASE("calculate_splittable_ranges aspect", "[bsp_layout][bkrl]") {
    constexpr auto const a  = 16.0 / 9.0;
    constexpr auto const a1 = 1.0 / a;
    constexpr auto const lo = 5;
    constexpr auto const hi = 25;

    using st = bkrl::split_type;

    auto const calc = [=](int const w, int const h) {
        return bkrl::calculate_splittable_ranges(w, h, lo, hi, lo, hi, a);
    };

    auto const check_type = [](auto const& result, st const v, st const h) {
        return (result.v == v) && (result.h == h);
    };

    SECTION("too small") {
        for (auto x = 0; x < lo; ++x) {
            for (auto y = 0; y < lo; ++y) {
                REQUIRE(check_type(calc(x, y), st::none, st::none));
            }
        }
    }

    SECTION("too small to split well") {
        for (auto x = 0; x < lo; ++x) {
            for (auto y = 0; y < lo; ++y) {
                REQUIRE(check_type(calc(lo + x, lo + y), st::degenerate, st::degenerate));
            }
        }
    }

    SECTION("good") {
        for (auto x = lo * 2; x <= hi; ++x) {
            auto const y = x * a1;
            auto const w = x;
            auto const h = bklib::ceil_to<int>(y);

            REQUIRE(calc(w, h).v == st::can);
        }
    }

    SECTION("too big") {
        for (auto x = hi + 1; x < hi * 2; ++x) {
            for (auto y = hi + 1; y < hi * 2; ++y) {
                REQUIRE(check_type(calc(x, y), st::must, st::must));
            }
        }
    }

    //REQUIRE(check_type(calc(lo * 2, lo * 2), st::degenerate, st::degenerate));

    //bkrl::calculate_splittable_ranges(144, 256, lo, hi, lo, hi, a);
    //bkrl::calculate_splittable_ranges(211, 119, lo, hi, lo, hi, a);
    //bkrl::calculate_splittable_ranges(255, 255, lo, hi, lo, hi, a);
    //bkrl::calculate_splittable_ranges(46, 32, lo, hi, lo, hi, a);
    //bkrl::calculate_splittable_ranges(7, 11, lo, hi, lo, hi, a);
    //bkrl::calculate_splittable_ranges(8, 5, lo, hi, lo, hi, a);
    //bkrl::calculate_splittable_ranges(9, 7, lo, hi, lo, hi, a);
    //bkrl::calculate_splittable_ranges(12, 11, lo, hi, lo, hi, a);
    //bkrl::calculate_splittable_ranges(11, 11, lo, hi, lo, hi, a);
    //bkrl::calculate_splittable_ranges(24, 27, lo, hi, lo, hi, a);
    //bkrl::calculate_splittable_ranges(4, 6, lo, hi, lo, hi, a);
}

TEST_CASE("calculate_splittable_ranges no aspect", "[bsp_layout][bkrl]") {
    constexpr auto min_w = 2;
    constexpr auto max_w = 8;
    constexpr auto min_h = 3;
    constexpr auto max_h = 7;

    auto w = min_w * 2;
    auto h = min_h * 2;

    auto const calc = [&] {
        return bkrl::calculate_splittable_ranges(w, h, min_w, max_w, min_h, max_h);
    };

    using st = bkrl::split_type;

    SECTION("w too small") {
        w = min_w - 1;
        h = min_h * 2;

        auto const result = calc();

        REQUIRE(result.v == st::none);
        REQUIRE(result.v_range == bklib::make_range(0, 0));

        REQUIRE(result.h == st::can);
        REQUIRE(result.h_range == bklib::make_range(min_h, h - min_h));
    }

    SECTION("w a bit too small") {
        w = min_w * 2 - 1;
        h = min_h * 2;

        auto const result = calc();

        REQUIRE(result.v == st::degenerate);
        REQUIRE(result.v_range == bklib::make_range(min_w, w - min_w));

        REQUIRE(result.h == st::can);
        REQUIRE(result.h_range == bklib::make_range(min_h, h - min_h));
    }

    SECTION("w too large") {
        w = max_w + 1;
        h = min_h * 2;

        auto const result = calc();

        REQUIRE(result.v == st::must);
        REQUIRE(result.v_range == bklib::make_range(min_w, w - min_w));

        REQUIRE(result.h == st::can);
        REQUIRE(result.h_range == bklib::make_range(min_h, h - min_h));
    }

    SECTION("h too small") {
        w = min_w * 2;
        h = min_h - 1;

        auto const result = calc();

        REQUIRE(result.h == st::none);
        REQUIRE(result.h_range == bklib::make_range(0, 0));

        REQUIRE(result.v == st::can);
        REQUIRE(result.v_range == bklib::make_range(min_w, w - min_w));
    }

    SECTION("h too large") {
        w = min_w * 2;
        h = max_h + 1;

        auto const result = calc();

        REQUIRE(result.h == st::must);
        REQUIRE(result.h_range == bklib::make_range(min_h, h - min_h));

        REQUIRE(result.v == st::can);
        REQUIRE(result.v_range == bklib::make_range(min_w, w - min_w));
    }

    SECTION("h a bit too small") {
        w = min_w * 2;
        h = min_h * 2 - 1;

        auto const result = calc();

        REQUIRE(result.h == st::degenerate);
        REQUIRE(result.h_range == bklib::make_range(min_h, h - min_h));

        REQUIRE(result.v == st::can);
        REQUIRE(result.v_range == bklib::make_range(min_w, w - min_w));
    }

    SECTION("in range") {
        for (int x = min_w * 2; x < max_w; ++x) {
            for (int y = min_h * 2; y < max_h; ++y) {
                auto const result = calc();

                REQUIRE(result.h == st::can);
                REQUIRE(result.h_range == bklib::make_range(min_h, h - min_h));

                REQUIRE(result.v == st::can);
                REQUIRE(result.v_range == bklib::make_range(min_w, w - min_w));
            }
        }
    }
}

TEST_CASE("bsp_layout split", "[bkrl]") {
    using bklib::aspect_ratio;
    using bklib::irect;
    using bkrl::split_type;
    using bkrl::calculate_splittable_ranges;
    using bkrl::random_split;
    using bklib::range;
    using bklib::make_range;

    bkrl::random_t random;

    SECTION("aspect case 7") {
        constexpr auto const w      = 7;
        constexpr auto const h      = 11;
        constexpr auto const min_w  = 5;
        constexpr auto const min_h  = 5;
        constexpr auto const aspect = 16.0 / 9.0;

        auto const r = irect {0, 0, w, h};
        auto const result = random_split(random, r, min_w, 25, min_h, 25, aspect);
        REQUIRE(std::get<2>(result) == true);
    }

    SECTION("aspect case 6") {
        constexpr auto const w      = 144;
        constexpr auto const h      = 256;
        constexpr auto const min_w  = 5;
        constexpr auto const min_h  = 5;
        constexpr auto const aspect = 16.0 / 9.0;

        auto const r = irect {0, 0, w, h};
        auto const result = random_split(random,r, min_w, 25, min_h, 25, aspect);
        REQUIRE(std::get<2>(result) == true);
    }

    SECTION("aspect case 5") {
        constexpr auto const w      = 8;
        constexpr auto const h      = 5;
        constexpr auto const min_w  = 5;
        constexpr auto const min_h  = 5;
        constexpr auto const aspect = 16.0 / 9.0;

        auto const r = irect {0, 0, w, h};
        auto const result = random_split(random,r, min_w, 25, min_h, 25, aspect);
        REQUIRE(std::get<2>(result) == false);
    }

    SECTION("aspect case 4") {
        constexpr auto const w      = 9;
        constexpr auto const h      = 7;
        constexpr auto const min_w  = 5;
        constexpr auto const min_h  = 5;
        constexpr auto const aspect = 16.0 / 9.0;

        auto const r = irect {0, 0, w, h};
        auto const result = random_split(random,r, min_w, 25, min_h, 25, aspect);
        REQUIRE(std::get<2>(result) == false);
    }

    SECTION("aspect case 3") {
        constexpr auto const w      = 12;
        constexpr auto const h      = 11;
        constexpr auto const min_w  = 5;
        constexpr auto const min_h  = 5;
        constexpr auto const aspect = 16.0 / 9.0;

        auto const r = irect {0, 0, w, h};
        auto const result = random_split(random,r, min_w, 25, min_h, 25, aspect);
        REQUIRE(std::get<2>(result) == true);
    }

    SECTION("aspect case 2") {
        constexpr auto const w      = 11;
        constexpr auto const h      = 11;
        constexpr auto const min_w  = 5;
        constexpr auto const min_h  = 5;
        constexpr auto const aspect = 16.0 / 9.0;

        auto const r = irect {0, 0, w, h};
        auto const result = random_split(random,r, min_w, 25, min_h, 25, aspect);
        REQUIRE(std::get<2>(result) == true);
    }

    SECTION("aspect -- random") {
        constexpr int const min_w {4};
        constexpr int const max_w {10};
        constexpr int const min_h {4};
        constexpr int const max_h {10};

        auto const r = irect {0, 0, max_w, max_h};

        irect r0 {};
        irect r1 {};
        bool  ok {};

        for (auto i = 0u; i < 100; ++i) {
            constexpr auto const aspect_limit = 3.0;

            std::tie(r0, r1, ok) =
                random_split(random, r, min_w, max_w, min_h, max_h, aspect_limit);

            REQUIRE(ok);
            REQUIRE(r0 != r);
            REQUIRE(r1 != r);

            REQUIRE(aspect_limit >= bklib::make_aspect_ratio(r0).as<double>());
            REQUIRE(aspect_limit >= bklib::make_aspect_ratio(r1).as<double>());

            REQUIRE(r0.width() >= min_w);
            REQUIRE(r1.width() >= min_w);

            REQUIRE(r0.height() >= min_h);
            REQUIRE(r1.height() >= min_h);

            if (r0.height() == r.height()) {
                REQUIRE(r0.right  == r1.left);
                REQUIRE(r0.top    == r1.top);
                REQUIRE(r0.bottom == r1.bottom);
            } else {
                REQUIRE(r0.bottom == r1.top);
                REQUIRE(r0.left   == r1.left);
                REQUIRE(r0.right  == r1.right);
            }
        }
    }
}

TEST_CASE("bsp_layout gen", "[bkrl]") {
    bkrl::random_t random;
    bkrl::bsp_layout layout {bklib::irect {0, 0, 100, 80}};

    auto const p = layout.params();

    layout.generate(random, [&](bklib::irect const r) {
        auto const w = r.width();
        auto const h = r.height();

        REQUIRE(w >= p.min_w);
        REQUIRE(h >= p.min_h);
        REQUIRE(w <= p.max_edge_size);
        REQUIRE(h <= p.max_edge_size);
    });
}

#endif // BK_NO_UNIT_TESTS
