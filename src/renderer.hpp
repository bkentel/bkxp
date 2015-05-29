#pragma once

#include "bklib/math.hpp"
#include <memory>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {

class system;

namespace detail { class renderer_impl; }

class tilemap {
public:

    tilemap(
        int const tile_w, int const tile_h
      , int const texture_w, int const texture_h
      , int const origin_x, int const origin_y
    )
      : origin_x_(origin_x),       origin_y_(origin_y)
      , texture_w_(texture_w),     texture_h_(texture_h)
      , tile_w_(tile_w),           tile_h_(tile_h)
      , rows_(texture_h / tile_h), cols_(texture_w / tile_w)
    {
    }

    tilemap(int const tile_w, int const tile_h, int const texture_w, int const texture_h)
      : tilemap {tile_w, tile_h, texture_w, texture_h, 0, 0}
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

    int tile_w() const noexcept {
        return tile_w_;
    }

    int tile_h() const noexcept {
        return tile_h_;
    }
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

    void set_scale(double sx, double sy);
    void set_scale(double scale);

    void set_translation(double dx, double dy);

    bklib::point_t<2, double> get_scale() const;
    bklib::point_t<2, double> get_translation() const;

    void clear();
    void present();

    void set_active_texture(texture tex);

    void draw_filled_rect(rect_t r);
    void draw_cell(int cell_x, int cell_y, int tile_index);
private:
    std::unique_ptr<detail::renderer_impl> impl_;
};

} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////