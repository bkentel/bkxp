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

    auto commands_ptr = bkrl::make_command_translator();
    auto& commands = *commands_ptr;

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

        REQUIRE((result == expected));
    };

    auto const expect_command_result = [&](bool& result, auto const expected_cmd, auto const expected_result) {
        commands.set_command_result_handler([=, &result](auto const cmd, auto const data) {
            result |= (cmd == expected_cmd)
                && static_cast<decltype(expected_result)>(data) == expected_result;
        });
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
              , bkrl::get_item_result::select);

            // should not yet have any items
            REQUIRE(creature.item_list().empty());

            bool got_ok = false;
            expect_command_result(got_ok, bkrl::command_type::get, bkrl::get_item_result::ok);
            select_first_item();
            REQUIRE(got_ok);

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
                  , bkrl::drop_item_result::select);

                bool got_ok = false;
                expect_command_result(got_ok, bkrl::command_type::drop, bkrl::drop_item_result::failed);
                select_first_item();
                REQUIRE(got_ok);

                REQUIRE(!creature.item_list().empty());
            }

            SECTION("good location") {
                m.at(p).type = bkrl::terrain_type::floor;

                expect_result(bkrl::drop_item(ctx, creature, m, p, imenu, commands)
                  , bkrl::drop_item_result::select);

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
              , bkrl::show_inventory_result::select);

            // equip via the inventory
            REQUIRE(!creature.item_list().begin()->flags().test(bkrl::item_flag::is_equipped));
            REQUIRE(!creature.equip_list().is_equipped(bkrl::equip_slot::torso));

            bool got_ok = false;
            expect_command_result(got_ok, bkrl::command_type::show_equipment, bkrl::equip_result_t::ok);
            equip_first_item();
            REQUIRE(got_ok);

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

    SECTION("open / close") {
        auto const make_door_at = [&](bklib::ipoint2 const where, bkrl::door::state const state) {
            auto& ter =  m.at(where);
            bkrl::door d {};
            d.set_open_close(state);

            ter.type = bkrl::terrain_type::door;
            ter = d;
        };

        SECTION("no door") {
            expect_result(bkrl::open(ctx, creature, m, commands)
              , bkrl::open_result::nothing);

            expect_result(bkrl::close(ctx, creature, m, commands)
              , bkrl::close_result::nothing);
        }

        SECTION("closed door") {
            make_door_at(p + bklib::ivec2 {1, 1}, bkrl::door::state::closed);

            expect_result(bkrl::close(ctx, creature, m, commands)
              , bkrl::close_result::nothing);

            expect_result(bkrl::open(ctx, creature, m, commands)
              , bkrl::open_result::ok);
        }

        SECTION("open door") {
            make_door_at(p + bklib::ivec2 {1, 1}, bkrl::door::state::open);

            expect_result(bkrl::open(ctx, creature, m, commands)
              , bkrl::open_result::nothing);

            expect_result(bkrl::close(ctx, creature, m, commands)
              , bkrl::close_result::ok);
        }

        SECTION("multiple closed doors") {
            make_door_at(p + bklib::ivec2 {1, 1}, bkrl::door::state::closed);
            make_door_at(p + bklib::ivec2 {1, 0}, bkrl::door::state::closed);
            make_door_at(p + bklib::ivec2 {0, 1}, bkrl::door::state::closed);

            expect_result(bkrl::close(ctx, creature, m, commands)
              , bkrl::close_result::nothing);

            expect_result(bkrl::open(ctx, creature, m, commands)
              , bkrl::open_result::select);

            bool got_ok = false;
            expect_command_result(got_ok, bkrl::command_type::open, bkrl::open_result::ok);
            commands.send_command(bkrl::command {bkrl::command_type::dir_east, 0, 0});
            REQUIRE(got_ok);
        }

        SECTION("multiple open doors") {
            make_door_at(p + bklib::ivec2 {1, 1}, bkrl::door::state::open);
            make_door_at(p + bklib::ivec2 {1, 0}, bkrl::door::state::open);
            make_door_at(p + bklib::ivec2 {0, 1}, bkrl::door::state::open);

            expect_result(bkrl::open(ctx, creature, m, commands)
              , bkrl::open_result::nothing);

            expect_result(bkrl::close(ctx, creature, m, commands)
              , bkrl::close_result::select);

            bool got_ok = false;
            expect_command_result(got_ok, bkrl::command_type::close, bkrl::close_result::ok);
            commands.send_command(bkrl::command {bkrl::command_type::dir_east, 0, 0});
            REQUIRE(got_ok);
        }
    }
}

#endif // BK_NO_UNIT_TESTS

