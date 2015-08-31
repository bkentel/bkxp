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

namespace bkrl {

template <typename C>
auto index_to_iterator(int const where, C&& c)
{
    auto const size = static_cast<int>(c.size());
    BK_PRECONDITION((where == -1 || where >= 0) && where <= size);

    using std::end;
    using std::begin;

    return (where == -1) || (where == size)
        ? end(c)
        : std::next(begin(c), where);
}

struct listbox_data {
    struct col_t {
        bklib::utf8_string label;
        int width_min;
        int width_max;
    };

    struct cell_t {
        bklib::utf8_string label;
    };

    using cells_t = std::vector<cell_t>;

    std::vector<col_t>   cols;
    std::vector<cells_t> cells;
};

struct listbox_render_data {
    using cells_t = std::vector<text_layout>;
    std::vector<cells_t> cells;
};

struct listbox_base {
    static constexpr int const col_shortcut = -2;
    static constexpr int const col_icon     = -1;
    static constexpr int const col_default  =  0;

    enum class unit  : int {
        px
      , em
      , pct
    };

    enum class event : int {
        on_hover, on_mouse_down, on_mouse_up, on_select, on_confirm, on_cancel
    };

    void insert_col(int where, bklib::utf8_string_view label);
    void append_col(bklib::utf8_string_view label);

    int cols() const noexcept {
        return static_cast<int>(data_.cols.size());
    }

    int cols_total() const noexcept {
        return cols() + (use_shortcuts_ ? 1 : 0) + (use_icons_ ? 1 : 0);
    }
protected:
    auto insert_row_(int const where) {
        auto const size = cols_total();

        auto const it0 = data_.cells.insert(
            index_to_iterator(where, data_.cells), listbox_data::cells_t {});

        auto const it1 = render_data_.cells.insert(
            index_to_iterator(where, render_data_.cells), listbox_render_data::cells_t {});

        it0->reserve(size);
        it1->reserve(size);

        return std::make_pair(it0, it1);
    }

    template <typename T, typename F>
    void insert_row_(int const where, F&& query, T const& value) {
        auto& cells = *insert_row_(where).first;

        auto const append = [&](int const i) {
            cells.emplace_back(listbox_data::cell_t {query(i, value)});
        };

        if (use_shortcuts_) { append(col_shortcut); }
        if (use_icons_)     { append(col_icon); }

        append(col_default);
    }

    listbox_data        data_;
    listbox_render_data render_data_;

    bool use_shortcuts_ = true;
    bool use_icons_     = true;
};

template <typename T>
class listbox : listbox_base {
public:
    using query_t = std::function<bklib::utf8_string (int, T const&)>;

    explicit listbox(query_t query)
      : listbox_base {}
      , query_ {std::move(query)}
    {
    }

    int rows() const noexcept {
        return static_cast<int>(rows_.size());
    }

    T const& row_data(int const where) const noexcept {
        return *index_to_iterator(where, rows_);
    }

    T const& row_data(int const where) noexcept {
        return *index_to_iterator(where, rows_);
    }

    template <typename U>
    void insert_row(int const where, U&& value) {
        insert_row_(where, std::forward<U>(value));
    }

    template <typename U>
    void append_row(U&& value) {
        using std::end;
        insert_row_(rows(), std::forward<U>(value));
    }

    void remove_col(int where);
    void remove_row(int where);

    void set_col_width(int col, double value, unit);
    void set_col_width(int col, double min, double max, unit);

    void set_width(double value, unit);
    void set_height(double value, unit);
private:
    template <typename U>
    void insert_row_(int const where, U&& value) {
        static_assert(std::is_convertible<std::decay_t<U>, T>::value, "");

        listbox_base::insert_row_(where, query_, value);
        rows_.insert(index_to_iterator(where, rows_), std::forward<U>(value));
    }

    std::vector<T> rows_;
    query_t        query_;
};

}

TEST_CASE("listbox") {
    static constexpr bklib::utf8_string_view const row_data[] = {
        bklib::make_string_view("row 0")
      , bklib::make_string_view("row 1")
      , bklib::make_string_view("row 2")
    };

    bkrl::listbox<bklib::utf8_string> list {[&](int const i, bklib::utf8_string const& s) {
        switch (i) {
        case bkrl::listbox_base::col_shortcut :
            return s + " shortcut";
        case bkrl::listbox_base::col_icon :
            return s + " icon";
        case bkrl::listbox_base::col_default :
            return s;
        default:
            break;
        }

        return s + " " + std::to_string(i);
    }};

    REQUIRE(list.rows() == 0);

    for (auto const& row : row_data) {
        list.append_row(row.to_string());
    }

    REQUIRE(list.rows() == 3);

    REQUIRE(list.row_data(0) == row_data[0]);
    REQUIRE(list.row_data(1) == row_data[1]);
    REQUIRE(list.row_data(2) == row_data[2]);
}

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
