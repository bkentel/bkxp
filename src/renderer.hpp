#pragma once

#include "bklib/math.hpp"
#include <memory>
#include <array>
#include <cstdint>
#include <cstddef>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {

class system;

namespace detail { class renderer_impl; }

class tilemap {
public:
    tilemap(
        int const tile_w, int const tile_h
      , int const texture_w, int const texture_h
      , int const origin_x = 0, int const origin_y = 0
    ) noexcept
      : origin_x_(origin_x),       origin_y_(origin_y)
      , texture_w_(texture_w),     texture_h_(texture_h)
      , tile_w_(tile_w),           tile_h_(tile_h)
      , rows_(texture_h / tile_h), cols_(texture_w / tile_w)
    {
    }

    bklib::irect get_bounds(int const i) const noexcept {
        auto const yi = i / cols_;
        auto const xi = i % cols_;

        return {
            origin_x_ + xi * tile_w_
          , origin_y_ + yi * tile_h_
          , origin_x_ + xi * tile_w_ + tile_w_
          , origin_y_ + yi * tile_h_ + tile_h_
        };
    }

    int tile_w()    const noexcept { return tile_w_; }
    int tile_h()    const noexcept { return tile_h_; }
    int texture_w() const noexcept { return texture_w_; }
    int texture_h() const noexcept { return tile_h_; }
    int cols()      const noexcept { return cols_; }
    int rows()      const noexcept { return rows_; }
private:
    int origin_x_;
    int origin_y_;
    int texture_w_;
    int texture_h_;
    int tile_w_;
    int tile_h_;
    int rows_;
    int cols_;
};

using color4 = std::array<uint8_t, 4>;

constexpr inline color4 make_color(
    uint8_t const r
  , uint8_t const g
  , uint8_t const b
  , uint8_t const a = uint8_t {255}
) noexcept {
    return {r, g, b, a};
}

constexpr inline uint32_t color_code(color4 const c) noexcept {
    return c[0] << 0 | c[1] << 8 | c[2] << 16 | c[3] << 24;
}

class renderer {
public:
    enum class texture {
        none, terrain, items, entities,
    };

    struct rect_t {
        int x, y;
        int w, h;
    };

    explicit renderer(system& sys);
    ~renderer();

    void   clear_clip_region();
    void   set_clip_region(rect_t r);
    rect_t get_clip_region();

    void set_scale(double sx, double sy);
    void set_scale(double scale);

    void set_translation(double dx, double dy);

    bklib::point_t<2, double> get_scale() const;
    bklib::point_t<2, double> get_translation() const;

    void clear();
    void present();

    void set_active_texture(texture tex);

    void draw_filled_rect(rect_t r);
    void draw_filled_rect(rect_t r, color4 c);
    void draw_cell(int cell_x, int cell_y, int tile_index);
    void draw_cell(int cell_x, int cell_y, int tile_index, color4 color);

    void draw_rect(rect_t src, rect_t dst);

    void draw_cells(
        int xoff, int yoff
      , size_t w, size_t h
      , void const* data
      , ptrdiff_t tex_offset, size_t tex_size
      , size_t stride
    );

    void draw_rects(
        int xoff, int yoff
      , size_t count
      , void const* data
      , ptrdiff_t src_pos_offset, size_t src_pos_size
      , ptrdiff_t dst_pos_offset, size_t dst_pos_size
      , ptrdiff_t color_offset,   size_t color_size
      , size_t stride
    );
private:
    std::unique_ptr<detail::renderer_impl> impl_;
};

constexpr inline auto make_renderer_rect(bklib::irect const r) noexcept {
    return renderer::rect_t {r.left, r.top, r.width(), r.height()};
}

struct terrain_render_data_t {
    uint16_t base_index;
    uint16_t unused0;
    uint16_t unused1;
    uint16_t unused2;
};

struct creature_render_data_t {
    int16_t x, y;
    uint16_t base_index;
    color4 color;
};

struct item_render_data_t {
    int16_t x, y;
    uint16_t base_index;
    color4 color;
};

} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
