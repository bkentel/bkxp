#include "listbox.hpp"

#include "bklib/scope_guard.hpp"

bkrl::listbox_base::listbox_base(int const left, int const top, int const width, int const height)
  : bounds_ {left, top, left + width, top + height}
{
    BK_PRECONDITION(width  >= 0);
    BK_PRECONDITION(height >= 0);
}

void bkrl::listbox_base::update_layout(text_renderer& trender)
{
    if (data_.cells.empty()) {
        return;
    }

    auto const n_rows = static_cast<size_t>(rows());
    auto const n_cols = static_cast<size_t>(cols());
    auto const em     = x(trender.bbox());
    auto const w      = width();
    auto const h      = height();
    auto const line_h = trender.line_spacing();

    //
    // update column width min / max
    //
    for (auto& col : data_.cols) {
        col.width = 0;

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
    }

    //
    // clear old render data
    //
    render_data_.line_h = line_h;
    render_data_.col_headers.resize(n_cols, text_layout {trender, ""});

    render_data_.cells.resize(n_rows);

    for (auto& row : render_data_.cells) {
        for (auto& cell : row) {
            cell.clear();
        }

        row.resize(n_cols, text_layout {trender, ""});
    }

    //
    // update text & actual widths; calculate total height
    //
    auto const fill_row = [n_cols, line_h, this, &trender](std::vector<text_layout>& row, auto const& src) {
        int row_h = 0;

        for (auto c = 0u; c < n_cols; ++c) {
            auto& col  = data_.cols[c];
            auto& cell = row[c];

            auto const max_w = bklib::clamp_to<text_layout::size_type>(col.width_max);
            auto const do_clip = col.width_min > 0 && col.width_max > 0;

            cell.clip_to(do_clip ? max_w : text_layout::unlimited, line_h);
            cell.set_text(trender, src[c].label);

            auto const extent = cell.extent();
            auto const tw = extent.width();
            auto const th = extent.height();

            col.width = std::max(col.width
                , do_clip ? bklib::clamp(tw, col.width_min, col.width_max) : tw);

            row_h = std::max(row_h, th);
        }

        return row_h;
    };

    int total_row_height = fill_row(render_data_.col_headers, data_.cols);
    for (auto r = 0u; r < n_rows; ++r) {
        total_row_height += fill_row(render_data_.cells[r], data_.cells[r]);
    }

    scroll_y_max_ = bklib::clamp_min(total_row_height - height(), 0);

    auto const total_col_width = col_left(cols());
    scroll_x_max_ = bklib::clamp_min(total_col_width - width(), 0);
}

bkrl::listbox_base::hit_test_t
bkrl::listbox_base::hit_test(bklib::ipoint2 const p) const noexcept
{
    auto const n_rows = rows();
    if (!n_rows) {
        return {0, 0, false};
    }

    auto const h = render_data_.cells[0][0].extent().height();
    if (!h) {
        return {0, 0, false};
    }

    auto const x0 = bounds_.left;
    auto const y0 = bounds_.top;

    auto const ok = bklib::intersects(p, bounds_);

    if (!bklib::intersects(p, bklib::make_rect(x0, y0, content_width(), content_height()))) {
        return {0, 0, false};
    }

    auto const x1 = x(p) + scroll_x_ - x0;
    auto const y1 = y(p) + scroll_y_ - y0;

    auto const row = bklib::floor_to<int>(y1 / static_cast<double>(h));

    auto const it = bklib::find_if(data_.cols, [x1, last_x = 0](auto const& col) mutable {
        auto const result = x1 < col.width + last_x;
        last_x += col.width;
        return result;
    });

    if (it == end(data_.cols)) {
        return {row, 0, false};
    }

    auto const col = static_cast<int>(std::distance(begin(data_.cols), it));

    return {row, col, ok};
}

void bkrl::listbox_base::draw(renderer& render)
{
    if (!is_visible()) {
        return;
    }

    auto const clip = render.get_clip_region();
    BK_SCOPE_EXIT {
        render.set_clip_region(clip);
    };

    auto const bounds = make_renderer_rect(bounds_);
    render.set_clip_region(bounds);

    render.draw_filled_rect(bounds, make_color(100, 100, 100, 240));

    auto const n_rows = static_cast<size_t>(rows());
    auto const n_cols = static_cast<size_t>(cols());

    auto const x0 = content_left();
    auto const y0 = content_top();
    auto const lh = render_data_.line_h;

    auto const cm = cell_margin_;

    int x     = x0 + cm.left;
    int y     = y0 + cm.top;
    int row_h = lh;

    for (auto c = 0u; c < n_cols; ++c) {
        auto const& col = render_data_.col_headers[c];
        col.draw(render, x + cm.left, y + cm.top);
        x += data_.cols[c].width + cm.left + cm.right;
        row_h = std::max(row_h, col.extent().height());
    }

    y += row_h + cm.top + cm.bottom;

    if (!n_rows) {
        return;
    }

    for (auto r = 0u; r < n_rows; ++r) {
        row_h = lh;
        x = x0 + cm.left;

        if (r % 2 == 0) {
            render.draw_filled_rect(make_renderer_rect(
                bklib::make_rect(x0, y, x0 + width(), y + lh))
              , make_color(120, 120, 120, 240));
        }

        for (auto c = 0u; c < n_cols; ++c) {
            auto const& col = data_.cols[c];
            auto const& cell = render_data_.cells[r][c];

            cell.draw(render, x + cm.left, y + cm.top);

            auto const ext = cell.extent();

            if (last_mouse_row_ == r && last_mouse_col_ == c) {
                auto const r = bklib::make_rect(x, y, x + ext.width(), y + ext.height());

                render.draw_filled_rect(make_renderer_rect(r)
                  , make_color(255, 120, 120, 240));
            }

            x += col.width + cm.left + cm.right;
            row_h = std::max(row_h, ext.height());
        }

        y += row_h + cm.top + cm.bottom;
    }
}

/////





////