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

    using std::begin;
    using std::end;

    return (where == -1) ?           end(c)
         : (size && where == size) ? std::next(begin(c), size - 1)
                                   : std::next(begin(c), where);
}

enum class layout_unit_t : int {
    px
  , em
  , pct
};

struct listbox_data {
    struct col_t {
        col_t(bklib::utf8_string col_label
            , double const min
            , double const max
            , layout_unit_t const units
        ) : label {std::move(col_label)}
          , width_min_value {min}
          , width_max_value {max}
          , unit {units}
        {
        }

        explicit col_t(bklib::utf8_string col_label = "")
          : label {std::move(col_label)}
        {
        }

        bklib::utf8_string label;

        int width     = 0; //!< actual width
        int width_min = 0; //!< minimum width in pixels
        int width_max = 0; //!< maximum width in pixels

        double width_min_value = 0.0; //! minimum width in units given by unit
        double width_max_value = 0.0; //! maximum width in units given by unit
        layout_unit_t unit     = layout_unit_t::pct; //! unit type
    };

    struct cell_t {
        bklib::utf8_string label;
    };

    using cells_t = std::vector<cell_t>;

    void clear() {
        cols.clear();
        cells.clear();
    }

    std::vector<col_t>   cols;
    std::vector<cells_t> cells;
};

struct listbox_render_data {
    using cells_t = std::vector<text_layout>;

    void clear() {
        cells.clear();
    }

    std::vector<cells_t> cells;
};

class listbox_base {
public:
    static constexpr int const col_shortcut = -2;
    static constexpr int const col_icon     = -1;
    static constexpr int const col_default  =  0;

    enum class event : int {
        on_hover, on_mouse_down, on_mouse_up, on_select, on_confirm, on_cancel
    };

    void insert_col(int const where, bklib::utf8_string label) {
        data_.cols.insert(index_to_iterator(where, data_.cols)
            , listbox_data::col_t {std::move(label)});
    }

    void append_col(bklib::utf8_string label) {
        insert_col(cols(), std::move(label));
    }

    void remove_col(int const where) {
        data_.cols.erase(index_to_iterator(where, data_.cols));
    }

    void set_col_width(int const col, double const min, double const max, layout_unit_t const u) {
        auto& c = *index_to_iterator(col, data_.cols);
        c.unit = u;
        c.width_min_value = min;
        c.width_max_value = max;
    }

    void set_col_width(int const col, double const value, layout_unit_t const u) {
        set_col_width(col, value, value, u);
    }

    int cols() const noexcept {
        return cols_total() - 1 - (use_shortcuts_ ? 1 : 0) - (use_icons_ ? 1 : 0);
    }

    int rows() const noexcept {
        return static_cast<int>(data_.cells.size());
    }

    int cols_total() const noexcept {
        return static_cast<int>(data_.cols.size());
    }

    int width() const noexcept {
        return bounds_.width();
    }

    int height() const noexcept {
        return bounds_.height();
    }

    void set_width(int const value) noexcept {
        BK_PRECONDITION(value > 0);
        bounds_.right = bounds_.left + value;
    }

    void set_height(int const value) noexcept {
        BK_PRECONDITION(value > 0);
        bounds_.bottom = bounds_.top + value;
    }

