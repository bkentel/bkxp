#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "bsp_layout.hpp"

TEST_CASE("bsp_layout split", "[bkrl]") {
    using bklib::aspect_ratio;
    using bklib::irect;
    using bkrl::split_type;

    SECTION("no aspect") {
        constexpr auto const w     = 10;
        constexpr auto const h     = 12;
        constexpr auto const min_w = 1;
        constexpr auto const min_h = 2;

        auto const result = bkrl::calculate_splittable_ranges(irect {0, 0, w, h}, min_w, min_h);

        REQUIRE(result.h == split_type::can);
        REQUIRE(result.h_range == bklib::make_range(min_h, h - min_h));

        REQUIRE(result.v == split_type::can);
        REQUIRE(result.v_range == bklib::make_range(min_w, w - min_w));
    }

    SECTION("aspect") {
        constexpr auto const w      = 10;
        constexpr auto const h      = 16;
        constexpr auto const min_w  = 4;
        constexpr auto const min_h  = 4;
        constexpr auto const aspect = 1.6;

        auto const result = bkrl::calculate_splittable_ranges(irect {0, 0, w, h}, min_w, min_h, aspect);

        auto const check_aspect = [=](int const len) {
            auto const a = static_cast<double>(w) / static_cast<double>(len);
            auto const b = static_cast<double>(w) / static_cast<double>(h - len);
            return (a <= aspect && a >= 1.0)
                && (b <= aspect && b >= 1.0);
        };

        REQUIRE(result.h == split_type::can);

        for (auto i = result.h_range.lo; i <= result.h_range.hi; ++i) {
            REQUIRE(check_aspect(i));
        }

        REQUIRE(!check_aspect(result.h_range.lo  - 1));
        REQUIRE(!check_aspect(result.h_range.hi + 1));

        REQUIRE(result.v == split_type::none);
        REQUIRE(result.v_range == bklib::make_range(0, 0));
    }

    constexpr auto const aspect = bklib::make_aspect_ratio(16, 10);
    constexpr int const min_w {4};
    constexpr int const max_w {10};
    constexpr int const min_h {4};
    constexpr int const max_h {10};

    bkrl::random_t random;

    auto const r = irect {0, 0, max_w, max_h};

    irect r0 {};
    irect r1 {};
    bool  ok {};

    for (auto i = 0u; i < 1000; ++i) {
        constexpr auto const aspect_limit = 3.0;

        std::tie(r0, r1, ok) =
            bkrl::random_split(random, r, min_w, max_w, min_h, max_h, aspect_limit);

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

#endif // BK_NO_UNIT_TESTS
