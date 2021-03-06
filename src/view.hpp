#pragma once

#include "bklib/math.hpp"

namespace bkrl {

template <typename T, typename Fallback>
using type_or_default = std::conditional_t<
    std::is_same<T, void>::value, Fallback, T
>;

class view {
public:
    static constexpr double zoom_min   = 0.25;
    static constexpr double zoom_max   = 4.0;
    static constexpr double zoom_delta = 0.1;

    //----------------------------------------------------------------------------------------------
    view(int const window_w, int const window_h, int const tile_w, int const tile_h) noexcept
      : window_w_(window_w), window_h_(window_h)
      , tile_w_(tile_w), tile_h_(tile_h)
    {
    }

    //----------------------------------------------------------------------------------------------
    void set_window_size(int const w, int const h) noexcept {
        window_w_ = w;
        window_h_ = h;
    }

    //----------------------------------------------------------------------------------------------
    template <typename T>
    void center_on_world(T const x, T const y) noexcept {
        scroll_x_ = window_w_ / 2.0 - x * tile_w_ * zoom_x_;
        scroll_y_ = window_h_ / 2.0 - y * tile_h_ * zoom_y_;
    }

    //----------------------------------------------------------------------------------------------
    template <typename Result = void, typename T>
    decltype(auto) screen_to_world_as(T const x, T const y) const noexcept {
        auto const xt = (x - scroll_x_) / zoom_x_ / tile_w_;
        auto const yt = (y - scroll_y_) / zoom_y_ / tile_h_;

        using fallback_t = std::common_type_t<decltype(xt), decltype(yt)>;
        using result_t = type_or_default<Result, fallback_t>;

        return bklib::point_t<2, result_t> {static_cast<result_t>(xt), static_cast<result_t>(yt)};
    }

    //----------------------------------------------------------------------------------------------
    template <typename Transform = bklib::transform_float_floor>
    bklib::ipoint2 screen_to_world(int const x, int const y) const noexcept {
        auto const p = screen_to_world_as(x, y);

        auto const xt = bklib::transform_float<Transform>(bklib::x(p));
        auto const yt = bklib::transform_float<Transform>(bklib::y(p));

        return {static_cast<int>(xt), static_cast<int>(yt)};
    }

    //----------------------------------------------------------------------------------------------
    bklib::irect screen_to_world(bklib::irect const r) const noexcept {
        auto const tl = screen_to_world<bklib::transform_float_round>(r.left,  r.top);
        auto const br = screen_to_world<bklib::transform_float_round>(r.right, r.bottom);

        return {x(tl), y(tl), x(br), y(br)};
    }

    //----------------------------------------------------------------------------------------------
    bklib::irect screen_to_world() const noexcept {
        bklib::irect const r {0, 0, window_w_, window_h_};

        auto const tl = screen_to_world<bklib::transform_float_floor>(r.left,  r.top);
        auto const br = screen_to_world<bklib::transform_float_ceil>(r.right, r.bottom);

        return {x(tl), y(tl), x(br), y(br)};
    }

    //----------------------------------------------------------------------------------------------
    bklib::ipoint2 world_to_screen(int const x, int const y) const noexcept {
        return {
            bklib::round_to<int>((x + 0.5) * tile_w_ * zoom_x_ - scroll_x_)
          , bklib::round_to<int>((y + 0.5) * tile_h_ * zoom_y_ - scroll_y_)
        };
    }

    //----------------------------------------------------------------------------------------------
    void scroll_by_world(double const dx, double const dy) noexcept {
        scroll_x_ += dx * zoom_x_;
        scroll_y_ += dy * zoom_y_;
    }

    //----------------------------------------------------------------------------------------------
    void scroll_by_screen(double const dx, double const dy) noexcept {
        scroll_x_ += dx;
        scroll_y_ += dy;
    }

    //----------------------------------------------------------------------------------------------
    void zoom_in() noexcept {
        zoom_x_ = bklib::clamp_max(zoom_x_ * (1.0 + zoom_delta), zoom_max);
        zoom_y_ = bklib::clamp_max(zoom_y_ * (1.0 + zoom_delta), zoom_max);
    }

    //----------------------------------------------------------------------------------------------
    void zoom_out() noexcept {
        zoom_x_ = bklib::clamp_min(zoom_x_ * (1.0 - zoom_delta), zoom_min);
        zoom_y_ = bklib::clamp_min(zoom_y_ * (1.0 - zoom_delta), zoom_min);
    }

    //----------------------------------------------------------------------------------------------
    void zoom_to(double const zx, double const zy) noexcept {
        zoom_x_ = bklib::clamp(zx, zoom_min, zoom_max);
        zoom_y_ = bklib::clamp(zy, zoom_min, zoom_max);
    }

    //----------------------------------------------------------------------------------------------
    void zoom_to(double const zoom = 1.0) noexcept {
        zoom_to(zoom, zoom);
    }

    //----------------------------------------------------------------------------------------------
    bklib::ipoint2 origin() const noexcept {
        return {
            bklib::round_to<int>(scroll_x_)
          , bklib::round_to<int>(scroll_y_)
        };
    }

    //----------------------------------------------------------------------------------------------
    bklib::point_t<2, double> get_zoom() const noexcept {
        return {zoom_x_, zoom_y_};
    }

    //----------------------------------------------------------------------------------------------
    bklib::point_t<2, double> get_scroll() const noexcept {
        return {scroll_x_, scroll_y_};
    }
private:
    int window_w_;
    int window_h_;
    int tile_w_;
    int tile_h_;
    double zoom_x_   = 1.0; //!< scaling factor
    double zoom_y_   = 1.0; //!< scaling factor
    double scroll_x_ = 0.0; //!< translation in world space
    double scroll_y_ = 0.0; //!< translation in world space
};

} //namespace bkrl
