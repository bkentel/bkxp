#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "bklib/spatial_map.hpp"
#include "bklib/math.hpp"
#include "bklib/utility.hpp"

TEST_CASE("simple spatial map test", "[spatial_map]") {
    constexpr int iterations = 10;

    using point_t = bklib::ipoint2;
    using id_t = bklib::tagged_value<int, struct tag_test_data>;

    struct test_data {
        id_t    id;
        point_t pos;
    };

    bklib::spatial_map_2d<test_data> map;

    auto const index_to_point_x = [](int const i) { return i * 10; };
    auto const index_to_point_y = [](int const i) { return i * 10 + 1; };

    auto const index_to_point = [&](int const i) {
        return point_t {index_to_point_x(i), index_to_point_y(i)};
    };

    auto const find_by_id = [](int const i) {
        return [i](test_data const& data) {
            return data.id.value == i;
        };
    };

    //
    // fill the map with some data
    //
    for (int i = 0; i < iterations; ++i) {
        auto const p = index_to_point(i);
        map.insert(p, test_data {id_t{i}, p});
    }

    SECTION("find each of the inserted values") {
        for (int i = 0; i < iterations; ++i) {
            auto const ptr = map.find(find_by_id(i));
            REQUIRE(!!ptr);
            REQUIRE(ptr->id.value == i);
            
            auto const& p = ptr->pos;
            REQUIRE(x(p) == index_to_point_x(i));
            REQUIRE(y(p) == index_to_point_y(i));
        }
    }

    SECTION("find invalid") {
        REQUIRE(map.find(find_by_id(iterations + 1)) == nullptr);
    }

    SECTION("find by position") {
        for (int i = 0; i < iterations; ++i) {
            auto const ptr = map.at(index_to_point(i));
            REQUIRE(!!ptr);
            REQUIRE(ptr->id.value == i);

            auto const& p = ptr->pos;
            REQUIRE(x(p) == index_to_point_x(i));
            REQUIRE(y(p) == index_to_point_y(i));
        }
    }
}

#endif // BK_NO_UNIT_TESTS
