#pragma once

#include "bklib/string.hpp"
#include "bklib/math.hpp"
#include <memory>

namespace bkrl {

class renderer;
class text_renderer;

class message_log {
public:
    virtual ~message_log();

    virtual void println(bklib::utf8_string msg) = 0;
    virtual void println(bklib::utf8_string_view msg) = 0;

    virtual void draw(renderer& render) = 0;

    virtual void set_bounds(bklib::irect bounds) = 0;

    enum class show_type {
        none, less, more, all
    };

    virtual void show(show_type type, int n = 1) = 0;

    virtual void set_minimum_lines(int n) = 0;
};

std::unique_ptr<message_log> make_message_log(text_renderer& text_render);

} //namespace bkrl
