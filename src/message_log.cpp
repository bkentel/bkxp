#include "message_log.hpp"

#include "renderer.hpp"
#include "text.hpp"
#include "bklib/assert.hpp"

#include <deque>

// TODO dump lines to a file as well

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::detail::message_log_impl
////////////////////////////////////////////////////////////////////////////////////////////////////
class bkrl::detail::message_log_impl {
public:
    explicit message_log_impl(text_renderer& text_render);

    void println(bklib::utf8_string&& msg);
    void draw(renderer& render);

    void set_bounds(bklib::irect bounds);

    void show(message_log::show_type type, int n);

    void set_minimum_lines(int n);
private:
    int get_visible_lines_() const noexcept;

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
bkrl::detail::message_log_impl::message_log_impl(text_renderer& text_render)
  : text_renderer_ {text_render}
{
}

//--------------------------------------------------------------------------------------------------
void bkrl::detail::message_log_impl::println(bklib::utf8_string&& msg)
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
void bkrl::detail::message_log_impl::draw(renderer& render)
{
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
void bkrl::detail::message_log_impl::set_bounds(bklib::irect const bounds)
{
    bounds_ = bounds;
}

//--------------------------------------------------------------------------------------------------
void bkrl::detail::message_log_impl::show(message_log::show_type const type, int const n)
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
void bkrl::detail::message_log_impl::set_minimum_lines(int const n)
{
    min_lines_ = bklib::clamp(n, 0, bounds_.height() / text_renderer_.line_spacing());
}

//--------------------------------------------------------------------------------------------------
int bkrl::detail::message_log_impl::get_visible_lines_() const noexcept {
    return (visible_lines_ == show_all_lines)
      ? static_cast<int>(records_.size())
      : visible_lines_;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::message_log
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bkrl::message_log::message_log(text_renderer& text_render)
  : impl_ {std::make_unique<detail::message_log_impl>(text_render)}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::message_log::~message_log() = default;

//--------------------------------------------------------------------------------------------------
void bkrl::message_log::println(bklib::utf8_string msg) {
    impl_->println(std::move(msg));
}

//--------------------------------------------------------------------------------------------------
void bkrl::message_log::println(bklib::utf8_string_view const msg)
{
    println(msg.to_string());
}

//--------------------------------------------------------------------------------------------------
void bkrl::message_log::draw(renderer& render) {
    impl_->draw(render);
}

//--------------------------------------------------------------------------------------------------
void bkrl::message_log::set_bounds(bklib::irect const bounds) {
    impl_->set_bounds(bounds);
}

//--------------------------------------------------------------------------------------------------
void bkrl::message_log::show(show_type const type, int const n)
{
    impl_->show(type, n);
}

//--------------------------------------------------------------------------------------------------
void bkrl::message_log::set_minimum_lines(int const n)
{
    impl_->set_minimum_lines(n);
}
