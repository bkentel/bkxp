#include "text.hpp"

#include "renderer.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
// text_renderer
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
class bkrl::detail::text_renderer_impl {
public:
    using size_type = text_renderer::size_type;
    using rect_t = text_renderer::rect_t;

    rect_t load_glyph_info(bklib::utf8_string_view const text) {
        if (text.empty() || text.front() > 0x7F) {
            return {0, 0, 18, 18};
        }

        auto const c = text.front();

        size_type const x = (c % 16) * 18;
        size_type const y = (c / 16) * 18;
        size_type const w = 18;
        size_type const h = 18;

        return {x, y, x + w, y + h};
    }

    size_type line_spacing() const {
        return 18;
    }
private:
};

//--------------------------------------------------------------------------------------------------
bkrl::text_renderer::text_renderer()
  : impl_ {std::make_unique<detail::text_renderer_impl>()}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::text_renderer::~text_renderer() = default;

//--------------------------------------------------------------------------------------------------
bkrl::text_renderer::rect_t
bkrl::text_renderer::load_glyph_info(bklib::utf8_string_view const text)
{
    return impl_->load_glyph_info(text);
}

//--------------------------------------------------------------------------------------------------
bkrl::text_renderer::size_type
bkrl::text_renderer::line_spacing() const
{
    return impl_->line_spacing();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// text_layout
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bkrl::text_layout::text_layout(
    text_renderer& render
  , bklib::utf8_string_view const text
  , size_type const x
  , size_type const y
  , size_type const w
  , size_type const h
)
  : x_ {x}, y_ {y}, w_ {w}, h_ {h}
{
    set_text(render, text);
}

//--------------------------------------------------------------------------------------------------
void bkrl::text_layout::clear(clear_type const type)
{
    data_.clear();

    if (type == clear_type::shrink) {
        data_.shrink_to_fit();
    }

    actual_w_ = 0;
    actual_h_ = 0;
}

//--------------------------------------------------------------------------------------------------
void bkrl::text_layout::set_text(text_renderer& render, bklib::utf8_string_view const text)
{    
    auto const line_h = render.line_spacing();
    auto const max_x  = w_;
    auto const max_y  = h_;

    size_type x = 0;
    size_type y = 0;

    clear();

    for (size_t i = 0; i < text.size(); ++i) {
        auto const glyph_info = render.load_glyph_info(text.substr(i, 1));
        
        auto const w = glyph_info.width();
        auto const h = glyph_info.height();
        
        x += w;

        // Wrap the line
        if (x > max_x) {           
            // No vertical space left
            if (y + line_h > max_y) {
                break;
            }

            actual_w_ = std::max(actual_w_, size_type {x - w});
            actual_h_ = std::max(actual_h_, size_type {y + line_h});

            x = 0;
            y += line_h;
        }
        
        data_.push_back(render_info {
            glyph_info.left, glyph_info.top, w, h
          , x, y
          , 0xFFFFFFFF
        });
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::text_layout::draw(renderer& render) const
{
    draw(render, x_, y_);
}

//--------------------------------------------------------------------------------------------------
void bkrl::text_layout::draw(renderer& render, int const x_off, int const y_off) const
{
    auto const scale = render.get_scale();
    auto const trans = render.get_translation();

    render.set_translation(x_off, y_off);
    render.set_scale(1.0);

    renderer::rect_t src {};
    renderer::rect_t dst {};

    for (auto const& glyph : data_) {
        src.x = glyph.src_x;
        src.y = glyph.src_y;
        src.w = glyph.src_w;
        src.h = glyph.src_h;
        
        dst.x = glyph.dst_x;
        dst.y = glyph.dst_y;

        render.draw_rect(src, dst);
    }

    render.set_scale(x(scale));
    render.set_translation(x(trans), y(trans));
}
