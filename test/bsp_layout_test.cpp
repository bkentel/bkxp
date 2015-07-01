#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "bsp_layout.hpp"

TEST_CASE("bsp_layout split", "[bkrl]") {
    using bklib::aspect_ratio;
    using bklib::irect;
    using bkrl::split_type;
    using bkrl::calculate_splittable_ranges;
    using bkrl::random_split;
    using bklib::range;
    using bklib::make_range;

    bkrl::random_t random;

    auto const check_aspect = [](int const len, int const w, int const h, double const aspect) {
        auto const a = static_cast<double>(w) / static_cast<double>(len);
        auto const b = static_cast<double>(w) / static_cast<double>(h - len);
        return (a <= aspect && a >= 1.0)
            && (b <= aspect && b >= 1.0);
    };

    SECTION("no aspect") {
        constexpr auto const w     = 10;
        constexpr auto const h     = 12;
        constexpr auto const min_w = 1;
        constexpr auto const min_h = 2;

        auto const r = irect {0, 0, w, h};
        auto const result = calculate_splittable_ranges(r, min_w, min_h);

        REQUIRE(result.h == split_type::can);
        REQUIRE(result.h_range == make_range(min_h, h - min_h));

        REQUIRE(result.v == split_type::can);
        REQUIRE(result.v_range == make_range(min_w, w - min_w));
    }

    SECTION("aspect case 7") {
        constexpr auto const w      = 7;
        constexpr auto const h      = 11;
        constexpr auto const min_w  = 5;
        constexpr auto const min_h  = 5;
        constexpr auto const aspect = 16.0 / 9.0;

        auto const r = irect {0, 0, w, h};
        auto const result = random_split(random,r, min_w, 25, min_h, 25, aspect);
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
        REQUIRE(std::get<2>(result) == false);
    }

    SECTION("aspect case 1") {
        constexpr auto const w      = 24;
        constexpr auto const h      = 27;
        constexpr auto const min_w  = 5;
        constexpr auto const min_h  = 5;
        constexpr auto const aspect = 16.0 / 9.0;

        auto const r = irect {0, 0, w, h};
        auto const result = calculate_splittable_ranges(r, min_w, min_h, aspect);

        REQUIRE(result.v == split_type::degenerate);
        REQUIRE(result.h == split_type::can);

        REQUIRE(result.v_range == make_range(16, 16));
        REQUIRE(result.h_range == make_range(14, 14));
    }

    SECTION("aspect -- case too small") {
        constexpr auto const w      = 4;
        constexpr auto const h      = 6;
        constexpr auto const min_w  = 5;
        constexpr auto const min_h  = 5;
        constexpr auto const aspect = 16.0 / 9.0;

        auto const r = irect {0, 0, w, h};
        auto const result = calculate_splittable_ranges(r, min_w, min_h, aspect);

        REQUIRE(result.v == split_type::none);
        REQUIRE(result.h == split_type::degenerate);

        REQUIRE(result.v_range == make_range(0, 0));
        REQUIRE(result.h_range == make_range(min_h, min_h));
    }

    SECTION("aspect -- random") {
        constexpr auto const aspect = bklib::make_aspect_ratio(16, 10);
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
