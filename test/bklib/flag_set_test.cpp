#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "bklib/flag_set.hpp"
#include <cstdint>

TEST_CASE("flag_set") {
    enum class test_enum : uint32_t {
        flag_1, flag_2, flag_3, flag_4, flag_5,
    };

    using flag_set = bklib::flag_set<test_enum>;

    static_assert(sizeof(flag_set) == 4, "");
    static_assert(std::is_standard_layout<flag_set>::value, "");

    flag_set fs {};
    auto const all_flags = {
        test_enum::flag_1
      , test_enum::flag_2
      , test_enum::flag_3
      , test_enum::flag_4
      , test_enum::flag_5
    };

    SECTION("initial value") {
        REQUIRE(fs.none());
        REQUIRE(fs.none_of(all_flags));
        REQUIRE(!fs.any());
        REQUIRE(!fs.any_of(all_flags));

        REQUIRE(!fs.test(test_enum::flag_1));
        REQUIRE(!fs.test(test_enum::flag_2));
        REQUIRE(!fs.test(test_enum::flag_3));
        REQUIRE(!fs.test(test_enum::flag_4));
        REQUIRE(!fs.test(test_enum::flag_5));
    }

    SECTION("set and test") {
        fs.set(test_enum::flag_4);

        REQUIRE(fs.test(test_enum::flag_4));
        REQUIRE(fs.any());
        REQUIRE(fs.any_of(all_flags));
        REQUIRE(!fs.none());
        REQUIRE(!fs.none_of(all_flags));
    }

    SECTION("set, clear, test") {
        fs.set(test_enum::flag_4);
        fs.clear(test_enum::flag_4);

        REQUIRE(!fs.test(test_enum::flag_4));
        REQUIRE(!fs.any());
        REQUIRE(!fs.any_of(all_flags));
        REQUIRE(fs.none());
        REQUIRE(fs.none_of(all_flags));
    }
}

#endif //BK_NO_UNIT_TESTS
