#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>
#include "bklib/utility.hpp"
#include "bklib/exception.hpp"
#include "bklib/scope_guard.hpp"
#include <fstream>
#include <algorithm>

TEST_CASE("to_array", "[bklib][utility]") {
    constexpr auto const len = ptrdiff_t {5};

    auto const mismatch_distance = [](auto const& str, auto const& arr) {
        using std::begin;
        using std::end;

        auto const beg0 = begin(str);
        auto const beg1 = begin(arr);

        auto const p = std::mismatch(beg0, end(str), beg1, end(arr));

        return std::make_pair(
            std::distance(beg0, p.first)
          , std::distance(beg1, p.second));
    };

    SECTION("equal length case") {
        constexpr auto const str = bklib::make_string_view("test");
        static_assert(str.size() == len - 1, "");

        auto const arr = bklib::to_array<len>(str);
        REQUIRE(arr.size() == len);

        REQUIRE(mismatch_distance(str, arr) == std::make_pair(len - 1, len - 1));
        REQUIRE(arr[len - 1] == 0);
    }

    SECTION("truncate case") {
        constexpr auto const str = bklib::make_string_view("long test");
        static_assert(str.size() > len, "");

        auto const arr = bklib::to_array<len>(str);
        REQUIRE(arr.size() == len);

        REQUIRE(mismatch_distance(str, arr) == std::make_pair(len - 1, len - 1));
        REQUIRE(arr[len - 1] == 0);
    }

    SECTION("short case") {
        constexpr auto const str = bklib::make_string_view("abc");
        constexpr auto const short_len = static_cast<ptrdiff_t>(str.size());
        static_assert(str.size() < len, "");

        auto const arr = bklib::to_array<len>(str);
        REQUIRE(arr.size() == len);

        REQUIRE(mismatch_distance(str, arr) == std::make_pair(short_len, short_len));
        REQUIRE(arr[short_len] == 0);
    }

    SECTION("empty string case") {
        REQUIRE(bklib::to_array<len>("")[0] == 0);
    }

    SECTION("size 0") {
        REQUIRE(bklib::to_array<0>("test").size() == 0);
    }

    SECTION("size 1") {
        REQUIRE(bklib::to_array<1>("test")[0] == 0);
    }
}

TEST_CASE("read file", "[bklib][utility]") {
    REQUIRE_THROWS_AS(bklib::read_file_to_buffer("foo"), bklib::io_error const&);

    constexpr auto const file_name   = "test.txt";
    constexpr auto const test_string = bklib::make_string_view("test string");

    BK_NAMED_SCOPE_EXIT(guard) {
        std::remove(file_name);
    };

    std::ofstream {file_name, std::ios_base::out | std::ios_base::trunc} << test_string << '\0';
    auto const buffer = bklib::read_file_to_buffer(file_name);
    REQUIRE(!std::remove(file_name));
    REQUIRE(buffer.data() == test_string);

    guard.dismiss();
}

#endif // BK_NO_UNIT_TESTS
