#include "inventory.hpp"
#include "renderer.hpp"
#include "text.hpp"
#include "input.hpp"
#include "commands.hpp"
#include "item.hpp"
#include "context.hpp"

#include "bklib/scope_guard.hpp"
#include "bklib/assert.hpp"
#include "bklib/string.hpp"

namespace bkrl { class inventory_impl; }

class bkrl::inventory_impl final : public inventory {
public:
    bklib::irect default_bounds() const noexcept {
        return {200, 200, 600, 450};
    }

    struct row_data_t {
        bkrl::text_layout shortcut; // TODO wasteful
        bkrl::text_layout symbol; // TODO wasteful
        bkrl::text_layout name;
        data_t            data;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // inventory interface
    ////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~inventory_impl();

    explicit inventory_impl(text_renderer& trender)
      : layout_ {default_bounds()}
      , text_renderer_ {trender}
      , title_ {trender, ""}
    {
        handler_ = [](action, int) noexcept {};
    }

    void insert(row_t&& r, int where) final override;

    void remove(int) final override {
        BK_ASSERT(false && "TODO");
    }

    void clear() final override {
        rows_.clear();
        title_.clear();
        selection_ = selection_none;
        scroll_offset_ = 0;
        is_moving_ = false;
        is_scrolling_ = false;
    }

    void set_title(bklib::utf8_string_view const title) final override {
        title_.set_text(text_renderer_, title);
        title_.clip_to(
            bklib::clamp_to<text_layout::size_type>(layout_.title.w)
          , bklib::clamp_to<text_layout::size_type>(layout_.title.h));
    }

    void draw(renderer& render) final override;

    int count() const noexcept final override {
        return static_cast<int>(rows_.size());
    }

    void show(bool const visible) noexcept  final override;

    bool is_visible() const noexcept final override{
        return visible_;
    }

    void resize(bklib::irect) final override {
        BK_ASSERT(false && "TODO");
    }

    bklib::irect bounds() const final override {
        return layout_.bounds();
    }

    bklib::irect client_area() const final override {
        auto const& c = layout_.client;
        return {c.x, c.y, c.x + c.w, c.y + c.h};
    }

    int row_height() const final override {
        return text_renderer_.line_spacing() + layout_t::padding;
    }

    void move_by(bklib::ivec2 const v) final override {
        layout_.move_by(v);
    }

    void move_to(bklib::ipoint2 const p) final override {
        layout_.move_to(p);
    }

    bklib::ipoint2 position() final override {
        return layout_.position();
    }

    int selection() const noexcept final override {
        return selection_;
    }

    data_t data(int const index) const final override {
        auto i = index;
        if (index == selection_current) {
            i = selection();
            BK_PRECONDITION(i != selection_none);
        }

        BK_PRECONDITION(i >= 0);

        return rows_[static_cast<size_t>(i)].data;
    }

    bool on_mouse_move(mouse_state const& m) final override;
    bool on_mouse_button(mouse_button_state const& m) final override;
    bool on_mouse_scroll(mouse_state const& m) final override;
    command_handler_result on_command(bkrl::command cmd) final override;

