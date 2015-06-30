#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "bklib/string.hpp"
#include <algorithm>
#include <iterator>

TEST_CASE("default constructed string hashes", "[bklib][string]") {
    bklib::string_id<struct tag_test> id;

    REQUIRE(id.hash == 0);
    for (auto const& e : id.hash_string) {
        REQUIRE(e == 0);
    }
}

TEST_CASE("string constructed string hashes", "[bklib][string]") {
    constexpr char string[] = "test";

    bklib::string_id<struct tag_test> id {string};

    REQUIRE(id.hash == bklib::static_djb2_hash(string));
    REQUIRE(std::equal(std::begin(string), std::end(string), id.hash_string.data()));

    SECTION("compare") {
        bklib::string_id<struct tag_test> id2 {std::string {"test"}};
        REQUIRE(id  == id2);
        REQUIRE(id2 == id);
    }
}


#endif // BK_NO_UNIT_TESTS
