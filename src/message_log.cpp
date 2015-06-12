#include "message_log.hpp"

#include "renderer.hpp"
#include "text.hpp"
#include "bklib/assert.hpp"

#include <deque>

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

    int visible_lines_ = show_all_lines;
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
    text_layout text {text_renderer_, msg};

    records_.push_front(record_t {
        std::move(text), std::move(msg)
    });

    show(bkrl::message_log::show_type::more, 1);
}

//--------------------------------------------------------------------------------------------------
void bkrl::detail::message_log_impl::draw(renderer& render)
{
    int x = 0;
    int y = 0;

    auto n = get_visible_lines_();
    for (auto const& rec : records_) {
        if (n-- <= 0) {
            break;
        }

        auto const extent = rec.text.extent();
        rec.text.draw(render, x, y);
        y += extent.height();
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::detail::message_log_impl::set_bounds(bklib::irect const bounds)
{
}

void bkrl::detail::message_log_impl::show(message_log::show_type const type, int const n)
{
    BK_PRECONDITION(n >= 0);

    auto const count = get_visible_lines_();

    using st = message_log::show_type;
    switch (type) {
    case st::none :
        visible_lines_ = 0;
        break;
    case st::less :
        visible_lines_ = std::max(count - n, 0);
        break;
    case st::more :
        visible_lines_ = std::min(count + n, static_cast<int>(records_.size()));
        break;
    case st::all :
        visible_lines_ = show_all_lines;
        break;
    default :
        BK_ASSERT(false);
        break;
    }
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
