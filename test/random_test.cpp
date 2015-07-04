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

    constexpr auto const s =
        R"(/*koh&o>F[@kfMHpe]`r<Gdbz5]w`'k:7}K/)LL5:6P6tJh)zWlw'f+(RdJ,<F%d)"
        R"(tDa)'66$q[8Mvt5Z1hB3j9kI_)9BhV&hFdl:$eqpIqKt}h/6,qIY\v,a}}lkpDXI)"
        R"(.@DszknaS*h)8mA|sK~$r!8W=W&?O)SGJD4pk*4z5'+ZS7Xo_1'(b8|w5VMVJd(()"
        R"(5mRRvT~-w5s+T7dZ$-a5ca%h1cwiCI-CqaFl_y)6=k.4Rm(=6ru^A<:4m'}xgre)";

    for (int i = 0; i < 255; ++i) {
        auto const c = static_cast<char>(bkrl::random_range(random, 33, 126));
        REQUIRE(s[i] == c);
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

    REQUIRE(range_min >= 0);
    REQUIRE(range_max >= 0);
    REQUIRE(range_max >= range_min);

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

    SECTION("random x_in_y_chance") {
        double i = 0;
        for (auto n = 0; n < iterations; ++n) {
            i += bkrl::x_in_y_chance(random, 3, 10) ? 1 : 0;
        }

        double const ratio = 3.0 / 10;
        double const delta = (i / iterations) - ratio;
        REQUIRE(0.05 > delta);
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
            auto const j = static_cast<size_t>(i);
            double const delta = result[j + 2] - p[j];
            REQUIRE(0.05 > delta / p[j]);
        }
    }

}

#endif // BK_NO_UNIT_TESTS
