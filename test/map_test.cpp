#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "map.hpp"

TEST_CASE("map generate_creature", "[map][creature][bkrl]") {
    bkrl::random_state random;
    bkrl::map map;
    bkrl::creature_dictionary dic;
    bkrl::creature_factory factory {dic};
    
    bkrl::creature_def const cdef {"test"};
    
    REQUIRE(dic.insert(cdef));

    SECTION("generate at same location") {
        bklib::ipoint2 const p {10, 10};

        REQUIRE(!map.creature_at(p));
        REQUIRE(generate_creature(random, map, factory, cdef, p));
        
        auto const ptr = map.creature_at(p);
        REQUIRE(ptr);
        REQUIRE(!generate_creature(random, map, factory, cdef, p));      
        REQUIRE(ptr->def() == cdef.id);
    }
}

TEST_CASE("map generate_item", "[map][item][bkrl]") {
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