    void update_layout(text_renderer& trender) {
        if (data_.cells.empty()) {
            return;
        }

        auto const n_rows = static_cast<size_t>(rows());
        auto const n_cols = static_cast<size_t>(cols_total());
        auto const em     = x(trender.bbox());
        auto const w      = width();
        auto const h      = height();

        //
        // clear old render data
        //
        for (auto& row : render_data_.cells) {
            for (auto& cell : row) {
                cell.clear();
            }

            row.resize(n_cols, text_layout {trender, ""});
        }

        render_data_.cells.resize(n_rows);

        //
        // reset column widths
        //
        for (auto& col : data_.cols) {
            col.width = 0;
        }

        //
        // update text & actual widths; calculate total height
        //
        int total_row_height = 0;

        for (auto r = 0u; r < n_rows; ++r) {
            int row_height = 0;

            for (auto c = 0u; c < n_cols; ++c) {
                auto&       cell = render_data_.cells[r][c];
                auto&       col  = data_.cols[c];
                auto const& data = data_.cells[r][c];

                cell.set_text(trender, data.label);

                auto const extent = cell.extent();
                col.width  = std::max(col.width, extent.width());
                row_height = std::max(row_height, extent.height());
            }

            total_row_height += row_height;
        }

        scroll_y_max_ = bklib::clamp_min(total_row_height - height(), 0);

        //
        // update column width min / max; calculate total width
        //
        int total_col_width = 0;

        for (auto& col : data_.cols) {
            switch (col.unit) {
            case layout_unit_t::em:
                col.width_min = bklib::round_to<int>(col.width_min_value * em);
                col.width_max = bklib::round_to<int>(col.width_max_value * em);
                break;
            case layout_unit_t::px:
                col.width_min = bklib::round_to<int>(col.width_min_value);
                col.width_max = bklib::round_to<int>(col.width_max_value);
                break;
            case layout_unit_t::pct:
                col.width_min = bklib::round_to<int>(col.width_min_value * w);
                col.width_max = bklib::round_to<int>(col.width_max_value * h);
                break;
            }

            if (col.width_min || col.width_max) {
                col.width = bklib::clamp(col.width, col.width_min, col.width_max);
            }

            total_col_width += col.width;
        }

        scroll_x_max_ = bklib::clamp_min(total_col_width - width(), 0);

        //
        // update clip
        //
        for (auto r = 0u; r < n_rows; ++r) {
            for (auto c = 0u; c < n_cols; ++c) {
                auto&       cell = render_data_.cells[r][c];
                auto const& col  = data_.cols[c];

                if (cell.extent().width() <= col.width_max) {
                    continue;
                }

                cell.clip_to(bklib::clamp_to<text_layout::size_type>(col.width_max)
                    , text_layout::unlimited);
            }
        }
    }

    void clear() {
        data_.clear();
        render_data_.clear();
        selection_ = 0;
    }

    void reserve(int const rows) {
        data_.cells.reserve(rows);
        render_data_.cells.reserve(rows);
    }

    void select_next(int const offset) noexcept {
        select(selection_ + offset);
    }

    void select(int const index) noexcept {
        auto const n = rows();
        if (n == 0) {
            selection_ = 0;
        } else {
            auto const mod = (index % n);
            selection_ = mod + (mod < 0 ? n : 0);
        }
    }

    void scroll_by(int const dx, int const dy) noexcept {
        scroll_x_ = bklib::clamp(scroll_x_ + dx, 0, scroll_x_max_);
        scroll_y_ = bklib::clamp(scroll_y_ + dy, 0, scroll_y_max_);
    }

    int current_selection() const noexcept {
        return selection_;
    }

    bool is_visible() const noexcept {
        return is_visible_;
    }

    void show(bool const visible) noexcept {
        is_visible_ = visible;
    }

    struct hit_test_t {
        int  row;
        int  col;
        bool ok;
    };

    hit_test_t hit_test(bklib::ipoint2 const p) const noexcept {
        auto const n_rows = rows();
        if (!n_rows || !bklib::intersects(p, bounds_)) {
            return {0, 0, false};
        }

        auto const h = render_data_.cells[0][0].extent().height();
        if (!h) {
            return {0, 0, false};
        }

        auto const x0 = x(p) + scroll_x_ - bounds_.left;
        auto const y0 = y(p) + scroll_y_ - bounds_.top;

        auto const row = bklib::floor_to<int>(y0 / static_cast<double>(h));

        auto const last = end(data_.cols);
        auto const it = std::find_if(begin(data_.cols), last, [&, last_x = 0](auto const& col) mutable {
            auto const result = x0 < col.width + last_x;
            last_x += col.width;
            return result;
        });

        auto const col = static_cast<int>(last - it);

        return {row, col, true};
    }
protected:
    listbox_base(int const width, int const height)
      : bounds_ {bklib::make_rect(0, 0, width, height)}
    {
        BK_PRECONDITION(width  >= 0);
        BK_PRECONDITION(height >= 0);

        data_.cols.resize(1 + (use_shortcuts_ ? 1 : 0) + (use_icons_ ? 1 : 0));
    }

