#pragma once

#include "bklib/string.hpp"
#include "bklib/math.hpp"
#include <memory>

namespace bkrl {

class renderer;
class text_renderer;

namespace detail { class message_log_impl; }

class message_log {
public:
    explicit message_log(text_renderer& text_render);
    ~message_log();

    void println(bklib::utf8_string msg);
    void println(bklib::utf8_string_view msg);

    void draw(renderer& render);

    void set_bounds(bklib::irect bounds);
private:
    std::unique_ptr<detail::message_log_impl> impl_;
};

} //namespace bkrl
