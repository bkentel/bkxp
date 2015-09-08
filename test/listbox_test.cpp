#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

//#include "listbox.hpp"

#include "text.hpp"

#include "bklib/math.hpp"
#include "bklib/assert.hpp"
#include "bklib/string.hpp"

namespace bkrl {

template <typename Iterator>
auto slide(Iterator f, Iterator l, Iterator p)
    -> std::pair<Iterator, Iterator>
{
    if (p < f) { return {p, std::rotate(p, f, l)}; }
    if (l < p) { return {std::rotate(f, l, p), p}; }
    return {f, l};
}

template <typename SrcIt, typename DstIt, typename Predicate, typename Transform>
DstIt transform_if(SrcIt first, SrcIt last, DstIt out, Predicate pred, Transform trans)
{
    for (auto it = first; it != last; ++it) {
        if (!pred(*it)) { continue; }
        out++ = trans(*it);
    }

    return out;
}

template <typename Container, typename SrcIt, typename DstIt, typename Predicate, typename Transform>
auto transform_if(
    Container& c
  , SrcIt first, SrcIt last, DstIt where
  , Predicate pred
  , Transform trans
  , std::random_access_iterator_tag
) -> std::pair<DstIt, DstIt>
{
    using std::begin;
    using std::end;

    auto const pos = std::distance(begin(c), where);
    auto const n   = c.size();

    transform_if(first, last, std::back_inserter(c), pred, trans);

    return slide(
        std::next(begin(c), n)
      , end(c)
      , std::next(begin(c), pos));
}

template <typename Container, typename SrcIt, typename DstIt, typename Predicate, typename Transform>
auto transform_if(
    Container& c
  , SrcIt first, SrcIt last, DstIt where
  , Predicate pred
  , Transform trans
) -> std::pair<DstIt, DstIt>
{
    return transform_if(c, first, last, where, pred, trans
      , typename std::iterator_traits<DstIt>::iterator_category {});
}

template <typename Container, typename SrcIt, typename DstIt, typename Transform>
auto transform_insert(
    Container& c
  , SrcIt first, SrcIt last, DstIt where
  , Transform trans
  , std::random_access_iterator_tag
) -> std::pair<DstIt, DstIt>
{
    using std::begin;
    using std::end;

    auto const pos = std::distance(begin(c), where);
    auto const n   = c.size();

    std::transform(first, last, std::back_inserter(c), trans);

    return slide(
        std::next(begin(c), n)
      , end(c)
      , std::next(begin(c), pos));
}

template <typename Container, typename SrcIt, typename DstIt, typename Transform>
auto transform_insert(
    Container& c
  , SrcIt first, SrcIt last, DstIt where
  , Transform trans
) -> std::pair<DstIt, DstIt>
{
    return transform_insert(c, first, last, where, trans
      , typename std::iterator_traits<DstIt>::iterator_category {});
}

class listview_base {
public:
    static constexpr int const at_end = -1;

    using rect_t   = bklib::irect;
    using string_t = bklib::utf8_string;

    struct label_t {
        label_t() = default;
        label_t(string_t str, bkrl::text_renderer& trender)
          : layout {trender, str}
          , text {std::move(str)}
        {
        }

        bkrl::text_layout layout;
        string_t          text;
    };

    struct row_t {
        row_t() = default;
        row_t(string_t str, bkrl::text_renderer& trender)
          : label {std::move(str), trender}
          , height {label.layout.extent().height()}
        {
        }

        label_t label;

        int32_t top    = 0;
        int32_t min_h  = 0;
        int32_t max_h  = 0;
        int32_t height = 0;

        int32_t bottom() const noexcept { return top + height; }
    };

    struct col_t {
        col_t() = default;
        col_t(string_t str, bkrl::text_renderer& trender)
          : label {std::move(str), trender}
          , width {label.layout.extent().width()}
        {
        }

        label_t label;

        int32_t left  = 0;
        int32_t min_w = 0;
        int32_t max_w = 0;
        int32_t width = 0;

        int32_t right() const noexcept { return left + width; }
    };

    struct cell_t {
        cell_t() = default;
        cell_t(string_t str, bkrl::text_renderer& trender)
          : label {std::move(str), trender}
        {
        }

        label_t label;
    };
public:
    template <typename C>
    static inline decltype(auto) checked_at_index(C&& c, int const i) noexcept {
        BK_PRECONDITION(i >= 0 && i < static_cast<int>(c.size()));
        return c[i];
    }

