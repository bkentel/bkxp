#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "listbox.hpp"

TEST_CASE("listbox") {
    static constexpr bklib::utf8_string_view const row_data[] = {
        bklib::make_string_view("row 0")
      , bklib::make_string_view("row 1")
      , bklib::make_string_view("row 2")
    };

    static constexpr int const list_w = 400;
    static constexpr int const list_h = 200;

    auto trender_ptr = bkrl::make_text_renderer();
    auto& trender = *trender_ptr;

    auto const line_h = trender.line_spacing();

    bkrl::listbox<bklib::utf8_string> list {list_w, list_h, [&](int const i, bklib::utf8_string const& s) {
        return s + " col " + std::to_string(i);
    }};

    auto const make_list = [&] {
        list.clear();

        list.append_col("Col 0");
        list.append_col("Col 1");
        list.append_col("Col 2");

        for (auto const& row : row_data) {
            list.append_row(row.to_string());
        }
    };

    SECTION("sanity") {
        REQUIRE(list.rows()       == 0);
        REQUIRE(list.cols()       == 0);

        REQUIRE(list.is_visible() == false);

        REQUIRE(list.width()  == list_w);
        REQUIRE(list.height() == list_h);

        REQUIRE(list.content_width()  == list_w);
        REQUIRE(list.content_height() == list_h);

        REQUIRE(list.scroll_width()  == 0);
        REQUIRE(list.scroll_height() == 0);

        REQUIRE(list.current_selection() == 0);

        REQUIRE(list.col_left(0) == 0);
    }

    SECTION("select") {
        make_list();
        REQUIRE(list.current_selection() == 0);

        list.select_next(3);
        REQUIRE(list.current_selection() == 0);

        list.select_next(-3);
        REQUIRE(list.current_selection() == 0);

        list.select_next(1);
        REQUIRE(list.current_selection() == 1);

        list.select_next(-2);
        REQUIRE(list.current_selection() == 2);
    }

    SECTION("append") {
        REQUIRE(list.rows() == 0);

        make_list();

        REQUIRE(list.rows() == 3);

        REQUIRE(list.row_data(0) == row_data[0]);
        REQUIRE(list.row_data(1) == row_data[1]);
        REQUIRE(list.row_data(2) == row_data[2]);
    }

    SECTION("insert") {
        REQUIRE(list.rows() == 0);

        list.append_row(row_data[1].to_string());
        list.insert_row(0, row_data[0].to_string());
        list.insert_row(-1, row_data[2].to_string());

        REQUIRE(list.rows() == 3);

        REQUIRE(list.row_data(0) == row_data[0]);
        REQUIRE(list.row_data(1) == row_data[1]);
        REQUIRE(list.row_data(2) == row_data[2]);
    }

    SECTION("remove") {
        make_list();
        list.remove_row(0);
        list.remove_row(-1);

        REQUIRE(list.rows() == 1);
        REQUIRE(list.row_data(0) == row_data[1]);
    }

    SECTION("hit test") {
        make_list();
        list.update_layout(trender);

        auto const check_hit_test = [&](bklib::ipoint2 const p, int const row, int const col, bool const ok) {
            auto const result = list.hit_test(p);
            REQUIRE(result.row == row);
            REQUIRE(result.col == col);
            REQUIRE(result.ok  == ok);
        };

        auto const c0 = list.col_left(0);
        auto const c1 = list.col_left(1);
        auto const c2 = list.col_left(2);

        // valid
        check_hit_test(bklib::ipoint2 {c0, line_h * 0}, 0, 0, true);
        check_hit_test(bklib::ipoint2 {c0, line_h * 1}, 1, 0, true);
        check_hit_test(bklib::ipoint2 {c0, line_h * 2}, 2, 0, true);

        check_hit_test(bklib::ipoint2 {c1, line_h * 0}, 0, 1, true);
        check_hit_test(bklib::ipoint2 {c1, line_h * 1}, 1, 1, true);
        check_hit_test(bklib::ipoint2 {c1, line_h * 2}, 2, 1, true);

        check_hit_test(bklib::ipoint2 {c2, line_h * 0}, 0, 2, true);
        check_hit_test(bklib::ipoint2 {c2, line_h * 1}, 1, 2, true);
        check_hit_test(bklib::ipoint2 {c2, line_h * 2}, 2, 2, true);

        //invalid
        check_hit_test(bklib::ipoint2 {c0 - 1, line_h * 0}, 0, 0, false);
        check_hit_test(bklib::ipoint2 {c0 - 1, line_h * 1}, 0, 0, false);
        check_hit_test(bklib::ipoint2 {c0 - 1, line_h * 2}, 0, 0, false);

    }
}

#endif // BK_NO_UNIT_TESTS
