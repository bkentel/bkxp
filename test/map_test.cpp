#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "map.hpp"

TEST_CASE("map terrain", "[map][terrain][bkrl]") {
    bkrl::map map;

    auto const r = bklib::irect {1, 1, bkrl::size_block + 1, bkrl::size_block + 1};
    map.fill(r, bkrl::terrain_type::floor);

    auto const b = map.bounds();
    for (int y = 0; y < b.height(); ++y) {
        for (int x = 0; x < b.width(); ++x) {
            auto const  p = bklib::ipoint2 {x, y};
            auto const& t = map.at(p);
            if (intersects(r, p)) {
                REQUIRE(t.type == bkrl::terrain_type::floor);
            } else {
                REQUIRE(t.type == bkrl::terrain_type::empty);
            }
        }
    }
}

TEST_CASE("map creatures", "[map][creature][bkrl]") {
    bkrl::random_state random;
    bkrl::map map;
    bkrl::creature_dictionary dic;
    bkrl::creature_factory factory {dic};

    bkrl::creature_def const cdef {"test"};

    bklib::ipoint2 const p {10, 10};

    REQUIRE(dic.insert_or_discard(cdef).second);

    SECTION("add, find and remove creature") {
        REQUIRE(!map.creature_at(p));
        REQUIRE(generate_creature(random, map, factory, cdef, p));
        REQUIRE(map.creature_at(p));

        auto const ptr = map.find_creature([&](bkrl::creature const& c) {
            return c.position() == p;
        });

        REQUIRE(ptr);
        REQUIRE(ptr->position() == p);
        REQUIRE(ptr->def() == cdef.id);

        auto const c = map.remove_creature_at(p);
        REQUIRE(c.position() == p);
        REQUIRE(ptr->def() == cdef.id);
        REQUIRE(!map.creature_at(p));
    }

    SECTION("generate at same location") {
        REQUIRE(!map.creature_at(p));
        REQUIRE(generate_creature(random, map, factory, cdef, p));

        auto const ptr = map.creature_at(p);
        REQUIRE(ptr);
        REQUIRE(!generate_creature(random, map, factory, cdef, p));
        REQUIRE(ptr->def() == cdef.id);
    }
}

TEST_CASE("map items", "[map][item][bkrl]") {
    bkrl::random_state random;
    bkrl::item_factory factory;
    bkrl::map map;

    bkrl::item_def const idef0 {"test0"};
    bkrl::item_def const idef1 {"test1"};

    SECTION("generate at same location") {
        bklib::ipoint2 const p {10, 10};

        REQUIRE(!map.items_at(p));
        REQUIRE(generate_item(random, map, factory, idef0, p));
        REQUIRE(generate_item(random, map, factory, idef1, p));

        auto const ptr = map.items_at(p);
        REQUIRE(ptr);

        REQUIRE(!ptr->empty());
        REQUIRE(std::distance(ptr->begin(), ptr->end()) == 2);

        auto it = std::begin(*ptr);
        REQUIRE(it->def() == idef1.id);
        REQUIRE((++it)->def() == idef0.id);
    }
}

#endif // BK_NO_UNIT_TESTS
