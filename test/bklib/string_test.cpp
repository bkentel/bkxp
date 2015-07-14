#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "bklib/string.hpp"
#include "bklib/algorithm.hpp"

//TEST_CASE("string_id", "[bklib][string]") {
//    using test_id_t = bklib::string_id<struct tag_test>;
//
//    SECTION("default constructed string hashes") {
//        test_id_t id;
//
//        REQUIRE(id.hash == 0);
//        auto const result = bklib::all_of(id.hash_string, [](auto const& e) { return e == 0; });
//        REQUIRE(result);
//    }
//
//    SECTION("maximum size") {
//        constexpr char const string[] = "0123456789A";
//        constexpr auto const hash = bklib::static_djb2_hash(string);
//
//        test_id_t const id {string};
//
//        REQUIRE(id.hash == hash);
//        REQUIRE(id.hash_string.back() == 0);
//
//        auto const result = std::equal(begin(id.hash_string), end(id.hash_string), std::begin(string));
//        REQUIRE(result);
//    }
//
//    SECTION("overlong string hashes") {
//        constexpr char const string[] = "this is too long";
//        constexpr auto const hash = bklib::static_djb2_hash(string);
//
//        test_id_t const id {string};
//
//        REQUIRE(id.hash == hash);
//        REQUIRE(id.hash_string.back() == 0);
//
//        auto const result = std::mismatch(
//            begin(id.hash_string), end(id.hash_string)
//          , std::begin(string), std::end(string));
//
//        auto const a = std::distance(begin(id.hash_string), result.first);
//        auto const b = std::distance(std::begin(string), result.second);
//
//        REQUIRE(a == b);
//        REQUIRE(a == sizeof(id.hash_string) - 1);
//    }
//}

#endif // BK_NO_UNIT_TESTS
