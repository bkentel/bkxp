#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "inventory.hpp"
#include "item.hpp"
#include "context.hpp"
#include "bklib/dictionary.hpp"
#include "text.hpp"
#include "definitions.hpp"
#include "system.hpp"
#include "random.hpp"

TEST_CASE("inventory", "[inventory][bkrl]") {
    auto text_render = bkrl::make_text_renderer();
    bkrl::text_renderer& trender = *text_render;

    SECTION("sanity checks") {
        auto il = bkrl::make_item_list(trender);
        bkrl::inventory& i = *il;
        auto const b = i.bounds();

        REQUIRE(!!b);
        REQUIRE(x(i.position()) == b.left);
        REQUIRE(y(i.position()) == b.top);
        REQUIRE(i.count() == 0);
        REQUIRE(i.is_visible() == false);
    }

    SECTION("size checks") {
        auto il = bkrl::make_item_list(trender);
        bkrl::inventory& i = *il;
        auto const b = i.bounds();

        i.move_to(bklib::ipoint2 {0, 0});
        auto const b0 = i.bounds();

        REQUIRE(b.width()  == b0.width());
        REQUIRE(b.height() == b0.height());
        REQUIRE(b0.top == 0);
        REQUIRE(b0.left == 0);

        i.move_by(bklib::ivec2 {10, 20});
        auto const b1 = i.bounds();

        REQUIRE(b.width()  == b1.width());
        REQUIRE(b.height() == b1.height());
        REQUIRE(b1.left == 10);
        REQUIRE(b1.top == 20);
    }

    bkrl::random_t        random;
    bkrl::item_dictionary idefs;
    bkrl::item_factory    ifac;

    bkrl::item_def const idef0 {"item 0"};
    bkrl::item_def const idef1 {"item 1"};

    idefs.insert_or_discard(idef0);
    idefs.insert_or_discard(idef1);

    bkrl::definitions const defs {nullptr, nullptr, nullptr};

    bkrl::context ctx {
        *static_cast<bkrl::random_state*>(nullptr)
      , defs
      , *static_cast<bkrl::output*>(nullptr)
      , *static_cast<bkrl::item_factory*>(nullptr)
      , *static_cast<bkrl::creature_factory*>(nullptr)
    };

    bkrl::item_pile pile;
    pile.insert(ifac.create(random, idefs, idef0));
    pile.insert(ifac.create(random, idefs, idef1));

    auto il = bkrl::make_item_list(trender);
    bkrl::inventory& i = *il;

    auto expected_action = bkrl::inventory::action::cancel;
    auto expected_index  = 0;
    bkrl::item const* expected_item = nullptr;

    using action = bkrl::inventory::action;

    i.set_on_action([&](action const type, int const index) {
        REQUIRE(type == expected_action);
        REQUIRE(index == expected_index);

        auto const ptr = &*i.data();
        REQUIRE(ptr == expected_item);
    });

    auto f = default_item_list_handler(bkrl::populate_item_list(
        ctx, i, pile, "title"));

    auto const set_expected = [&](auto const a, auto const index, auto const it) {
        expected_action = a;
        expected_index  = index;
        expected_item   = &*std::next(pile.begin(), it);
    };

    //----------------------------- test scrolling down---------------------------------------------
    set_expected(action::select, 0, 0);
    f(bkrl::command {bkrl::command_type::dir_south, 0, 0});

    set_expected(action::select, 1, 1);
    f(bkrl::command {bkrl::command_type::dir_south, 0, 0});

    set_expected(action::select, 0, 0);
    f(bkrl::command {bkrl::command_type::dir_south, 0, 0});

    //----------------------------- test scrolling up-----------------------------------------------
    set_expected(action::select, 1, 1);
    f(bkrl::command {bkrl::command_type::dir_north, 0, 0});

    set_expected(action::select, 0, 0);
    f(bkrl::command {bkrl::command_type::dir_north, 0, 0});

    //----------------------------- test selecting via hotkey---------------------------------------
    set_expected(action::select, 0, 0);
    f(bkrl::command {bkrl::command_type::text, 1, reinterpret_cast<intptr_t>(std::addressof("a"))});

    set_expected(action::select, 1, 1);
    f(bkrl::command {bkrl::command_type::text, 1, reinterpret_cast<intptr_t>(std::addressof("b"))});

    set_expected(action::select, 1, 1);
    f(bkrl::command {bkrl::command_type::text, 1, reinterpret_cast<intptr_t>(std::addressof("c"))});

    //----------------------------- test cancel and confirm-----------------------------------------
    set_expected(action::cancel, 1, 1);
    f(bkrl::command {bkrl::command_type::cancel, 0, 0});

    set_expected(action::confirm, 1, 1);
    f(bkrl::command {bkrl::command_type::confirm, 0, 0});

    //----------------------------- test selection with mouse---------------------------------------
    auto const c = i.client_area();

    set_expected(action::select, 0, 0);
    i.on_mouse_move(bkrl::mouse_state {c.left, c.top, 0, 0, 0, 0, 0, 0});

    set_expected(action::select, 1, 1);
    i.on_mouse_move(bkrl::mouse_state {c.left, c.top + + i.row_height(), 0, 0, 0, 0, 0, 0});

    set_expected(action::confirm, 0, 0);
    i.on_mouse_button(bkrl::mouse_button_state {c.left, c.top, 0, 1, 1, 1});

    set_expected(action::confirm, 1, 1);
    i.on_mouse_button(bkrl::mouse_button_state {c.left, c.top + i.row_height(), 0, 1, 1, 1});
}

#endif // BK_NO_UNIT_TESTS
