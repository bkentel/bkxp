#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "bklib/spatial_map.hpp"
#include "bklib/math.hpp"
#include "bklib/utility.hpp"

TEST_CASE("simple spatial map test", "[spatial_map]") {
    constexpr int iterations = 10;

    using point_t = bklib::ipoint2;
    using id_t = bklib::tagged_value<struct tag_test_data, int>;

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
            return static_cast<int>(data.id) == i;
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
            REQUIRE(static_cast<int>(ptr->id) == i);

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
            REQUIRE(static_cast<int>(ptr->id) == i);

            auto const& p = ptr->pos;
            REQUIRE(x(p) == index_to_point_x(i));
            REQUIRE(y(p) == index_to_point_y(i));
        }
    }

    auto const checked_at = [&](bklib::ipoint2 const p) -> test_data const& {
        auto const result = map.at(p);
        REQUIRE(result);
        return *result;
    };

    SECTION("relocate invalid") {
        auto const& data = *map.at(index_to_point(0));
        REQUIRE(!map.relocate(point_t {-1, -1}, point_t {0, 0}, data));
    }

    SECTION("relocate valid to same location") {
        auto const& before = checked_at(index_to_point(0));

        auto const from = before.pos;
        auto const to   = from;

        REQUIRE(map.relocate(from, to, before));

        auto const& after = checked_at(index_to_point(0));

        REQUIRE(&before == &after);
    }

    SECTION("relocate valid to end") {
        auto const& last = checked_at(index_to_point(9));
        auto const& data = checked_at(index_to_point(0));

        auto const to   = last.pos + bklib::ivec2 {1, 1};
        auto const from = data.pos;

        REQUIRE(map.relocate(from, to, data));
        REQUIRE(checked_at(to).id == id_t {0});
    }

    SECTION("relocate valid to start") {
        auto const& first = checked_at(index_to_point(0));
        auto const& data  = checked_at(index_to_point(9));

        auto const to   = first.pos + bklib::ivec2 {-1, -1};
        auto const from = data.pos;

        REQUIRE(map.relocate(from, to, data));
        REQUIRE(checked_at(to).id == id_t {9});
    }

    SECTION("relocate valid to same internal index") {
        auto const& before = checked_at(index_to_point(5));

        auto const to   = before.pos + bklib::ivec2 {1, 1};
        auto const from = before.pos;

        REQUIRE(map.relocate(from, to, before));

        auto const& after = checked_at(to);
        REQUIRE(&before == &after);

        REQUIRE(after.id == id_t {5});
    }

    SECTION("remove") {
        auto const i = iterations / 2;
        auto const p = index_to_point(i);
        auto const result = map.remove(p);

        REQUIRE(static_cast<int>(result.id) == i);
        REQUIRE(result.pos == p);
        REQUIRE(!map.at(p));
    }
}

#endif // BK_NO_UNIT_TESTS
