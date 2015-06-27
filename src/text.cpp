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
        if (text.empty() || static_cast<unsigned char>(text.front()) & unsigned char {0b1000'0000}) {
            return {0, 0, 18, 18};
        }

        auto const c = text.front();

        size_type const x = (c % 16) * 18;
        size_type const y = (c / 16) * 18;
        size_type const w = 18;
        size_type const h = 18;

        return {x, y, static_cast<size_type>(x + w), static_cast<size_type>(y + h)};
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
    auto const max_x  = (w_ == unlimited) ? std::numeric_limits<size_type>::max() : w_;
    auto const max_y  = (h_ == unlimited) ? std::numeric_limits<size_type>::max() : h_;

    size_type x = 0;
    size_type y = 0;

    clear();

    for (size_t i = 0; i < text.size(); ++i) {
        auto const glyph_info = render.load_glyph_info(text.substr(i, 1));

        auto const w = glyph_info.width();
        auto const h = glyph_info.height();

        // Wrap the line
        if (x + w > max_x) {
            size_type const next_y = y + line_h;

            // No vertical space left
            if (next_y > max_y) {
                break;
            }

            actual_w_ = std::max(actual_w_, x);
            actual_h_ = std::max(actual_h_, next_y);

            x = 0;
            y = next_y;
        }

        data_.push_back(render_info {
            glyph_info.left, glyph_info.top, w, h
          , x, y
          , 0xFFFFFFFF
        });

        x += w;
    }

    actual_w_ = std::max(actual_w_, x);
    actual_h_ = std::max(actual_h_, line_h);
}

//--------------------------------------------------------------------------------------------------
void bkrl::text_layout::set_position(int const x, int const y)
{
    x_ = static_cast<size_type>(x);
    y_ = static_cast<size_type>(y);
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

    dst.h = 18;
    dst.w = 18;

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

//--------------------------------------------------------------------------------------------------
bklib::irect bkrl::text_layout::extent() const noexcept
{
    return {x_, y_, x_ + actual_w_, y_ + actual_h_};
}
