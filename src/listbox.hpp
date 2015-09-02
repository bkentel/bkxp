#pragma once

#include "bklib/assert.hpp"
#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include "bklib/algorithm.hpp"

#include "text.hpp"
#include "renderer.hpp"

#include <vector>
#include <functional>

namespace bkrl {

template <typename C>
auto index_to_iterator(int const where, C&& c)
{
    auto const size = static_cast<int>(c.size());
    BK_PRECONDITION((where == -1 || where >= 0) && where <= size);

    using std::begin;
    using std::end;

    return (where == -1)           ? end(c)
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
        col_headers.clear();
        cells.clear();
    }

    int line_h = 0;

    cells_t              col_headers;
    std::vector<cells_t> cells;
};

class listbox_base {
public:
    enum class event : int {
        on_hover, on_mouse_down, on_mouse_up, on_select, on_confirm, on_cancel
    };

    void insert_col(int const where, bklib::utf8_string label) {
        data_.cols.insert(index_to_iterator(where, data_.cols)
            , listbox_data::col_t {std::move(label)});
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

    int col_width(int const col) const noexcept {
        return index_to_iterator(col, data_.cols)->width;
    }

    int col_left(int const col) const noexcept {
        return std::accumulate(
            begin(data_.cols)
          , std::next(begin(data_.cols), col)
          , bounds_.left
          , [](int const sum, auto const& c) noexcept {
                return sum + c.width;
        });
    }

    int cols() const noexcept {
        return static_cast<int>(data_.cols.size());
    }

    int rows() const noexcept {
        return static_cast<int>(data_.cells.size());
    }

    int width() const noexcept {
        return bounds_.width();
    }

    int height() const noexcept {
        return bounds_.height();
    }

    int content_width() const noexcept {
        return width() + scroll_width();
    }

    int content_height() const noexcept {
        return height() + scroll_height();
    }

    void set_width(int const value) noexcept {
        BK_PRECONDITION(value > 0);
        bounds_.right = bounds_.left + value;
    }

    void set_height(int const value) noexcept {
        BK_PRECONDITION(value > 0);
        bounds_.bottom = bounds_.top + value;
    }

    void update_layout(text_renderer& trender);

    void clear() {
        data_.clear();
        render_data_.clear();
        selection_ = 0;
        scroll_x_ = 0;
        scroll_y_ = 0;
        scroll_x_max_ = 0;
        scroll_y_max_ = 0;
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

    int scroll_width() const noexcept {
        return scroll_x_max_;
    }

    int scroll_height() const noexcept {
        return scroll_y_max_;
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

    hit_test_t hit_test(bklib::ipoint2 const p) const noexcept;

    void draw(renderer& render);
protected:
    listbox_base(int const width, int const height)
      : bounds_ {bklib::make_rect(0, 0, width, height)}
    {
        BK_PRECONDITION(width  >= 0);
        BK_PRECONDITION(height >= 0);
    }

    auto insert_row_(int const where) {
        auto const size = cols();

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

        auto const n_cols = cols();
        for (auto c = 0; c < n_cols; ++c) {
            cells.emplace_back(listbox_data::cell_t {query(c, value)});
        }
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

    listbox(int const width, int const height, query_t query)
      : listbox_base {width, height}
      , query_ {std::move(query)}
    {
    }

    listbox(int const width, int const height)
      : listbox {width, height, [](int, T const&) { return bklib::utf8_string {}; }}
    {
    }

    void set_query_function(query_t query) {
        query_ = std::move(query);
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

    int insert_col(int const where, bklib::utf8_string label) {
        auto const c = (where == -1) ? cols() : where;

        listbox_base::insert_col(where, std::move(label));

        auto const n_rows = static_cast<size_t>(rows());
        for (auto r = 0u; r < n_rows; ++r) {
            data_.cells[r][c].label = query_(c, rows_[r]);
        }

        return c;
    }

    int append_col(bklib::utf8_string label) {
        return insert_col(-1, std::move(label));
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

} //namespace bkrl

