#pragma once

#include "renderer.hpp"

#include "bklib/math.hpp"
#include "bklib/string.hpp"

#include <memory>
#include <vector>

namespace bklib { template <typename T> class dictionary; }

namespace bkrl {

struct color_def;
using color_dictionary = bklib::dictionary<color_def>;

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
class text_renderer {
public:
    using size_type = int16_t;
    using rect_t    = bklib::rect_t<size_type>;
    using point_t   = bklib::point_t<2, size_type>;

    virtual ~text_renderer();

    virtual rect_t load_glyph_info(bklib::utf8_string_view text) = 0;
    virtual size_type line_spacing() const noexcept = 0;

    virtual point_t bbox() const noexcept = 0;

    virtual void set_colors(color_dictionary const* colors = nullptr) = 0;
    virtual color4 get_color(bklib::utf8_string_view code) noexcept = 0;
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
std::unique_ptr<text_renderer> make_text_renderer();

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
class text_layout {
public:
    using size_type = text_renderer::size_type;

    static constexpr auto unlimited = size_type {-1};

    text_layout() = default;

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
    void clip_to(size_type w, size_type h);

    void draw(renderer& render) const;
    void draw(renderer& render, int x, int y) const;

    bklib::irect extent() const noexcept;
    bklib::irect bounds() const noexcept;

    size_type glyphs_at_line(int i) const noexcept;
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

    size_type x_ = 0;
    size_type y_ = 0;
    size_type w_ = unlimited;
    size_type h_ = unlimited;
    size_type actual_w_ = 0;
    size_type actual_h_ = 0;

    std::vector<render_info> data_;
};

} //namespace bkrl
