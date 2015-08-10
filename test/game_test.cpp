#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "game.hpp"
#include "context.hpp"
#include "creature.hpp"
#include "map.hpp"
#include "inventory.hpp"
#include "commands.hpp"
#include "item.hpp"
#include "random.hpp"
#include "output.hpp"
#include "text.hpp"

#include "bklib/dictionary.hpp"

TEST_CASE("get and drop items", "[bkrl][game]") {
    bkrl::map                 m;
    bkrl::random_state        random_state;
    bkrl::random_t&           random = random_state[bkrl::random_stream::substantive];
    bkrl::item_dictionary     idefs;
    bkrl::item_factory        ifac;
    bkrl::creature_dictionary cdefs;
    bkrl::creature_factory    cfac;
    bkrl::text_renderer       trender;
    bkrl::command_translator  commands;

    bkrl::definitions const defs {nullptr, nullptr, nullptr};

    bkrl::item_def const idef0 = [] {
        bkrl::item_def def {"item 0"};
        def.flags.set(bkrl::item_flag::is_equippable);
        def.slots.set(bkrl::equip_slot::torso);
        return def;
    }();

    bkrl::item_def const idef1 {"item 1"};

    bkrl::creature_def const cdef0 {"creature 0"};
    bkrl::creature_def const cdef1 {"creature 1"};

    idefs.insert_or_discard(idef0);
    idefs.insert_or_discard(idef1);

    cdefs.insert_or_discard(cdef0);
    cdefs.insert_or_discard(cdef1);

    bkrl::output out;

    bkrl::context ctx {
        random_state, defs, out, ifac, cfac
    };

    bkrl::inventory imenu {trender};

    auto const p =  bklib::ipoint2 {0, 0};
    auto creature = cfac.create(random, cdef0, p);

    // helper for checking results
    auto const expect_result = [](auto const result, auto const expected) {
        using T = decltype(expected);

        if (result == T::ok) {
            REQUIRE(!!result);
        } else {
            REQUIRE(!result);
        }

        REQUIRE(result == expected);
    };

    // helper to select the first (and only) item
    auto const select_first_item = [&] {
        imenu.command(bkrl::command {bkrl::command_type::dir_south, 0, 0});
        imenu.command(bkrl::command {bkrl::command_type::confirm, 0, 0});
    };

    auto const equip_first_item = [&] {
        imenu.command(bkrl::command {bkrl::command_type::dir_south, 0, 0});
        imenu.command(bkrl::make_command(bkrl::command_raw_t {
            bkrl::key_mod_state {bkrl::key_mod::ctrl}, 'e'
        }));
    };

    SECTION("get item") {
        SECTION("nothing present") {
            expect_result(bkrl::get_item(ctx, creature, m, p, imenu, commands)
              , bkrl::get_item_result::no_items);
        }

        SECTION("distant item") {
            auto const where = p + bklib::ivec2 {2, 2};
            expect_result(bkrl::get_item(ctx, creature, m, where, imenu, commands)
              , bkrl::get_item_result::out_of_range);
        }

        SECTION("good location") {
            // need a valid tile for the item
            m.at(p).type = bkrl::terrain_type::floor;
            bkrl::generate_item(ctx, m, idef0, p);

            expect_result(bkrl::get_item(ctx, creature, m, p, imenu, commands)
              , bkrl::get_item_result::ok);

            // should not yet have any items
            REQUIRE(creature.item_list().empty());
            select_first_item();

            auto const& items = creature.item_list();
            REQUIRE(!items.empty());
            REQUIRE(items.begin()->def() == idef0);

            REQUIRE_FALSE(m.items_at(p));
        }
    }

    SECTION("drop item") {
        SECTION("no items") {
            expect_result(bkrl::drop_item(ctx, creature, m, p, imenu, commands)
              , bkrl::drop_item_result::no_items);
        }

        SECTION("with one item") {
            creature.get_item(ifac.create(random, idef0));

            SECTION("distant location") {
                auto const where = p + bklib::ivec2 {2, 2};
                expect_result(bkrl::drop_item(ctx, creature, m, where, imenu, commands)
                  , bkrl::drop_item_result::out_of_range);

                REQUIRE(!creature.item_list().empty());
            }

            SECTION("bad location") {
                expect_result(bkrl::drop_item(ctx, creature, m, p, imenu, commands)
                  , bkrl::drop_item_result::ok);

                select_first_item();

                REQUIRE(!creature.item_list().empty());
            }

            SECTION("good location") {
                m.at(p).type = bkrl::terrain_type::floor;

                expect_result(bkrl::drop_item(ctx, creature, m, p, imenu, commands)
                  , bkrl::drop_item_result::ok);

                select_first_item();

                REQUIRE(creature.item_list().empty());

                auto const pile = m.items_at(p);
                REQUIRE(pile);
                REQUIRE(!pile->empty());
                REQUIRE(pile->begin()->def() == idef0);
            }
        }
    }

    SECTION("show inventory") {
        SECTION("no items") {
            expect_result(bkrl::show_inventory(ctx, creature, imenu, commands)
              , bkrl::show_inventory_result::no_items);
        }

        SECTION("with one item") {
            creature.get_item(ifac.create(random, idef0));

            expect_result(bkrl::show_inventory(ctx, creature, imenu, commands)
              , bkrl::show_inventory_result::ok);

            // equip via the inventory
            REQUIRE(!creature.item_list().begin()->flags().test(bkrl::item_flag::is_equipped));
            REQUIRE(!creature.equip_list().is_equipped(bkrl::equip_slot::torso));

            equip_first_item();

            REQUIRE(creature.item_list().begin()->flags().test(bkrl::item_flag::is_equipped));
            REQUIRE(creature.equip_list().is_equipped(bkrl::equip_slot::torso));
        }
    }

    SECTION("equip item") {
        SECTION("not equippable") {
            creature.get_item(ifac.create(random, idef1));
            expect_result(bkrl::equip_item(ctx, creature, creature.item_list().advance(0))
              , bkrl::equip_result_t::not_equippable);
        }

        SECTION("already equipped") {
            creature.get_item(ifac.create(random, idef0));

            expect_result(bkrl::equip_item(ctx, creature, creature.item_list().advance(0))
              , bkrl::equip_result_t::ok);

            expect_result(bkrl::equip_item(ctx, creature, creature.item_list().advance(0))
              , bkrl::equip_result_t::already_equipped);
        }

        SECTION("occupied") {
            creature.get_item(ifac.create(random, idef0));
            creature.get_item(ifac.create(random, idef0));

            expect_result(bkrl::equip_item(ctx, creature, creature.item_list().advance(0))
              , bkrl::equip_result_t::ok);

            expect_result(bkrl::equip_item(ctx, creature, creature.item_list().advance(1))
              , bkrl::equip_result_t::slot_occupied);
        }
    }
}

#endif // BK_NO_UNIT_TESTS

