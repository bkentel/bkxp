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

    auto text_render = bkrl::make_text_renderer();
    bkrl::text_renderer& trender = *text_render;

    auto commands_ptr = bkrl::make_command_translator();
    auto& commands = *commands_ptr;

    bkrl::definitions const defs {&cdefs, &idefs, nullptr};

    bkrl::item_def const idef0 = [] {
        bkrl::item_def def {"item 0"};
        def.flags.set(bkrl::item_flag::is_equippable);
        def.slots.set(bkrl::equip_slot::torso);
        return def;
    }();

    bkrl::item_def const idef1 = [] {
        bkrl::item_def def {"item 1"};
        def.flags.set(bkrl::item_flag::is_container);
        return def;
    }();

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

    auto il = bkrl::make_item_list(trender);
    bkrl::inventory& imenu = *il;

    auto const p =  bklib::ipoint2 {1, 1};
    auto creature = cfac.create(random, cdef0, p);

    // helper to select the first (and only) item
    auto const select_first_item = [&] {
        imenu.on_command(bkrl::command {bkrl::command_type::dir_south, 0, 0});
        imenu.on_command(bkrl::command {bkrl::command_type::confirm, 0, 0});
    };

    auto const equip_first_item = [&] {
        imenu.on_command(bkrl::command {bkrl::command_type::dir_south, 0, 0});
        imenu.on_command(bkrl::make_command(bkrl::command_raw_t {
            bkrl::key_mod_state {bkrl::key_mod::ctrl}, 'e'
        }));
    };

    using ctype   = bkrl::command_type;
    using cresult = bkrl::command_result;

    std::vector<std::pair<ctype, cresult>> command_results;
    commands.set_command_result_handler([&](ctype const ct, cresult const cr) {
        command_results.push_back(std::make_pair(ct, cr));
    });

    auto const check_cmd_result = [&](size_t const i, ctype const ct, cresult const cr) {
        return command_results[i] == std::make_pair(ct, cr);
    };

    SECTION("get item") {
        SECTION("nothing present") {
            bkrl::get_item_at(ctx, commands, creature, m, imenu);
            REQUIRE(command_results.size() == 1);
            REQUIRE(check_cmd_result(0, ctype::get, cresult::none_present));
        }

        SECTION("distant item") {
            auto const where = p + bklib::ivec2 {2, 2};
            bkrl::get_item_at(ctx, commands, creature, m, where, imenu);
            REQUIRE(command_results.size() == 1);
            REQUIRE(check_cmd_result(0, ctype::get, cresult::out_of_range));
        }

        SECTION("good location") {
            // need a valid tile for the item
            m.at(p).type = bkrl::terrain_type::floor;
            bkrl::generate_item(ctx, m, idef0, p);

            bkrl::get_item_at(ctx, commands, creature, m, imenu);

            // should not yet have any items
            REQUIRE(command_results.size() == 0);
            REQUIRE(creature.item_list().empty());
            select_first_item();

            REQUIRE(command_results.size() == 2);
            REQUIRE(check_cmd_result(0, ctype::get, cresult::ok_advance));
            REQUIRE(check_cmd_result(1, ctype::get, cresult::none_present));

            auto const& items = creature.item_list();
            REQUIRE(!items.empty());
            REQUIRE(items.begin()->def() == idef0);

            REQUIRE_FALSE(m.items_at(p));
        }
    }

    SECTION("drop item") {
        SECTION("no items") {
            bkrl::drop_item(ctx, commands, creature, m, imenu);
            REQUIRE(command_results.size() == 1);
            REQUIRE(check_cmd_result(0, ctype::drop, cresult::none_present));
        }

        SECTION("with one item") {
            creature.get_item(ifac.create(random, idefs, idef0));

            SECTION("distant location") {
                auto const where = p + bklib::ivec2 {2, 2};
                bkrl::drop_item_at(ctx, commands, creature, m, where, imenu);

                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::drop, cresult::out_of_range));
                REQUIRE(!creature.item_list().empty());
            }

            SECTION("bad location") {
                bkrl::drop_item(ctx, commands, creature, m, imenu);
                REQUIRE(command_results.size() == 0);
                REQUIRE(!creature.item_list().empty());

                select_first_item();

                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::drop, cresult::failed));
                REQUIRE(!creature.item_list().empty());
            }

            SECTION("good location") {
                m.at(p).type = bkrl::terrain_type::floor;

                bkrl::drop_item(ctx, commands, creature, m, imenu);
                REQUIRE(command_results.size() == 0);
                REQUIRE(!creature.item_list().empty());

                select_first_item();

                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::drop, cresult::ok_advance));
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
            bkrl::display_item_list(ctx, commands, creature, m, creature.item_list(), imenu, "");
            commands.send_command(bkrl::make_command<ctype::cancel>());

            REQUIRE(command_results.size() == 2);
            REQUIRE(check_cmd_result(0, ctype::show_inventory, cresult::ok_no_advance));
            REQUIRE(check_cmd_result(1, ctype::show_inventory, cresult::canceled));
        }

        SECTION("with one item") {
            creature.get_item(ifac.create(random, idefs, idef0));

            bkrl::display_item_list(ctx, commands, creature, m, creature.item_list(), imenu, "");
            commands.send_command(bkrl::make_command<ctype::cancel>());

            REQUIRE(command_results.size() == 2);
            REQUIRE(check_cmd_result(0, ctype::show_inventory, cresult::ok_no_advance));
            REQUIRE(check_cmd_result(1, ctype::show_inventory, cresult::canceled));
        }
    }

    SECTION("equip item") {
        auto const equip_and_expect = [&](int const index, auto const expected) {
            auto const result = bkrl::equip_item(ctx, creature, creature.item_list().advance(index));
            REQUIRE(result == expected);
        };

        SECTION("not equippable") {
            creature.get_item(ifac.create(random, idefs, idef1));
            equip_and_expect(0, bkrl::equip_result_t::not_equippable);
        }

        SECTION("already equipped") {
            creature.get_item(ifac.create(random, idefs, idef0));
            equip_and_expect(0, bkrl::equip_result_t::ok);
            equip_and_expect(0, bkrl::equip_result_t::already_equipped);
        }

        SECTION("occupied") {
            creature.get_item(ifac.create(random, idefs, idef0));
            creature.get_item(ifac.create(random, idefs, idef0));

            equip_and_expect(0, bkrl::equip_result_t::ok);
            equip_and_expect(1, bkrl::equip_result_t::slot_occupied);
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
            SECTION("open") {
                bkrl::open_around(ctx, commands, creature, m, imenu);
                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::open, cresult::none_present));
            }

            SECTION("close") {
                bkrl::close_around(ctx, commands, creature, m);
                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::close, cresult::none_present));
            }
        }

        SECTION("closed door") {
            make_door_at(p + bklib::ivec2 {1, 1}, bkrl::door::state::closed);

            SECTION("open") {
                bkrl::open_around(ctx, commands, creature, m, imenu);
                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::open, cresult::ok_advance));
            }

            SECTION("close") {
                bkrl::close_around(ctx, commands, creature, m);
                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::close, cresult::none_present));
            }
        }

        SECTION("open door") {
            make_door_at(p + bklib::ivec2 {1, 1}, bkrl::door::state::open);

            SECTION("open") {
                bkrl::open_around(ctx, commands, creature, m, imenu);
                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::open, cresult::none_present));
            }

            SECTION("close") {
                bkrl::close_around(ctx, commands, creature, m);
                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::close, cresult::ok_advance));
            }
        }

        SECTION("multiple closed doors") {
            make_door_at(p + bklib::ivec2 {1, 1}, bkrl::door::state::closed);
            make_door_at(p + bklib::ivec2 {1, 0}, bkrl::door::state::closed);
            make_door_at(p + bklib::ivec2 {0, 1}, bkrl::door::state::closed);

            SECTION("open") {
                bkrl::open_around(ctx, commands, creature, m, imenu);
                commands.send_command(bkrl::make_command<ctype::dir_east>());

                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::open, cresult::ok_advance));
            }

            SECTION("close") {
                bkrl::close_around(ctx, commands, creature, m);
                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::close, cresult::none_present));
            }
        }

        SECTION("multiple open doors") {
            make_door_at(p + bklib::ivec2 {1, 1}, bkrl::door::state::open);
            make_door_at(p + bklib::ivec2 {1, 0}, bkrl::door::state::open);
            make_door_at(p + bklib::ivec2 {0, 1}, bkrl::door::state::open);

            SECTION("open") {
                bkrl::open_around(ctx, commands, creature, m, imenu);

                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::open, cresult::none_present));
            }

            SECTION("close") {
                bkrl::close_around(ctx, commands, creature, m);
                commands.send_command(bkrl::make_command<ctype::dir_east>());

                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::close, cresult::ok_advance));
            }
        }

        auto const make_container_at = [&](bklib::ipoint2 const where) {
            m.at(where).type = bkrl::terrain_type::floor;
            bkrl::generate_item(ctx, m, idef1, where);
        };

        SECTION("open adjacent container and cancel") {
            auto const check_adjacent_container = [&](size_t const i) {
                make_container_at(bkrl::index_to_offset(creature.position(), i));
                bkrl::open_around(ctx, commands, creature, m, imenu);
                commands.send_command(bkrl::make_command<ctype::cancel>());

                REQUIRE(command_results.size() == 3);
                REQUIRE(check_cmd_result(0, ctype::open, cresult::ok_advance));
                REQUIRE(check_cmd_result(1, ctype::show_inventory, cresult::ok_no_advance));
                REQUIRE(check_cmd_result(2, ctype::show_inventory, cresult::canceled));
            };

            SECTION("case 0") { check_adjacent_container(0); }
            SECTION("case 1") { check_adjacent_container(1); }
            SECTION("case 2") { check_adjacent_container(2); }
            SECTION("case 3") { check_adjacent_container(3); }
            SECTION("case 4") { check_adjacent_container(4); }
            SECTION("case 5") { check_adjacent_container(5); }
            SECTION("case 6") { check_adjacent_container(6); }
            SECTION("case 7") { check_adjacent_container(7); }
            SECTION("case 8") { check_adjacent_container(8); }
        }

        SECTION("open adjacent container") {
            make_container_at(p);
            bkrl::open_around(ctx, commands, creature, m, imenu);
            bkrl::item_pile const& container =
                *bkrl::get_item_data<bkrl::item_data_type::container>(*m.items_at(p)->begin());

            SECTION("... and get") {
                auto const size_before = std::distance(container.begin(), container.end());

                commands.send_command(bkrl::make_command<ctype::dir_south>());
                commands.send_command(make_command(bkrl::command_raw_t {bkrl::key_mod::ctrl, 'g'}));
                commands.send_command(bkrl::make_command<ctype::cancel>());

                REQUIRE(command_results.size() == 4);
                REQUIRE(check_cmd_result(0, ctype::open, cresult::ok_advance));
                REQUIRE(check_cmd_result(1, ctype::show_inventory, cresult::ok_no_advance));
                REQUIRE(check_cmd_result(2, ctype::get, cresult::ok_advance));
                REQUIRE(check_cmd_result(3, ctype::show_inventory, cresult::canceled));

                auto const size_after = std::distance(container.begin(), container.end());
                REQUIRE(size_after == (size_before - 1));
                REQUIRE(!creature.item_list().empty());
            }
        }

        SECTION("open door or container") {
            make_container_at(p);
            make_door_at(bkrl::index_to_offset(p, 0), bkrl::door::state::closed);

            SECTION("container") {
                bkrl::open_around(ctx, commands, creature, m, imenu);
                commands.send_command(bkrl::make_command<ctype::yes>());
                commands.send_command(bkrl::make_command<ctype::cancel>());

                REQUIRE(command_results.size() == 3);
                REQUIRE(check_cmd_result(0, ctype::open, cresult::ok_advance));
                REQUIRE(check_cmd_result(1, ctype::show_inventory, cresult::ok_no_advance));
                REQUIRE(check_cmd_result(2, ctype::show_inventory, cresult::canceled));
            }

            SECTION("multiple containers; different position") {
                make_container_at(bkrl::index_to_offset(p, 1));

                bkrl::open_around(ctx, commands, creature, m, imenu);
                commands.send_command(bkrl::make_command<ctype::yes>());
                REQUIRE(command_results.size() == 0);

                commands.send_command(bkrl::make_command<ctype::dir_north>());
                commands.send_command(bkrl::make_command<ctype::cancel>());

                REQUIRE(command_results.size() == 3);
                REQUIRE(check_cmd_result(0, ctype::open, cresult::ok_advance));
                REQUIRE(check_cmd_result(1, ctype::show_inventory, cresult::ok_no_advance));
                REQUIRE(check_cmd_result(2, ctype::show_inventory, cresult::canceled));
            }

            SECTION("door") {
                bkrl::open_around(ctx, commands, creature, m, imenu);
                commands.send_command(bkrl::make_command<ctype::no>());

                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::open, cresult::ok_advance));
            }

            SECTION("multiple doors") {
                make_door_at(bkrl::index_to_offset(p, 1), bkrl::door::state::closed);

                bkrl::open_around(ctx, commands, creature, m, imenu);
                commands.send_command(bkrl::make_command<ctype::no>());
                commands.send_command(bkrl::make_command<ctype::dir_north>());

                REQUIRE(command_results.size() == 1);
                REQUIRE(check_cmd_result(0, ctype::open, cresult::ok_advance));
            }
        }
    }

    SECTION("inspect tile") {
        //TODO these checks are pretty primitive, but perhaps enough

        auto const count_lines = [](auto const& str) {
            return std::count(begin(str), end(str), '\n');
        };

        SECTION("out of bounds") {
            auto const b = m.bounds();

            REQUIRE("" == bkrl::inspect_tile(ctx, creature, m, bklib::ipoint2 {b.left - 1, b.top}));
            REQUIRE("" == bkrl::inspect_tile(ctx, creature, m, bklib::ipoint2 {b.left, b.top - 1}));
            REQUIRE("" == bkrl::inspect_tile(ctx, creature, m, bklib::ipoint2 {b.right, b.top}));
            REQUIRE("" == bkrl::inspect_tile(ctx, creature, m, bklib::ipoint2 {b.left, b.bottom}));
        }

        SECTION("no items, no creatures") {
            auto const str = bkrl::inspect_tile(ctx, creature, m, bklib::ipoint2 {0, 0});
            REQUIRE(!str.empty());
            REQUIRE(count_lines(str) == 0);
        }

        SECTION("no items, one creature") {
            auto const where = bklib::ipoint2 {0, 0};
            m.at(where).type = bkrl::terrain_type::floor;

            bkrl::generate_creature(ctx, m, cdef0, where);
            auto const str = bkrl::inspect_tile(ctx, creature, m, where);
            REQUIRE(count_lines(str) == 3);
        }

        SECTION("one item, one creature") {
            auto const where = bklib::ipoint2 {0, 0};
            m.at(where).type = bkrl::terrain_type::floor;

            bkrl::generate_creature(ctx, m, cdef0, where);
            bkrl::generate_item(ctx, m, idef0, where);

            auto const str = bkrl::inspect_tile(ctx, creature, m, where);
            REQUIRE(count_lines(str) == 5);
        }

        SECTION("two items, one creature") {
            auto const where = bklib::ipoint2 {0, 0};
            m.at(where).type = bkrl::terrain_type::floor;

            bkrl::generate_creature(ctx, m, cdef0, where);
            bkrl::generate_item(ctx, m, idef0, where);
            bkrl::generate_item(ctx, m, idef0, where);

            auto const str = bkrl::inspect_tile(ctx, creature, m, where);
            REQUIRE(count_lines(str) == 6);
        }
    }

    SECTION("quit") {
        bkrl::display_quit_prompt(ctx, commands);

        SECTION("yes") {
            commands.send_command(bkrl::make_command<ctype::yes>());
            REQUIRE(command_results.size() == 1);
            REQUIRE(command_results[0] == std::make_pair(ctype::quit, cresult::ok_advance));
        }

        SECTION("no") {
            commands.send_command(bkrl::make_command<ctype::no>());
            REQUIRE(command_results.size() == 1);
            REQUIRE(command_results[0] == std::make_pair(ctype::quit, cresult::canceled));
        }

        SECTION("cancel") {
            commands.send_command(bkrl::make_command<ctype::cancel>());
            REQUIRE(command_results.size() == 1);
            REQUIRE(command_results[0] == std::make_pair(ctype::quit, cresult::canceled));
        }
    }
}

#endif // BK_NO_UNIT_TESTS

