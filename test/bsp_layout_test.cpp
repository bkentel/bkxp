#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "bsp_layout.hpp"

TEST_CASE("bsp_layout split", "[bkrl]") {
    using bklib::aspect_ratio;
    using bklib::irect;
    using bkrl::classify_split;
    using bkrl::split_type;

    constexpr auto const aspect = bklib::make_aspect_ratio(16, 10);
    constexpr int const min_w {4};
    constexpr int const max_w {10};
    constexpr int const min_h {4};
    constexpr int const max_h {10};

    auto const classify = [&](irect const r) {
        return classify_split(r, min_w, max_w, min_h, max_h, aspect);
    };

    auto const expect_type = [](auto const type, auto const v, auto const h) {
        REQUIRE(type.vertical   == v);
        REQUIRE(type.horizontal == h);
    };

    using st = bkrl::split_type;

    expect_type(classify(irect {0, 0, max_w,     max_h    }), st::can,  st::can);
    expect_type(classify(irect {0, 0, max_w + 1, max_h    }), st::must, st::can);
    expect_type(classify(irect {0, 0, max_w,     max_h + 1}), st::can,  st::must);
    expect_type(classify(irect {0, 0, max_w + 1, max_h + 1}), st::must, st::must);

    expect_type(classify(irect {0, 0, min_w,     min_h    }), st::none, st::none);
    expect_type(classify(irect {0, 0, min_w + 1, min_h    }), st::can,  st::none);
    expect_type(classify(irect {0, 0, min_w,     min_h + 1}), st::none, st::can);
    expect_type(classify(irect {0, 0, min_w + 1, min_h + 1}), st::can,  st::can);

    constexpr auto const a = aspect.num / 2;
    constexpr auto const b = aspect.den / 2;
    expect_type(classify(irect {0, 0, a,     b    }), st::can,  st::can);
    expect_type(classify(irect {0, 0, a + 1, b    }), st::must, st::can);
    expect_type(classify(irect {0, 0, b,     a + 1}), st::can,  st::must);
}

#endif // BK_NO_UNIT_TESTS
