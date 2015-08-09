#include "text.hpp"
#include "renderer.hpp"
#include "color.hpp"

#include "bklib/assert.hpp"
#include "bklib/scope_guard.hpp"
#include "bklib/dictionary.hpp"
#include "bklib/algorithm.hpp"
#include "bklib/stack_arena_allocator.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
// text_renderer
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
class bkrl::detail::text_renderer_impl {
public:
    using size_type = text_renderer::size_type;
    using rect_t = text_renderer::rect_t;
    using point_t = text_renderer::point_t;

    rect_t load_glyph_info(bklib::utf8_string_view const text) {
        if (text.empty() || static_cast<std::uint8_t>(text.front()) & std::uint8_t {0b1000'0000}) {
            return {0, 0, 18, 18};
        }

        auto const c = text.front();

        size_type const x = (c % 16) * 18;
        size_type const y = (c / 16) * 18;
        size_type const w = 18;
        size_type const h = 18;

        return {x, y, static_cast<size_type>(x + w), static_cast<size_type>(y + h)};
    }

    size_type line_spacing() const noexcept {
        return 18;
    }

    point_t bbox() const noexcept {
        return {18, 18};
    }

    void set_colors(color_dictionary const* const colors) {
        if (!colors) {
            colors_.clear();
            return;
        }

        std::transform(
            std::begin(*colors)
          , std::end(*colors)
          , std::back_inserter(colors_)
          , [&](color_def const& def) {
                return color_t {make_color_key(def.short_name), def.color};
          });

        auto const last = end(colors_);
        std::sort(begin(colors_), last);
        auto const it = std::unique(begin(colors_), last);
        if (it != last) {
            BK_ASSERT(false); //TODO
        }
    }

    color4 get_color(bklib::utf8_string_view const code) noexcept {
        auto const last = end(colors_);
        auto const key  = make_color_key(code);
        auto const it   = std::lower_bound(begin(colors_), last, key);

        return (it != last && it->key == key)
          ? it->value
          : make_color(255, 255, 255);
    }
private:
    struct color_t {
        uint32_t key;
        color4   value;

        constexpr friend bool operator==(color_t const lhs, color_t const rhs) noexcept {
            return lhs.key == rhs.key;
        }

        constexpr friend bool operator<(uint32_t const lhs, color_t const rhs) noexcept {
            return lhs < rhs.key;
        }

        constexpr friend bool operator<(color_t const lhs, uint32_t const rhs) noexcept {
            return lhs.key < rhs;
        }

        constexpr friend bool operator<(color_t const lhs, color_t const rhs) noexcept {
            return lhs.key < rhs.key;
        }
    };

    static uint32_t make_color_key(bklib::utf8_string_view const code) noexcept {
        uint32_t key = 0;

        switch (code.size()) {
        default: BK_FALLTHROUGH
        case 4: key |= static_cast<uint32_t>(code[3]) << 24; BK_FALLTHROUGH
        case 3: key |= static_cast<uint32_t>(code[2]) << 16; BK_FALLTHROUGH
        case 2: key |= static_cast<uint32_t>(code[1]) <<  8; BK_FALLTHROUGH
        case 1: key |= static_cast<uint32_t>(code[0]) <<  0; BK_FALLTHROUGH
        case 0: break;
        }

        return key;
    }

    std::vector<color_t> colors_;
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
bkrl::text_renderer::line_spacing() const noexcept
{
    return impl_->line_spacing();
}

//--------------------------------------------------------------------------------------------------
bkrl::text_renderer::point_t bkrl::text_renderer::bbox() const noexcept {
    return impl_->bbox();
}

//--------------------------------------------------------------------------------------------------
void bkrl::text_renderer::set_colors(color_dictionary const* const colors) {
    impl_->set_colors(colors);
}

//--------------------------------------------------------------------------------------------------
bkrl::color4 bkrl::text_renderer::get_color(bklib::utf8_string_view const code) noexcept {
    return impl_->get_color(code);
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


namespace {

std::pair<size_t, char> parse_escape(bklib::utf8_string_view const s, size_t pos) noexcept {
    BK_PRECONDITION(s[pos] == '\\');

    switch (s[++pos]) {
    case '<':
    case '>':
        break;
    default:
        BK_ASSERT(false);
        break;
    }

    return {pos, s[pos]};
}

enum class tag_type {
    invalid
  , color
};

struct tag_data {
    bklib::utf8_string_view type;
    bklib::utf8_string_view value;
    size_t next_pos;
    bool is_end;
};

tag_data parse_tag(bklib::utf8_string_view const s, size_t pos) noexcept {
    BK_PRECONDITION(s[pos] == '<');

    constexpr auto const npos = bklib::utf8_string_view::npos;

    auto const tag_beg = s.substr(pos);
    auto const tag_end = tag_beg.find('>');

    if (tag_end == npos) {
        BK_ASSERT(false); //TODO
    }

    auto const tag      = tag_beg.substr(0, tag_end + 1);
    auto const eq_pos   = tag.find('=');
    auto const tag_type = tag.substr(1, eq_pos - 1);
    bool const is_end   = !tag_type.empty() && tag_type.starts_with('/');

    if (eq_pos == npos) {
        size_t const offset = is_end? 1 : 0;
        return {tag_type.substr(offset, tag_end - (offset + 1)), "", pos + tag_end, is_end};
    }

    if (eq_pos < 1) {
        BK_ASSERT(false); //TODO
    }

    auto const tag_value = tag.substr(eq_pos + 1, tag.size() - (eq_pos + 1) - 1);

    return {tag_type, tag_value, pos + tag_end, is_end};
}

}

//--------------------------------------------------------------------------------------------------
void bkrl::text_layout::set_text(
    text_renderer& render
  , bklib::utf8_string_view const text
) {
    auto const line_h = render.line_spacing();
    auto const max_x  = (w_ == unlimited) ? std::numeric_limits<size_type>::max() : w_;
    auto const max_y  = (h_ == unlimited) ? std::numeric_limits<size_type>::max() : h_;

    size_type x = 0;
    size_type y = 0;

    clear();

    constexpr size_t last_color_size = 16;

    using alloc_t = bklib::short_alloc<uint32_t, 64>;
    alloc_t::arena_t arena;
    alloc_t alloc {arena};

    std::vector<uint32_t, alloc_t> last_color(alloc);

    last_color.reserve(last_color_size);
    last_color.push_back(color_code(make_color(255, 255, 255)));

    bool in_color = false; // TODO
    size_t break_last      = 0;
    size_t break_candidate = 0;

    for (size_t i = 0; i < text.size(); ++i) {
        auto beg = text.substr(i, 1);
        auto c   = beg[0];

        switch (c) {
        case '\\':
            std::tie(i, c) = parse_escape(text, i);
            break;
        case '<': {
            auto const result = parse_tag(text, i);
            i = result.next_pos;

            if (result.is_end) {
                if (in_color) {
                    last_color.pop_back();
                    in_color = false;
                }
            } else {
                last_color.push_back(color_code(render.get_color(result.value)));
                in_color = true;
            }

            continue;
        }
        default:
            break;
        }

        auto const glyph_info = render.load_glyph_info(beg);

        auto const w = glyph_info.width();
        auto const h = glyph_info.height();

        // wrap, or move, to next line
        if (c == '\n' || (x + w > max_x)) {
            size_type const next_y = y + line_h;

            // no vertical space left
            if (next_y >= max_y) {
                break;
            }

            actual_w_ = std::max(actual_w_, x);
            actual_h_ = std::max(actual_h_, next_y);

            x = 0;
            y = next_y;

            auto const n = data_.size();

            if (break_candidate != break_last // mid-word break
             && break_candidate != n          // break at a candidate
            ) {
                // break after a candidate -- shift and move down the spill-over
                auto const first = break_candidate;
                auto const dx = data_[first].dst_x;

                for (auto j = first; j < n; ++j) {
                    data_[j].dst_x -= dx;
                    data_[j].dst_y  = y;
                }

                x = data_.back().dst_x + data_.back().src_w;
            }

            break_last      = n - 1;
            break_candidate = break_last;

            if (c == '\n' || c == ' ') {
                break_candidate = ++break_last;
                continue;
            }
        }

        if (c == ' ') {
            break_candidate = data_.size() + 1;
        }

        data_.push_back(render_info {
            glyph_info.left, glyph_info.top, w, h, x, y, last_color.back()});

        x += w;

        actual_h_ = std::max(actual_h_, static_cast<size_type>(y + h));
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
void bkrl::text_layout::clip_to(size_type const w, size_type const h)
{
    w_ = w;
    h_ = h;
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

    BK_SCOPE_EXIT {
        render.set_scale(x(scale));
        render.set_translation(x(trans), y(trans));
    };

    render.set_translation(0, 0);
    render.set_scale(1.0);

    render.draw_rects(x_off, y_off, data_.size(), data_.data()
        , offsetof(render_info, src_x), sizeof(render_info::src_x)
        , offsetof(render_info, dst_x), sizeof(render_info::dst_x)
        , offsetof(render_info, color), sizeof(render_info::color)
        , sizeof(render_info)
    );
}

//--------------------------------------------------------------------------------------------------
bklib::irect bkrl::text_layout::extent() const noexcept
{
    return {x_, y_, x_ + actual_w_, y_ + actual_h_};
}

//--------------------------------------------------------------------------------------------------
bklib::irect bkrl::text_layout::bounds() const noexcept
{
    auto const max_x  = (w_ == unlimited) ? std::numeric_limits<size_type>::max() : x_ + w_;
    auto const max_y  = (h_ == unlimited) ? std::numeric_limits<size_type>::max() : y_ + h_;

    return {x_, y_, max_x, max_y};
}
