#include "message_log.hpp"

#include "renderer.hpp"
#include "text.hpp"
#include "bklib/assert.hpp"
#include "bklib/scope_guard.hpp"

#include <deque>

// TODO dump lines to a file as well

namespace bkrl { class message_log_impl; }

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::detail::message_log_impl
////////////////////////////////////////////////////////////////////////////////////////////////////
class bkrl::message_log_impl final : public message_log {
public:
    virtual ~message_log_impl();

    explicit message_log_impl(text_renderer& text_render)
      : text_renderer_ {text_render}
    {
    }

    void println(bklib::utf8_string msg) final override;

    void println(bklib::utf8_string_view const msg) final override {
        println(msg.to_string());
    }

    void draw(renderer& render) final override;

    void set_bounds(bklib::irect bounds) final override {
        bounds_ = bounds;
    }

    void show(message_log::show_type type, int n) final override;

    void set_minimum_lines(int const n) final override {
        min_lines_ = bklib::clamp(n, 0, bounds_.height() / text_renderer_.line_spacing());
    }
private:
    int get_visible_lines_() const noexcept {
        return (visible_lines_ == show_all_lines)
          ? static_cast<int>(records_.size())
          : visible_lines_;
    }

    struct record_t {
        text_layout        text;
        bklib::utf8_string string;
    };

    static constexpr int show_all_lines = -1;

    bklib::irect         bounds_        = bklib::irect {5, 5, 640, 180};
    int                  visible_lines_ = show_all_lines;
    int                  min_lines_     = 1;
    text_renderer&       text_renderer_;
    std::deque<record_t> records_;
};

//--------------------------------------------------------------------------------------------------
bkrl::message_log_impl::~message_log_impl() {
}

//--------------------------------------------------------------------------------------------------
void bkrl::message_log_impl::println(bklib::utf8_string msg)
{
    using type = text_layout::size_type;

    auto const max_w = static_cast<type>(bounds_.width());
    auto const max_h = static_cast<type>(bounds_.height());

    text_layout text {text_renderer_, msg, 0, 0, max_w, max_h};

    records_.push_front(record_t {
        std::move(text), std::move(msg)
    });

    auto const lines = text.extent().height() / text_renderer_.line_spacing();
    show(bkrl::message_log::show_type::more, lines);
}

//--------------------------------------------------------------------------------------------------
void bkrl::message_log_impl::draw(renderer& render)
{
    auto const old_clip = render.get_clip_region();
    BK_SCOPE_EXIT {
        render.set_clip_region(old_clip);
    };

    render.set_clip_region(make_renderer_rect(bounds_));

    int x = bounds_.left;
    int y = bounds_.top;

    render.draw_filled_rect(make_renderer_rect(bounds_), make_color(100, 100, 100, 220));

    auto const spacing = text_renderer_.line_spacing();

    auto n = get_visible_lines_();
    for (auto const& rec : records_) {
        if (n <= 0) {
            break;
        }

        auto const h = rec.text.extent().height();
        auto const lines = h / spacing;

        rec.text.draw(render, x, y);
        y += h;
        n -= lines;
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::message_log_impl::show(message_log::show_type const type, int const n)
{
    BK_PRECONDITION(n >= 0);

    auto const n0 = bklib::clamp(
        get_visible_lines_() + (type == message_log::show_type::less ? -n : n)
      , min_lines_
      , bounds_.height() / text_renderer_.line_spacing()
    );

    using st = message_log::show_type;
    switch (type) {
    case st::none :
        visible_lines_ = 0;
        break;
    case st::less :
        visible_lines_ = n0;
        break;
    case st::more :
        visible_lines_ = n0;
        break;
    case st::all :
        visible_lines_ = show_all_lines;
        break;
    default :
        BK_ASSERT(false);
        break;
    }
}

//--------------------------------------------------------------------------------------------------
bkrl::message_log::~message_log() {
}

//--------------------------------------------------------------------------------------------------
std::unique_ptr<bkrl::message_log> bkrl::make_message_log(text_renderer& text_render)
{
    return std::make_unique<message_log_impl>(text_render);
}
