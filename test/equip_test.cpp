#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>
#include "equip.hpp"
#include "item.hpp"
#include "random.hpp"

TEST_CASE("equipment", "[bkrl][equip]") {
    bkrl::random_t random;

    using status_t = bkrl::equipment::result_t::status_t;
    bkrl::equipment eq;

    bkrl::item_factory ifac;
    bkrl::item_def idef0 {"item0"};

    SECTION("not equippable") {
        auto itm = ifac.create(random, idef0);
        auto const result = eq.can_equip(itm);
        REQUIRE(!result);
        REQUIRE(result.status == status_t::not_equippable);
    }

    idef0.tags.push_back(bkrl::make_tag("EQS_HEAD"));
    idef0.tags.push_back(bkrl::make_tag("CAT_ARMOR"));
    bkrl::process_tags(idef0);
    auto itm = ifac.create(random, idef0);

    SECTION("equippable") {
        auto const result = eq.can_equip(itm);
        REQUIRE(!!result);
        REQUIRE(result.status == status_t::ok);
    }

    SECTION("equip - empty") {
        auto result = eq.equip(itm);
        REQUIRE(!!result);
        REQUIRE(result.status == status_t::ok);
        REQUIRE(static_cast<bkrl::item_slots>(result).test(bkrl::equip_slot::head));
    }

    SECTION("equip - double") {
        REQUIRE(!!eq.equip(itm));
        auto result = eq.equip(itm);
        REQUIRE(!result);
        REQUIRE(result.status == status_t::already_equipped);
        REQUIRE(result.data == 0);
    }

    SECTION("unequip") {
        REQUIRE(!!eq.equip(itm));
        auto const result = eq.unequip(bkrl::equip_slot::head);
        REQUIRE(!!result);
        REQUIRE(result.status == status_t::ok);
        REQUIRE(static_cast<bkrl::item_slots>(result).test(bkrl::equip_slot::head));
    }

    SECTION("unequip - empty") {
        auto const result = eq.unequip(bkrl::equip_slot::torso);
        REQUIRE(!result);
        REQUIRE(result.status == status_t::slot_empty);
        REQUIRE(static_cast<bkrl::equip_slot>(result) == bkrl::equip_slot::torso);
    }
}

#endif // BK_NO_UNIT_TESTS