    void set_on_action(action_handler_t handler) final override {
        handler_ = std::move(handler);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////

    void select(int const i, bool const send_event = true) {
        auto const n = static_cast<int>(rows_.size());
        if (n == 0) {
            selection_ = selection_none;
            return;
        }

        selection_ = (i % n) + (i < 0 ? n : 0);

        if (send_event) {
            handler_(action::select, selection_);
        }
    }

    void select_next(int const n = 1) {
        select(selection_ + n);
    }

    void select_prev(int const n = 1) {
        select(selection_ + -n);
    }

    void do_equip() {
        BK_ASSERT(selection_ < count());
        handler_(action::equip, selection_);
    }

    void do_confirm() {
        BK_ASSERT(selection_ < count());
        handler_(action::confirm, selection_);
    }

    void do_cancel() {
        BK_ASSERT(selection_ < count());
        handler_(action::cancel, selection_);
    }

    int position_to_index(bklib::ipoint2 const p) const noexcept {
        auto const y0 = bklib::clamp_min(
            y(p) - (layout_.client.y + scroll_offset()), 0
        );

        auto const h = text_renderer_.line_spacing();

        auto const row  = y0 / (h + 2);
        auto const rows = count();

        return (rows && row >= rows) ? -1 : row;
    }

    bool should_handle_mouse_input(bklib::ipoint2 const p) const noexcept {
        return is_visible() && (is_moving_ || is_scrolling_ || intersects(p, layout_.bounds()));
    }

    void scroll_items(int const dy) {
        if (!dy) {
            return;
        }

        auto const top = layout_.scroll_bar.y + layout_t::padding;
        auto const bot = layout_.scroll_bar.y + layout_.scroll_bar.h - layout_t::padding;
        auto const box_top = layout_.scroll_box.y;
        auto const box_bot = layout_.scroll_box.y + layout_.scroll_box.h;

        layout_.scroll_box.y += (dy < 0)
            ? (std::max(box_top + dy, top) - box_top)
            : (std::min(box_bot + dy, bot) - box_bot);

        scroll_offset_ = bklib::clamp_to<text_renderer::size_type>(top - layout_.scroll_box.y);
    }

    int scroll_offset() const noexcept {
        return bklib::ceil_to<int>(layout_.scroll_factor_ * scroll_offset_);
    }
private:
    static constexpr auto const selection_none = -1;

    struct layout_t {
        using size_type = bkrl::text_layout::size_type;

        static constexpr size_type const padding = 2;
        static constexpr size_type const w_scroll_bar = 18;
        static constexpr size_type const w_scroll_box = w_scroll_bar - 4 - 4;
        static constexpr size_type const h_scroll_box_min = 18;
        static constexpr size_type const h_status_bar = 20;
        static constexpr size_type const h_title_bar  = 20;

        layout_t() noexcept = default;
        explicit layout_t(bklib::irect const r) noexcept { resize(r, 0, 0); }

        void resize(bklib::irect r, size_type client_w, size_type client_h) noexcept;
        void move_to(bklib::ipoint2 const p) noexcept { move_by(p - position()); }
        void move_by(bklib::ivec2 v) noexcept;

        bklib::ipoint2 position() const noexcept { return {background.x, background.y}; }
        bklib::irect bounds() const noexcept {
            return {background.x
                  , background.y
                  , background.x + background.w
                  , background.y + background.h
                };
        }

        double scroll_factor_ = 1.0;

        renderer::rect_t title      {};
        renderer::rect_t client     {};
        renderer::rect_t background {};
        renderer::rect_t scroll_bar {};
        renderer::rect_t scroll_box {};
        renderer::rect_t status     {};
    } layout_;

    action_handler_t   handler_;
    text_renderer&     text_renderer_;
    int                selection_ = selection_none;
    std::vector<row_data_t> rows_;
    text_layout        title_;
    text_renderer::size_type scroll_offset_ = 0;
    bool               visible_ = false;
    bool               is_moving_ = false;
    bool               is_scrolling_ = false;
};

bkrl::inventory_impl::~inventory_impl() {
}

bkrl::inventory::~inventory() {
}

//===----------------------------------------------------------------------------------------===
//===                             inventory_impl                                             ===
//===----------------------------------------------------------------------------------------===

//----------------------------------------------------------------------------------------------
void bkrl::inventory_impl::layout_t::resize(
    bklib::irect const r
  , size_type const // client_w // required client width to show everything
  , size_type const client_h // required client height to show everything
) noexcept {
    auto const x = r.left;
    auto const y = r.top;
    auto const w = r.width();
    auto const h = r.height();

    background.x = x;
    background.y = y;
    background.w = w;
    background.h = h;

    title.x = x;
    title.y = y;
    title.w = w;
    title.h = h_title_bar;

    status.x = x;
    status.y = (background.y + background.h) - h_status_bar;
    status.w = w;
    status.h = h_status_bar;

    auto const client_h_available = h - title.h - status.h - padding - padding;
    auto const h_delta = client_h_available - client_h;

    if (h_delta > 0) {
        scroll_bar = renderer::rect_t {};
        scroll_box = renderer::rect_t {};
        scroll_factor_ = 1.0;
    } else {
        scroll_bar.x = (background.x + background.w) - w_scroll_bar;
        scroll_bar.y = title.y + title.h;
        scroll_bar.w = w_scroll_bar;
        scroll_bar.h = h - h_status_bar - title.h;

        auto const total_h = scroll_bar.h - padding * 2;
        auto const bar_h = total_h + h_delta;

        scroll_box.x = scroll_bar.x + (scroll_bar.w - w_scroll_box) / 2;
        scroll_box.y = scroll_bar.y + padding;
        scroll_box.w = w_scroll_box;

        if (bar_h < h_scroll_box_min) {
            scroll_factor_ = 1.0 + (h_scroll_box_min - bar_h) / static_cast<double>(total_h);
            scroll_box.h = h_scroll_box_min;
        } else {
            scroll_box.h = bar_h;
        }
    }

    auto const client_w_available = w - scroll_bar.w - padding - padding;

    client.x = x + padding;
    client.y = (title.y + title.h) + padding;
    client.w = client_w_available;
    client.h = client_h_available;
}

//----------------------------------------------------------------------------------------------
void bkrl::inventory_impl::layout_t::move_by(bklib::ivec2 const v) noexcept
{
    auto const dx = x(v);
    auto const dy = y(v);

    auto const do_move = [dx, dy](renderer::rect_t& r) noexcept {
        r.x += dx;
        r.y += dy;
    };

    do_move(title);
    do_move(client);
    do_move(background);
    do_move(scroll_bar);
    do_move(scroll_box);
    do_move(status);
}

//----------------------------------------------------------------------------------------------
void bkrl::inventory_impl::insert(inventory::row_t&& r, int const where)
{
    auto const n = count();
    auto const c = bklib::alphanum_id::to_char(n);

    char const shortcut[2] = {c > 0 ? static_cast<char>(c) : char {0}, 0};

    row_data_t row {
        bkrl::text_layout {text_renderer_, shortcut}
      , bkrl::text_layout {text_renderer_, r.symbol}
      , bkrl::text_layout {text_renderer_, r.name}
      , r.data
    };

    auto const h = text_renderer_.line_spacing();
    auto const max_w = x(text_renderer_.bbox());
    auto const w = static_cast<text_layout::size_type>(bklib::clamp<int>(
        layout_.client.w - (max_w + layout_t::padding + max_w + layout_t::padding)
        , 0
        , std::numeric_limits<text_layout::size_type>::max()
        ));

    row.name.clip_to(w, h);

    if (where == at_end) {
        rows_.push_back(std::move(row));
    } else {
        rows_.insert(next(begin(rows_), where), std::move(row));
    }
}

//----------------------------------------------------------------------------------------------
void bkrl::inventory_impl::draw(renderer& render)
{
    struct color_t {
        bkrl::color4 fg;
        bkrl::color4 bg;
    };

    auto const color_back      = make_color(200, 200, 200, 200);
    auto const color_client_bg = make_color(180, 180, 180, 200);
    auto const color_hilite    = make_color(200, 0, 0, 200);

    auto const color_scroll = color_t {
        make_color(100, 100, 100)
      , make_color(66, 66, 66) };

    auto const color_title = color_t {
        make_color(255, 255, 255)
      , make_color(66, 66, 66) };

    auto const color_status = color_t {
        make_color(255, 255, 255)
      , make_color(66, 66, 66) };

    constexpr auto const padding = layout_t::padding;

    if (!visible_) {
        return;
    }

    auto const& client = layout_.client;

    auto const h  = client.h;
    auto const lh = text_renderer_.line_spacing();

    render.draw_filled_rect(layout_.background, color_back);
    render.draw_filled_rect(layout_.title,      color_title.bg);
    render.draw_filled_rect(layout_.status,     color_status.bg);
    render.draw_filled_rect(layout_.scroll_bar, color_scroll.bg);
    render.draw_filled_rect(layout_.scroll_box, color_scroll.fg);
    render.draw_filled_rect(layout_.client,     color_client_bg);

    title_.draw(render, layout_.title.x, layout_.title.y);

    render.set_clip_region(layout_.client);
    BK_SCOPE_EXIT {
        render.clear_clip_region();
    };

    auto i = 0;
    auto y = client.y + scroll_offset();

    auto const next_row = [&]() noexcept {
        y += lh + padding;
        ++i;
    };

    for (auto const& row : rows_) {
        auto x = client.x;

        if (y + lh < client.y) {
            next_row();
            continue;
        } else if (y - client.y > h) {
            break;
        }

        if (selection_ == i) {
            renderer::rect_t const r {
                client.x
              , y
              , client.w
              , lh
            };

            render.draw_filled_rect(r, color_hilite);
        }

        row.shortcut.draw(render, x, y);
        x += row.symbol.extent().width() + padding;

        row.symbol.draw(render, x, y);
        x += row.symbol.extent().width() + padding;

        row.name.draw(render, x, y);
        x += row.name.extent().width() + padding;

        next_row();
    }
}

//----------------------------------------------------------------------------------------------
void bkrl::inventory_impl::show(bool const visible) noexcept
{
    visible_ = visible;
    if (!visible_) {
        return;
    }

    auto const lh      = text_renderer_.line_spacing();
    auto const h_total = layout_t::padding + count() * (lh + layout_t::padding) + layout_t::padding;

    layout_.resize(
        layout_.bounds()
      , bklib::clamp_to<text_renderer::size_type>(layout_.client.w)
      , bklib::clamp_to<text_renderer::size_type>(h_total));

    auto const cell_w = x(text_renderer_.bbox());

    auto const clamp = [](auto const n) noexcept -> layout_t::size_type {
        constexpr auto const min = layout_t::size_type {0};
        constexpr auto const max = std::numeric_limits<layout_t::size_type>::max();

        return static_cast<layout_t::size_type>(bklib::clamp<decltype(n)>(n, min, max));
    };

    auto const w_short_cut = clamp(cell_w + layout_t::padding);
    auto const w_type_icon = clamp(cell_w + layout_t::padding);
    auto const w_slack     = clamp(layout_.client.w - w_short_cut - w_type_icon);

    for (auto& row : rows_) {
        row.shortcut.clip_to(w_short_cut, lh);
        row.symbol.clip_to(w_type_icon, lh);
        row.name.clip_to(w_slack, lh);
    }
}

namespace {

template <typename T>
inline bool intersects(T const lhs, bkrl::renderer::rect_t const rhs) noexcept {
    auto const r = bklib::irect {rhs.x, rhs.y, rhs.x + rhs.w, rhs.y + rhs.h};
    return bklib::intersects(lhs, r);
}

template <typename T>
inline bool intersects(bkrl::renderer::rect_t const lhs, T const rhs) noexcept {
    return intersects(rhs, lhs);
}

}

//----------------------------------------------------------------------------------------------
bool bkrl::inventory_impl::on_mouse_move(mouse_state const& m)
{
    auto const p = bklib::ipoint2 {m.x, m.y};
    if (!should_handle_mouse_input(p)) {
        return false;
    }

    if (is_moving_) {
        move_by(bklib::ivec2 {m.dx, m.dy});
        return true;
    }

    if (is_scrolling_) {
        scroll_items(m.dy);
        return true;
    }

    if (m.state) {
        return false;
    }

    if (!intersects(p, layout_.client)) {
        return true;
    }

    auto const i = position_to_index(p);
    if (i < 0) {
        return true;
    }

    auto const n = count();
    select(i - (i >= n ? 1 : 0));

    return true;
}

//----------------------------------------------------------------------------------------------
bool bkrl::inventory_impl::on_mouse_button(mouse_button_state const& m)
{
    auto const p = bklib::ipoint2 {m.x, m.y};
    if (!should_handle_mouse_input(p)) {
        return false;
    }

    if (m.was_released()) {
        if (is_moving_) {
            is_moving_ = false;
        } else if (is_scrolling_) {
            is_scrolling_ = false;
        }
    }

    if (!m.was_pressed() || m.button() != bkrl::mouse_button::left) {
        return true;
    }

    if (intersects(p, layout_.title)) {
        is_moving_ = true;
        return true;
    }

    if (intersects(p, layout_.scroll_box)) {
        is_scrolling_ = true;
        return true;
    }

    if (!intersects(p, layout_.client)) {
        return true;
    }

    auto const i = position_to_index(p);
    if (i < 0) {
        return true;
    }

    auto const n = count();
    select(i - (i >= n ? 1 : 0), false); // no event

    do_confirm();

    return true;
}

//----------------------------------------------------------------------------------------------
bool bkrl::inventory_impl::on_mouse_scroll(mouse_state const& m) {
    auto const p = bklib::ipoint2 {m.x, m.y};
    if (!should_handle_mouse_input(p)) {
        return false;
    }

    if (!intersects(p, layout_.client)
     && !intersects(p, layout_.scroll_box)
    ) {
        return false;
    }

    auto const lh = text_renderer_.line_spacing() + layout_t::padding;
    scroll_items(m.sy < 0 ? lh : -lh);

    return true;
}

//----------------------------------------------------------------------------------------------
bkrl::command_handler_result bkrl::inventory_impl::on_command(bkrl::command const cmd)
{
    auto const default_result = command_handler_result::capture;

    if (cmd.type == command_type::raw) {
        auto const raw = get_command_data<command_type::raw>(cmd);
        if (raw.virtual_key == 'e' && raw.mods.test(key_mod::ctrl)) {
            do_equip();
            return command_handler_result::filter;
        }
    } else if (cmd.type == command_type::text) {
        auto const str = get_command_data<command_type::text>(cmd);

        if (str.size() > 1) {
            return default_result;
        }

        auto const c = str.front();
        auto const i = bklib::alphanum_id::to_index(c);
        if (i < 0) {
            return default_result;
        }

        auto const n = static_cast<int>(rows_.size());
        if (i >= n) {
            return default_result;
        }

        select(i);
    } else if (cmd.type == command_type::dir_south) {
        select_next();
    } else if (cmd.type == command_type::dir_north) {
        select_prev();
    } else if (cmd.type == command_type::confirm) {
        do_confirm();
    } else if (cmd.type == command_type::cancel) {
        do_cancel();
    }

    return default_result;
}

//--------------------------------------------------------------------------------------------------
bkrl::inventory& bkrl::populate_item_list(
    context& ctx
  , inventory& i
  , item_pile& pile
  , bklib::utf8_string_view const title
) {
    i.clear();
    i.set_title(title);

    item::format_flags flags;
    flags.use_color = true;

    auto const last = std::end(pile);
    for (auto it = std::begin(pile); it != last; ++it) {
        item     const& itm  = *it;
        item_def const* idef = ctx.data.find(itm.def());

        flags.use_definition = idef;

        if (itm.flags().test(item_flag::is_equipped)) {
            flags.override_color = "r";
        } else {
            flags.override_color.clear();
        }

        i.insert(inventory::row_t {
            idef ? idef->symbol : " " //TODO find will be called twice
          , itm.friendly_name(ctx, flags)
          , it
        });
    }

    i.show(true);
    return i;
}

std::unique_ptr<bkrl::inventory> bkrl::make_item_list(text_renderer& trender)
{
    return std::make_unique<inventory_impl>(trender);
}
