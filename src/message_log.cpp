#include "message_log.hpp"
#include "renderer.hpp"
#include "text.hpp"

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
private:
    struct record_t {
        text_layout        text;
        bklib::utf8_string string;
    };

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
}

//--------------------------------------------------------------------------------------------------
void bkrl::detail::message_log_impl::draw(renderer& render)
{
    int x = 0;
    int y = 0;

    for (auto const& rec : records_) {
        auto const extent = rec.text.extent();
        rec.text.draw(render, x, y);
        y += extent.height();
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::detail::message_log_impl::set_bounds(bklib::irect bounds)
{
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
