#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "random.hpp"

#include <map>
#include <vector>
#include <numeric>
#include <array>

TEST_CASE("random cross platform consistent", "[bkrl][random]") {
    bkrl::random_t random;

    std::array<uint32_t, 256> const v {
        0xfffff50b, 0xfffff322, 0x00000978, 0x00000aca, 0x00000855, 0xfffff1eb, 0x00000ae0, 0xfffff9e9,
        0xfffffc9a, 0x0000040f, 0xfffffab5, 0x0000095f, 0x000007d1, 0xffffff2f, 0xfffffd56, 0x00000b24,
        0x0000075a, 0x00000477, 0x000005aa, 0x00000bb7, 0xfffff93a, 0xfffffd07, 0x00000712, 0x0000066e,
        0x00000e6d, 0xfffff6e1, 0x000004b1, 0x00000d48, 0x000005c3, 0xfffff226, 0x00000947, 0xfffff8ba,
        0xfffff799, 0x00000f8a, 0xfffffe90, 0xfffff4df, 0xfffff2e0, 0xfffffea9, 0xfffffec6, 0xfffff6e2,
        0xfffff8c8, 0xfffff74e, 0x00000028, 0xfffff72d, 0x00000c58, 0xfffffe33, 0x00000882, 0xfffff2ee,
        0x00000e5d, 0x00000293, 0x0000098e, 0x00000d4e, 0xfffff21a, 0x000007bf, 0xfffff377, 0xfffff267,
        0x000000dc, 0x000006d9, 0xfffffe3c, 0xfffff3c0, 0xfffff943, 0xfffffce0, 0xfffff166, 0x000006fb,
        0x00000c8e, 0xfffffc22, 0x000005fd, 0xfffff30d, 0xfffff258, 0xfffff77d, 0xfffff748, 0xfffff12e,
        0x00000b43, 0x000003f2, 0xfffff7ea, 0xffffff1e, 0x00000d2b, 0x00000c5a, 0xfffff709, 0x000003a7,
        0xfffff583, 0x0000086c, 0xfffffb8e, 0xfffff675, 0x00000902, 0xfffff83a, 0x0000094b, 0xfffffdb6,
        0x00000531, 0xfffff2ec, 0xfffff836, 0xfffffb8f, 0x0000085d, 0x00000251, 0xfffff1dc, 0x0000083b,
        0xfffffcd5, 0x000006fe, 0x000009d0, 0xfffff8b9, 0xfffff128, 0x00000761, 0x00000b85, 0x00000af0,
        0xfffffde8, 0x00000b79, 0xfffffe89, 0x00000c4a, 0x00000f92, 0x0000083c, 0xfffff504, 0xfffff758,
        0xfffff411, 0x00000b71, 0xfffffdae, 0x00000347, 0x0000041e, 0x00000d36, 0xfffff3d0, 0x00000611,
        0x00000f75, 0x00000f7b, 0x000009c4, 0x00000948, 0x00000b12, 0xfffffbec, 0x00000300, 0xfffffdc4,
        0xfffff477, 0xfffffa98, 0xfffffbf0, 0x00000c16, 0x00000e81, 0x0000093a, 0x00000a6f, 0x000005d8,
        0x00000158, 0xfffff312, 0x00000864, 0xfffff2ec, 0xfffff809, 0x00000a29, 0xfffffb1e, 0x00000f48,
        0x00000c07, 0xfffffe8b, 0x00000fe6, 0xfffff132, 0x00000bb3, 0xfffff036, 0xfffff80c, 0x000002ac,
        0xfffff9a2, 0x0000029d, 0xfffff209, 0xfffffa3f, 0xfffffffb, 0xfffff30b, 0x00000106, 0xfffffd12,
        0xfffffe38, 0xfffffc15, 0xfffff6a7, 0x00000b1c, 0x00000979, 0xfffff349, 0xfffff6c5, 0x00000e9c,
        0xfffff702, 0xfffff246, 0xfffff3b2, 0x000003a1, 0x00000123, 0xfffff7be, 0x000002c3, 0x00000ab1,
        0x00000546, 0xfffff5c4, 0xfffff261, 0xfffff2af, 0x0000062e, 0xfffff7fe, 0x00000f04, 0x00000d65,
        0xfffff723, 0x0000025d, 0xffffff13, 0x00000219, 0xfffffe0f, 0x000006d1, 0xfffff26b, 0xfffff282,
        0xfffff6f9, 0x00000a26, 0x000000e6, 0x000000c8, 0x00000d04, 0x0000017d, 0x00000fab, 0xfffff448,
        0x00000d59, 0xfffff6e0, 0x00000bf7, 0xfffff38e, 0x0000018e, 0xfffff78e, 0x000006dc, 0x000003b8,
        0xfffff15c, 0xfffff444, 0x00000603, 0xfffff70b, 0x000006c4, 0x000005e3, 0xfffff1a3, 0x0000086f,
        0xfffff5bb, 0x00000684, 0x00000d8d, 0x000008b2, 0xfffffb95, 0xfffffde4, 0xfffff465, 0xfffffba2,
        0x00000b41, 0x00000603, 0xfffffcda, 0x000009b0, 0x00000540, 0x00000e1d, 0xfffff2ca, 0xfffff751,
        0xfffff9d4, 0x00000987, 0xfffff496, 0xfffff6c6, 0x000000e9, 0x00000a29, 0xfffff27d, 0xfffff98e,
        0xfffff767, 0x00000bca, 0x00000ccf, 0x000004d9, 0xfffffb13, 0xfffff959, 0xfffff8af, 0xfffff68f,
        0x00000a27, 0xfffff23f, 0x00000f5f, 0x00000da3, 0x00000814, 0x00000be3, 0x00000739
    };

    for (auto n = 0; n < 255; ++n) {
        REQUIRE(static_cast<int32_t>(v[n]) == bkrl::random_range(random, -0xFFF, 0xFFF));
    }
}

