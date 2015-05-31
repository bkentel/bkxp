#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "bklib/spatial_map.hpp"
#include "bklib/math.hpp"

TEST_CASE("simple spatial map test", "[spatial_map]") {
    struct test_data {
        int            id;
        bklib::ipoint2 pos;
    };

    struct compare_key {
        bool operator()(int const key, test_data const& data) const noexcept {
            return data.id == key;
        }
    };
    
    struct compare_pos {
        bool operator()(test_data const& data, int const xx, int const yy) const noexcept {
            return x(data.pos) == xx && y(data.pos) == yy;
        }
    };

    bklib::spatial_map<test_data, int, compare_key, compare_pos> map;

    for (int i = 0; i < 10; ++i) {
        map.insert(test_data {i, {i * 10, i * 10 + 1}});
    }

    SECTION("find invalid") {
        REQUIRE(map[11] == nullptr);
        REQUIRE(map.at(-1, -1) == nullptr);
    }

    SECTION("find by id") {
        for (int i = 0; i < 10; ++i) {
            auto const ptr = map[i];
            REQUIRE(ptr != nullptr);
            REQUIRE(ptr->id == i);
            REQUIRE(x(ptr->pos) == i * 10);
            REQUIRE(y(ptr->pos) == i * 10 + 1);
        }
    }

    SECTION("find by position") {
        for (int i = 0; i < 10; ++i) {
            int const xx = i * 10;
            int const yy = i * 10 + 1;

            auto const ptr = map.at(xx, yy);
            REQUIRE(ptr != nullptr);
            REQUIRE(ptr->id == i);
            REQUIRE(x(ptr->pos) == xx);
            REQUIRE(y(ptr->pos) == yy);
        }
    }
}

#endif // BK_NO_UNIT_TESTS
