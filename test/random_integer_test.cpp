#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>
#include "random.hpp"
#include "bklib/assert.hpp"
#include "bklib/string.hpp"

TEST_CASE("random_integer") {
    using namespace bkrl;

    using dtype = random_integer::distribution_type;

    auto const expect_result = [](random_integer const& i, auto const type, auto const a, auto const b, auto const c) {
        REQUIRE(i.type == type);

        switch (type) {
        case dtype::dice:
            REQUIRE(i.data.d.number   == a);
            REQUIRE(i.data.d.sides    == b);
            REQUIRE(i.data.d.modifier == c);
            break;
        case dtype::gaussian:
            REQUIRE(i.data.g.variance == b);
            REQUIRE(i.data.g.mean     == a);
            break;
        case dtype::uniform:
            REQUIRE(i.data.u.lo == a);
            REQUIRE(i.data.u.hi == b);
            break;
        default:
            break;
        }
    };

    SECTION("default") {
        SECTION("valid") {
            expect_result(make_random_integer( "0"), dtype::uniform,  0,  0, 0);
            expect_result(make_random_integer( "1"), dtype::uniform,  1,  1, 0);
            expect_result(make_random_integer("-1"), dtype::uniform, -1, -1, 0);
        }

        SECTION("invalid") {
            REQUIRE_THROWS_AS(make_random_integer( "2147483648"), std::overflow_error);
            REQUIRE_THROWS_AS(make_random_integer("-2147483649"), std::underflow_error);
            REQUIRE_THROWS_AS(make_random_integer( "9223372036854775808"), std::overflow_error);
            REQUIRE_THROWS_AS(make_random_integer("-9223372036854775809"), std::underflow_error);
        }
    }

    SECTION("range") {
        SECTION("valid") {
            expect_result(make_random_integer( "0~1"),    dtype::uniform,   0,  1,  0);
            expect_result(make_random_integer( "1~10"),   dtype::uniform,   1,  10, 0);
            expect_result(make_random_integer("-10~10"),  dtype::uniform, -10,  10, 0);
            expect_result(make_random_integer("-10~-10"), dtype::uniform, -10, -10, 0);
        }

        SECTION("invalid") {
            REQUIRE_THROWS_AS(make_random_integer("1~0"),  std::invalid_argument);
            REQUIRE_THROWS_AS(make_random_integer("~0"),   std::invalid_argument);
            REQUIRE_THROWS_AS(make_random_integer("0~"),   std::invalid_argument);
            REQUIRE_THROWS_AS(make_random_integer("0~1~"), std::invalid_argument);
        }
    }

    SECTION("dice") {
        SECTION("valid") {
            expect_result(make_random_integer("1d2"),    dtype::dice,   1,  2,   0);
            expect_result(make_random_integer("10d11"),  dtype::dice,  10, 11,   0);
            expect_result(make_random_integer("1d6+1"),  dtype::dice,   1,  6,   1);
            expect_result(make_random_integer("1d6+10"), dtype::dice,   1,  6,  10);
            expect_result(make_random_integer("1d6-2"),  dtype::dice,   1,  6,  -2);
            expect_result(make_random_integer("1d6-5"),  dtype::dice,   1,  6,  -5);
        }

        SECTION("invalid") {
            REQUIRE_THROWS_AS(make_random_integer("0d2"),    std::invalid_argument);
            REQUIRE_THROWS_AS(make_random_integer("2d0"),    std::invalid_argument);
            REQUIRE_THROWS_AS(make_random_integer("1d3+"),   std::invalid_argument);
            REQUIRE_THROWS_AS(make_random_integer("1d3-"),   std::invalid_argument);
            REQUIRE_THROWS_AS(make_random_integer("1d3-1 "), std::invalid_argument);
            REQUIRE_THROWS_AS(make_random_integer("d3"),     std::invalid_argument);
        }
    }

    SECTION("gaussian") {
        SECTION("valid") {
            expect_result(make_random_integer("1(2)"),   dtype::gaussian,   1,  2, 0);
            expect_result(make_random_integer("2(10)"),  dtype::gaussian,   2, 10, 0);
            expect_result(make_random_integer("-2(10)"), dtype::gaussian,  -2, 10, 0);
            expect_result(make_random_integer("10(11)"), dtype::gaussian,  10, 11, 0);
        }

        SECTION("invalid") {
            REQUIRE_THROWS_AS(make_random_integer("1("),    std::invalid_argument);
            REQUIRE_THROWS_AS(make_random_integer("1(2"),   std::invalid_argument);
            REQUIRE_THROWS_AS(make_random_integer("1(-2)"), std::invalid_argument);
            REQUIRE_THROWS_AS(make_random_integer("1(2) "), std::invalid_argument);
        }
    }
}

#endif // BK_NO_UNIT_TESTS

