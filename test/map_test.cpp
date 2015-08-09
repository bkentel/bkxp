#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "map.hpp"
#include "context.hpp"
#include "output.hpp"
#include "bklib/dictionary.hpp"

namespace {

void require_at(bkrl::map const& m, bklib::ipoint2 const p, bkrl::creature_def const& def) {
    auto const ptr = m.creature_at(p);
    REQUIRE(ptr);
    REQUIRE(ptr->def() == get_id(def));
}

void require_at(bkrl::map const& m, bklib::ipoint2 const p, bkrl::item_def const& def) {
    auto const ptr = m.items_at(p);
    REQUIRE(ptr);

    auto const it = std::find_if(std::begin(*ptr), std::end(*ptr), [&](auto const& i) {
        return i.def() == get_id(def);
    });

    REQUIRE(it != std::end(*ptr));
}

} //namespace

TEST_CASE("map terrain", "[map][terrain][bkrl]") {
    bkrl::map map;

    auto const r = bklib::irect {1, 1, bkrl::size_block + 1, bkrl::size_block + 1};
    map.fill(r, bkrl::terrain_type::floor);

    for (int y = 0; y < r.bottom + 1; ++y) {
        for (int x = 0; x < r.right + 1; ++x) {
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
    bkrl::random_state        random;
    bkrl::creature_dictionary dic;
    bkrl::definitions         defs {&dic, nullptr, nullptr};
    bkrl::creature_factory    cfactory;
    bkrl::item_factory        ifactory;
    bkrl::output              out;

    bkrl::creature_def const cdef {"test"};
    bklib::ipoint2 const p {10, 10};
    REQUIRE(dic.insert_or_discard(cdef).second);

    bkrl::context ctx {random, defs, out, ifactory, cfactory};

    bkrl::map map;

    SECTION("add outside room should fail") {
        auto const result = generate_creature(ctx, map, cdef, p);
        REQUIRE(!result.success);
    }

    SECTION("add, find and remove creature") {
        map.at(p).type = bkrl::terrain_type::floor;

        REQUIRE(!map.creature_at(p));
        REQUIRE(generate_creature(ctx, map, cdef, p));
        REQUIRE(map.creature_at(p));

        auto const ptr = map.find_creature([&](bkrl::creature const& c) {
            return c.position() == p;
        });

        REQUIRE(ptr);
        REQUIRE(ptr->position() == p);
        REQUIRE(ptr->def() == get_id(cdef));

        auto const c = map.remove_creature_at(p);
        REQUIRE(c.position() == p);
        REQUIRE(ptr->def() == get_id(cdef));
        REQUIRE(!map.creature_at(p));
    }

    SECTION("generate at same location") {
        map.at(p).type = bkrl::terrain_type::floor;

        REQUIRE(!map.creature_at(p));
        {
            // The first creature should be placed exactly as requested
            auto const result = generate_creature(ctx, map, cdef, p);
            REQUIRE(!!result);
            REQUIRE(result == p);
            require_at(map, p, cdef);
        }

        {
            bklib::irect const r {x(p) - 1, y(p) - 1, x(p) + 1, y(p) + 1};
            map.fill(r, bkrl::terrain_type::floor);

            // The second creature placed at the same location should succeed, but be
            // moved to an adjacent location.
            auto const result = generate_creature(ctx, map, cdef, p);
            REQUIRE(!!result);
            REQUIRE(result != p);
            require_at(map, result, cdef);

            auto const d = std::abs(std::sqrt(2) - bklib::distance(result.where, p));
            REQUIRE(d >= 0.0);
        }
    }
}

TEST_CASE("map items", "[map][item][bkrl]") {
    bkrl::random_state        random;
    bkrl::definitions         defs {nullptr, nullptr, nullptr};
    bkrl::creature_factory    cfactory;
    bkrl::item_factory        ifactory;
    bkrl::output              out;

    bkrl::item_def const idef0 {"test0"};
    bkrl::item_def const idef1 {"test1"};

    bkrl::context ctx {random, defs, out, ifactory, cfactory};

    bkrl::map map;

    SECTION("generate at same location") {
        bklib::ipoint2 const p {10, 10};

        map.at(p).type = bkrl::terrain_type::floor;

        REQUIRE(!map.items_at(p));
        REQUIRE(generate_item(ctx, map, idef0, p));
        REQUIRE(generate_item(ctx, map, idef1, p));

        auto const ptr = map.items_at(p);
        REQUIRE(ptr);

        REQUIRE(!ptr->empty());
        REQUIRE(std::distance(ptr->begin(), ptr->end()) == 2);

        auto it = std::begin(*ptr);
        REQUIRE(it->def() == get_id(idef1));
        REQUIRE((++it)->def() == get_id(idef0));
    }
}

#endif // BK_NO_UNIT_TESTS
