#pragma once

#include "bklib/math.hpp"
#include "bklib/string.hpp"

#include <memory>
#include <vector>

namespace bkrl {

class renderer;

namespace detail { class text_renderer_impl; }

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
class text_renderer {
public:
    using size_type = int16_t;
    using rect_t = bklib::rect_t<size_type>;

    text_renderer();
    ~text_renderer();

    rect_t load_glyph_info(bklib::utf8_string_view text);
    size_type line_spacing() const;
private:
    std::unique_ptr<detail::text_renderer_impl> impl_;
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
class text_layout {
public:
    using size_type = text_renderer::size_type;

    static constexpr auto unlimited = size_type {-1};

    text_layout(text_renderer& render, bklib::utf8_string_view text
        , size_type x = 0
        , size_type y = 0
        , size_type w = unlimited
        , size_type h = unlimited
    );

    enum class clear_type {
        no_shrink, shrink
    };

    void clear(clear_type type = clear_type::no_shrink);

    void set_text(text_renderer& render, bklib::utf8_string_view text);
    void set_position(int x, int y);

    void draw(renderer& render) const;
    void draw(renderer& render, int x, int y) const;

    bklib::irect extent() const noexcept;
private:
    struct render_info {
        size_type src_x;
        size_type src_y;
        size_type src_w;
        size_type src_h;
        size_type dst_x;
        size_type dst_y;
        uint32_t  color;
    };

    size_type x_;
    size_type y_;
    size_type w_;
    size_type h_;
    size_type actual_w_ = 0;
    size_type actual_h_ = 0;

    std::vector<render_info> data_;
};

} //namespace bkrl
