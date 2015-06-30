#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>
#include "bklib/utility.hpp"
#include "bklib/exception.hpp"
#include <fstream>

TEST_CASE("read file", "[bklib][utility]") {
    REQUIRE_THROWS_AS(bklib::read_file_to_buffer("foo"), bklib::io_error const&);

    auto const test_string = bklib::utf8_string_view {"test string", 11};
    std::ofstream {"test.txt", std::ios_base::out | std::ios_base::trunc} << test_string << '\0';

    auto const buffer = bklib::read_file_to_buffer("test.txt");
    REQUIRE(buffer.data() == test_string);
}

#endif // BK_NO_UNIT_TESTS