TEST_CASE("random", "[bkrl][random]") {
    bkrl::random_t random;

    std::map<int, int> result;

    constexpr auto const range_min  = 0;
    constexpr auto const range_max  = 10;
    constexpr auto const range      = range_max - range_min + 1;
    constexpr auto const iterations = 10000;
    constexpr auto const expected   = iterations / range;

    SECTION("random range") {
        for (auto n = 0; n < iterations; ++n) {
            auto const i = bkrl::random_range(random, range_min, range_max);
            ++result[i];
        }

        REQUIRE(result.size() == range);

        for (auto const& count : result) {
            double const delta = count.second - expected;
            REQUIRE(0.05 > delta / expected);
        }
    }

    SECTION("random element") {
        std::vector<int> v(range);
        std::iota(begin(v), end(v), 0);

        for (auto n = 0; n < iterations; ++n) {
            auto const i = bkrl::random_element(random, v);
            ++result[i];
        }

        REQUIRE(result.size() == range);

        for (auto const& count : result) {
            double const delta = count.second - expected;
            REQUIRE(0.05 > delta / expected);
        }
    }

    SECTION("random dice") {
        for (auto n = 0; n < iterations; ++n) {
            auto const i = bkrl::roll_dice(random, 2, 6);
            ++result[i];
        }

        REQUIRE(result.size() == 11);

        auto const minmax = std::minmax_element(begin(result), end(result)
            , [](auto const& a, auto const& b) { return a.first < b.first; });

        REQUIRE(minmax.first->first  == 2);
        REQUIRE(minmax.second->first == 12);

        std::array<double, 11> p = {1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1};
        std::transform(begin(p), end(p), begin(p), [=](double n) { return iterations * n / 36.0; });

        for (auto i = range_min; i <= range_max; ++i) {
            double const delta = result[i + 2] - p[i];
            REQUIRE(0.05 > delta / p[i]);
        }
    }

}

#endif // BK_NO_UNIT_TESTS