    auto insert_row_(int const where) {
        auto const size = cols_total();

        auto const it0 = data_.cells.insert(
            index_to_iterator(where, data_.cells), listbox_data::cells_t {});

        auto const it1 = render_data_.cells.insert(
            index_to_iterator(where, render_data_.cells), listbox_render_data::cells_t {});

        it0->reserve(size);
        it1->reserve(size);

        return it0;
    }

    template <typename T, typename F>
    void insert_row_(int const where, F&& query, T const& value) {
        auto& cells = *insert_row_(where);

        auto const append = [&](int const i) {
            cells.emplace_back(listbox_data::cell_t {query(i, value)});
        };

        if (use_shortcuts_) { append(col_shortcut); }
        if (use_icons_)     { append(col_icon); }

        append(col_default);
    }

    void remove_row_(int const where) {
        data_.cells.erase(index_to_iterator(where, data_.cells));
        render_data_.cells.erase(index_to_iterator(where, render_data_.cells));
    }

    listbox_data        data_;
    listbox_render_data render_data_;

    bklib::irect bounds_;
    int scroll_x_ = 0;
    int scroll_y_ = 0;
    int scroll_x_max_ = 0;
    int scroll_y_max_ = 0;
    int selection_ = 0;

    bool is_visible_    = false;
    bool use_shortcuts_ = true;
    bool use_icons_     = true;
};

template <typename T>
class listbox final : public listbox_base {
public:
    using query_t = std::function<bklib::utf8_string (int, T const&)>;

    explicit listbox(int const width, int const height, query_t query)
      : listbox_base {width, height}
      , query_ {std::move(query)}
    {
    }

    T const& row_data(int const where) const noexcept {
        return *index_to_iterator(where, rows_);
    }

    T const& row_data(int const where) noexcept {
        return *index_to_iterator(where, rows_);
    }

    template <typename U>
    void insert_row(int const where, U&& value) {
        static_assert(std::is_convertible<std::decay_t<U>, T>::value, "");

        listbox_base::insert_row_(where, query_, value);
        rows_.insert(index_to_iterator(where, rows_), std::forward<U>(value));
    }

    template <typename U>
    void append_row(U&& value) {
        insert_row(-1, std::forward<U>(value));
    }

    void remove_row(int const where) {
        BK_PRECONDITION(!rows_.empty());

        auto const i = (where == -1) ? rows() : where;
        listbox_base::remove_row_(i);
        rows_.erase(index_to_iterator(i, rows_));
    }

    void reserve(int const rows) {
        BK_PRECONDITION(rows >= 0);
        listbox_base::reserve(rows);
        rows_.reserve(rows);
    }

    void clear() {
        listbox_base::clear();
        rows_.clear();
    }
private:
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

    static constexpr int const list_w = 400;
    static constexpr int const list_h = 200;

    auto trender_ptr = bkrl::make_text_renderer();
    auto& trender = *trender_ptr;

    bkrl::listbox<bklib::utf8_string> list {list_w, list_h, [&](int const i, bklib::utf8_string const& s) {
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

    auto const make_list = [&] {
        for (auto const& row : row_data) {
            list.append_row(row.to_string());
        }
    };

    SECTION("sanity") {
        REQUIRE(list.rows() == 0);
        REQUIRE(list.cols() == 0);
        REQUIRE(list.cols_total() == 3);

        REQUIRE(list.is_visible() == false);
        REQUIRE(list.width()  == list_w);
        REQUIRE(list.height() == list_h);
        REQUIRE(list.current_selection() == 0);
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

    SECTION("layout") {
        make_list();
        list.update_layout(trender);
    }
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