    template <typename C>
    static decltype(auto) index_to_iterator(C&& c, int const i) noexcept {
        auto const size = static_cast<int>(c.size());

        BK_PRECONDITION((i == -1 || i >= 0) && i < size);

        using std::end;
        using std::begin;

        return (i == -1 || i == size)
          ? end(c)
          : std::next(begin(c), i);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    listview_base(int32_t const x, int32_t const y, int32_t const w, int32_t const h
      , bkrl::text_renderer& trender
    )
      : bounds_ {bklib::make_rect(x, y, x + w, y + h)}
      , text_renderer_ {&trender}
    {
        rows_.emplace_back();
        cols_.emplace_back();
    }

    void clear() {

    }

    bkrl::text_renderer* set_text_renderer(bkrl::text_renderer& trender) {
        auto const old = text_renderer_;
        text_renderer_ = &trender;
        return old;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    rect_t bounds() const noexcept {
        return bounds_;
    }

    rect_t client_bounds() const noexcept {
        return { scroll_x_ + bounds_.left
               , scroll_y_ + bounds_.top
               , scroll_x_ + bounds_.right
               , scroll_y_ + bounds_.bottom };
    }

    rect_t row_bounds(int const row) const noexcept {
        auto const& r = checked_at_index(rows_, row);
        return { scroll_x_ + bounds_.left
               , scroll_y_ + r.top
               , scroll_x_ + bounds_.right
               , scroll_y_ + r.top + r.height };
    }

    rect_t col_bounds(int const col) const noexcept {
        auto const& c = checked_at_index(cols_, col);
        return { scroll_x_ + c.left
               , scroll_y_ + bounds_.top
               , scroll_x_ + c.left + c.width
               , scroll_y_ + bounds_.bottom };
    }

    rect_t cell_bounds(int const row, int const col) const noexcept {
        auto const& r = checked_at_index(rows_, row);
        auto const& c = checked_at_index(cols_, col);
        return { scroll_x_ + c.left
               , scroll_y_ + r.top
               , scroll_x_ + c.left + c.width
               , scroll_y_ + r.top + r.height };
    }

    struct hit_test_t {
        int  row;
        int  col;
        bool is_hit;
    };

    hit_test_t hit_test(bklib::ipoint2 const p) const noexcept {
        hit_test_t result {-1, -1, intersects(p, client_bounds())};
        if (!result.is_hit) {
            return result;
        }

        result.col = std::count_if(begin(cols_), end(cols_), [xp = x(p)](col_t const& c) noexcept {
            return xp < c.left;
        });

        result.row = std::count_if(begin(rows_), end(rows_), [yp = y(p)](row_t const& r) noexcept {
            return yp < r.top;
        });

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    int rows() const noexcept { return static_cast<int>(rows_.size()); }
    int cols() const noexcept { return static_cast<int>(cols_.size()); }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename Query>
    cell_t generate_cell_at_(Query query, text_renderer& trender, int const r, int const c) {
        cell_t result {query(r, c), trender};

        auto const extent = result.label.layout.extent();
        cols_[c + 1].width  = std::max(cols_[c + 1].width,  extent.width());
        rows_[r + 1].height = std::max(rows_[r + 1].height, extent.height());

        return result;
    }

    template <typename Iterator, typename GetLabel, typename Query>
    void insert_row(
        Iterator first, Iterator last
      , GetLabel label, Query query
      , int const n, int const where = at_end
    ) {
        auto&      trender = *text_renderer_;
        auto const n_cols  = cols() - 1;
        auto       r       = rows() - 1;

        BK_PRECONDITION(n      >= 0);
        BK_PRECONDITION(n_cols >= 0);
        BK_PRECONDITION(r      >= 0);

        rows_.reserve(static_cast<size_t>(rows_.size() + n));
        transform_insert(rows_, first, last, end(rows_), [&](auto const& data) {
            return row_t {label(data), trender};
        });

        auto const first_row = cells_.insert(index_to_iterator(cells_, where), n
          , decltype(cells_)::value_type {});

        auto const last_row = std::next(first_row, n);

        for (auto it = first_row; it != last_row; ++it) {
            auto& row = *it;

            row.reserve(static_cast<size_t>(row.size() + n_cols));
            std::generate_n(back_inserter(row), n_cols, [&, r, c = 0]() mutable {
                return generate_cell_at_(query, trender, r, c++);
            });

            ++r;
        }

        layout_rows_(where, n);
        layout_cols_(where, n);
    }

    template <typename Iterator, typename GetLabel, typename Query>
    void insert_col(
        Iterator first, Iterator last
      , GetLabel label, Query query
      , int const n, int const where = at_end
    ) {
        BK_PRECONDITION(n >= 0);

        auto& trender = *text_renderer_;

        cols_.reserve(static_cast<size_t>(cols_.size() + n));
        transform_insert(cols_, first, last, end(cols_), [&](auto const& data) {
            return col_t {label(data), trender};
        });

        auto const n_cols = cols() - 1;
        auto const c0     = (where == at_end) ? n_cols : where;
        int        r       = 0;

        BK_PRECONDITION(n_cols >= 0);

        for (auto& row : cells_) {
            row.reserve(static_cast<size_t>(row.size() + n_cols));
            std::generate_n(back_inserter(row), n, [&, r, c = c0]() mutable {
                return generate_cell_at_(query, trender, r, c++);
            });

            slide(next(begin(row), n_cols), end(row), index_to_iterator(row, where));
            ++r;
        }

        layout_rows_(where, n);
        layout_cols_(where, n);
    }

    void erase_row(int r);
    void erase_col(int c);

    void insert(string_t text);

    cell_t const& at(int const r, int const c) const {
        BK_PRECONDITION(r + 1 < static_cast<int>(rows_.size()));
        BK_PRECONDITION(c + 1 < static_cast<int>(cols_.size()));

        return cells_[r][c];
    }
private:
    void layout_rows_(int const where, int const n) {
        int32_t y = 0;
        int32_t h_total = 0;

        for (auto& row : rows_) {
            row.top = y;
            y += row.height;
            h_total += row.height;
        }

        content_height_ = h_total;
        scroll_y_max_ = bklib::clamp_min(client_bounds().height() - bounds().height(), 0);
    }

    void layout_cols_(int const where, int const n) {
        int32_t x = 0;
        int32_t w_total = 0;

        for (auto& col : cols_) {
            col.left = x;
            x += col.width;
            w_total += col.width;
        }

        content_width_ = w_total;
        scroll_x_max_ = bklib::clamp_min(client_bounds().width() - bounds().width(), 0);
    }

    rect_t bounds_;

    bkrl::text_renderer* text_renderer_ = nullptr;

    int32_t content_width_  = 0;
    int32_t content_height_ = 0;

    int32_t scroll_x_     = 0;
    int32_t scroll_x_max_ = 0;
    int32_t scroll_y_     = 0;
    int32_t scroll_y_max_ = 0;

    std::vector<row_t> rows_;
    std::vector<col_t> cols_;

    std::vector<std::vector<cell_t>> cells_;
};

template <typename RowData, typename ColData>
class listview : public listview_base {
    static_assert(std::is_nothrow_move_assignable<RowData>::value, "");
    static_assert(std::is_nothrow_move_assignable<ColData>::value, "");
public:
    using query_t = std::function<string_t (RowData const&, ColData const&)>;

    listview(int32_t const x, int32_t const y, int32_t const w, int32_t const h
      , bkrl::text_renderer& trender, query_t query
    )
      : listview_base {x, y, w, h, trender}, query_ {std::move(query)}
    {
    }

    //! @tparam Iterator  forward_iterator
    //! @tparam Predicate function(Iterator::value_type) -> bool
    //! @tparam GetLabel  function(ColData)              -> string_t
    //! @tparam GetData   function(Iterator::value_type) -> RowData
    template <typename Iterator, typename Predicate, typename GetLabel, typename GetData>
    void insert_row(
        Iterator first, Iterator last
      , Predicate pred, GetLabel label, GetData data
      , int const where = at_end
    ) {
        auto const size = row_data_.size();

        auto const result = transform_if(
            row_data_, first, last, index_to_iterator(row_data_, where), pred, data);

        listview_base::insert_row(
            result.first, result.second
          , label
          , [this](int const r, int const c) { return query_(row_data_[r], col_data_[c]); }
          , static_cast<int>(row_data_.size() - size)
          , where);
    }

    //! @tparam Iterator  forward_iterator
    //! @tparam Predicate function(Iterator::value_type) -> bool
    //! @tparam GetLabel  function(ColData)              -> string_t
    //! @tparam GetData   function(Iterator::value_type) -> ColData
    template <typename Iterator, typename Predicate, typename GetLabel, typename GetData>
    void insert_col(
        Iterator first, Iterator last
      , Predicate pred, GetLabel label, GetData data
      , int const where = at_end
    ) {
        auto const size = col_data_.size();

        auto const result = transform_if(
            col_data_, first, last, index_to_iterator(col_data_, where), pred, data);

        listview_base::insert_col(
            result.first, result.second
          , label
          , [this](int const r, int const c) { return query_(row_data_[r], col_data_[c]); }
          , static_cast<int>(col_data_.size() - size)
          , where);
    }

    std::pair<RowData const&, ColData const&>
    data(int const r, int const c) const {
        BK_PRECONDITION(r < static_cast<int>(row_data_.size()));
        BK_PRECONDITION(c < static_cast<int>(col_data_.size()));

        return {row_data_[r], col_data_[c]};
    }
private:
    query_t query_;
    std::vector<RowData> row_data_;
    std::vector<ColData> col_data_;
};

} //namespace bkrl

TEST_CASE("listview") {
    auto text_renderer = bkrl::make_text_renderer();
    auto& trender = *text_renderer;

    constexpr int32_t const list_x = 50;
    constexpr int32_t const list_y = 100;
    constexpr int32_t const list_w = 400;
    constexpr int32_t const list_h = 200;

    struct data_t {
        float       value;
        std::string text;
    };

    enum class col_t {
        value, text
    };

    std::array<data_t, 5> const data {
        data_t {1.0f, "one"}
      , data_t {2.1f, "two"}
      , data_t {3.2f, "three"}
      , data_t {4.3f, "four"}
      , data_t {5.4f, "five"}
    };

    std::array<col_t, 2> const col_header {
        col_t::text, col_t::value
    };

    bkrl::listview<data_t const*, col_t> list {list_x, list_y, list_w, list_h, trender
      , [](data_t const* const rdata, col_t const cdata) {
            switch (cdata) {
            case col_t::value: return std::to_string(rdata->value);
            case col_t::text:  return rdata->text;
            default:
                break;
            }

            BK_UNREACHABLE;
        }};

    auto const make_list = [&] {
        list.insert_col(begin(col_header), end(col_header)
          , [](col_t const)   { return true; }
          , [](col_t const c) { return c == col_t::text ? "text" : "value"; }
          , [](col_t const c) { return c; }
        );

        list.insert_row(begin(data), end(data)
          , [](auto const&)     { return true; }
          , [](auto const&)     { return ""; }
          , [](data_t const& d) { return &d; }
        );
    };

    SECTION("initial state") {
        REQUIRE(list.cols() == 1);
        REQUIRE(list.rows() == 1);

        auto const b = list.bounds();
        REQUIRE(b.left     == list_x);
        REQUIRE(b.top      == list_y);
        REQUIRE(b.width()  == list_w);
        REQUIRE(b.height() == list_h);

        auto const cb = list.client_bounds();
        REQUIRE(b == cb);
    }

    SECTION("simple") {
        make_list();

        auto const rows = static_cast<int>(data.size());
        auto const cols = static_cast<int>(col_header.size());

        REQUIRE(list.rows() == rows + 1);
        REQUIRE(list.cols() == cols + 1) ;

        for (auto r = 0; r < rows; ++r) {
            for (auto c = 0; c < cols; ++c) {
                auto const p = list.data(r, c);
                REQUIRE(p.first  == &data[r]);
                REQUIRE(p.second == col_header[c]);

                auto const& text = list.at(r, c).label.text;

                switch (col_header[c]) {
                case col_t::text:  REQUIRE(text == data[r].text); break;
                case col_t::value: REQUIRE(text == std::to_string(data[r].value)); break;
                default:           REQUIRE(false); break;
                }
            }
        }
    }

}

//TEST_CASE("listbox") {
//    static constexpr bklib::utf8_string_view const row_data[] = {
//        bklib::make_string_view("row 0")
//      , bklib::make_string_view("row 1")
//      , bklib::make_string_view("row 2")
//    };
//
//    static constexpr int const list_w = 400;
//    static constexpr int const list_h = 200;
//
//    auto trender_ptr = bkrl::make_text_renderer();
//    auto& trender = *trender_ptr;
//
//    auto const line_h = trender.line_spacing();
//
//    bkrl::listbox<bklib::utf8_string> list {0, 0, list_w, list_h, [&](int const i, bklib::utf8_string const& s) {
//        return s + " col " + std::to_string(i);
//    }};
//
//    auto const make_list = [&] {
//        list.clear();
//
//        list.append_col("Col 0");
//        list.append_col("Col 1");
//        list.append_col("Col 2");
//
//        for (auto const& row : row_data) {
//            list.append_row(row.to_string());
//        }
//    };
//
//    SECTION("sanity") {
//        REQUIRE(list.rows()       == 0);
//        REQUIRE(list.cols()       == 0);
//
//        REQUIRE(list.is_visible() == false);
//
//        REQUIRE(list.width()  == list_w);
//        REQUIRE(list.height() == list_h);
//
//        REQUIRE(list.content_width()  == list_w);
//        REQUIRE(list.content_height() == list_h);
//
//        REQUIRE(list.scroll_width()  == 0);
//        REQUIRE(list.scroll_height() == 0);
//
//        REQUIRE(list.current_selection() == 0);
//
//        REQUIRE(list.col_left(0) == 0);
//    }
//
//    SECTION("select") {
//        make_list();
//        REQUIRE(list.current_selection() == 0);
//
//        list.select_next(3);
//        REQUIRE(list.current_selection() == 0);
//
//        list.select_next(-3);
//        REQUIRE(list.current_selection() == 0);
//
//        list.select_next(1);
//        REQUIRE(list.current_selection() == 1);
//
//        list.select_next(-2);
//        REQUIRE(list.current_selection() == 2);
//    }
//
//    SECTION("append") {
//        REQUIRE(list.rows() == 0);
//
//        make_list();
//
//        REQUIRE(list.rows() == 3);
//
//        REQUIRE(list.row_data(0) == row_data[0]);
//        REQUIRE(list.row_data(1) == row_data[1]);
//        REQUIRE(list.row_data(2) == row_data[2]);
//    }
//
//    SECTION("insert") {
//        REQUIRE(list.rows() == 0);
//
//        list.append_row(row_data[1].to_string());
//        list.insert_row(0, row_data[0].to_string());
//        list.insert_row(-1, row_data[2].to_string());
//
//        REQUIRE(list.rows() == 3);
//
//        REQUIRE(list.row_data(0) == row_data[0]);
//        REQUIRE(list.row_data(1) == row_data[1]);
//        REQUIRE(list.row_data(2) == row_data[2]);
//    }
//
//    SECTION("remove") {
//        make_list();
//        list.remove_row(0);
//        list.remove_row(-1);
//
//        REQUIRE(list.rows() == 1);
//        REQUIRE(list.row_data(0) == row_data[1]);
//    }
//
//    SECTION("hit test") {
//        make_list();
//        list.update_layout(trender);
//
//        auto const check_hit_test = [&](bklib::ipoint2 const p, int const row, int const col, bool const ok) {
//            auto const result = list.hit_test(p);
//            REQUIRE(result.row == row);
//            REQUIRE(result.col == col);
//            REQUIRE(result.ok  == ok);
//        };
//
//        auto const c0 = list.col_left(0);
//        auto const c1 = list.col_left(1);
//        auto const c2 = list.col_left(2);
//
//        // valid
//        check_hit_test(bklib::ipoint2 {c0, line_h * 0}, 0, 0, true);
//        check_hit_test(bklib::ipoint2 {c0, line_h * 1}, 1, 0, true);
//        check_hit_test(bklib::ipoint2 {c0, line_h * 2}, 2, 0, true);
//
//        check_hit_test(bklib::ipoint2 {c1, line_h * 0}, 0, 1, true);
//        check_hit_test(bklib::ipoint2 {c1, line_h * 1}, 1, 1, true);
//        check_hit_test(bklib::ipoint2 {c1, line_h * 2}, 2, 1, true);
//
//        check_hit_test(bklib::ipoint2 {c2, line_h * 0}, 0, 2, true);
//        check_hit_test(bklib::ipoint2 {c2, line_h * 1}, 1, 2, true);
//        check_hit_test(bklib::ipoint2 {c2, line_h * 2}, 2, 2, true);
//
//        //invalid
//        check_hit_test(bklib::ipoint2 {c0 - 1, line_h * 0}, 0, 0, false);
//        check_hit_test(bklib::ipoint2 {c0 - 1, line_h * 1}, 0, 0, false);
//        check_hit_test(bklib::ipoint2 {c0 - 1, line_h * 2}, 0, 0, false);
//
//    }
//}

#endif // BK_NO_UNIT_TESTS
