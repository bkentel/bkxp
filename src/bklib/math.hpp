#pragma once

#include <type_traits>
#include <array>
#include <cstdlib>

namespace bklib {
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename T = int, typename SetPixel>
void bresenham_line(T const x0, T const y0, T const x1, T const y1, SetPixel&& set_pixel) {
    static_assert(std::is_signed<T>::value, "");

    T const dx = std::abs(x1 - x0);
    T const dy = std::abs(y1 - y0);
    T const sx = (x0 < x1) ? 1 : -1;
    T const sy = (y0 < y1) ? 1 : -1;
    T       x  = x0;
    T       y  = y0;

    for (T err = (dx > dy ? dx : -dy) / 2; ; ) {
        set_pixel(x, y);

        if (x == x1 && y == y1) {
            break;
        }

        T const prev_err = err;

        if (prev_err > -dx) { err -= dy; x += sx; }
        if (prev_err <  dy) { err += dx; y += sy; }
    }
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename T>
struct aspect_ratio {
    aspect_ratio(T const num = T {1}, T const den = T {1}) noexcept
      : num {num > den ? num : den}
      , den {num > den ? den : num}
    {
    }

    T num;
    T den;
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename T>
constexpr T clamp(T const lo, T const n, T const hi) noexcept {
    return (n < lo) ? lo : (n > hi) ? hi : n;
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
namespace detail {
template <unsigned N, typename T>
constexpr inline unsigned combine_bool_impl(T const head) noexcept {
    return (((!!head) ? 1 : 0) << (N));
}

template <unsigned N, typename T, typename... Ts>
constexpr inline unsigned combine_bool_impl(T const head, Ts const... tail) noexcept {
    return combine_bool_impl<N>(head) | combine_bool_impl<N - 1>(tail...);
}
} //namespace detail

template <typename... Ts>
constexpr inline unsigned combine_bool(Ts const... values) noexcept {
    return detail::combine_bool_impl<sizeof...(Ts) - 1>(values...);
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------

template <typename Tag, unsigned Dimension, typename T = int>
struct tuple_base_t {
    using tag_t = Tag;
    using value_t = T;
    static constexpr unsigned dimension = Dimension;

    std::array<value_t, Dimension> data;
};

template <unsigned Dimension, typename T = int>
using point_t = tuple_base_t<struct tag_point_t, Dimension, T>;

template <unsigned Dimension, typename T = int>
using vector_t = tuple_base_t<struct tag_vector_t, Dimension, T>;

template <typename Tag, unsigned D, typename T>
constexpr T x(tuple_base_t<Tag, D, T> const& p) noexcept {
    static_assert(D > 0, "wrong dimension");
    return p.data[0];
}

template <typename Tag, unsigned D, typename T>
constexpr T y(tuple_base_t<Tag, D, T> const& p) noexcept {
    static_assert(D > 1, "wrong dimension");
    return p.data[1];
}

template <typename Tag, unsigned D, typename T>
constexpr T z(tuple_base_t<Tag, D, T> const& p) noexcept {
    static_assert(D > 2, "wrong dimension");
    return p.data[2];
}

using ipoint2 = point_t<2, int>;
using ivec2   = vector_t<2, int>;
using ipoint3 = point_t<3, int>;
using ivec3   = vector_t<3, int>;

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename T = int>
struct rect_t {
    T width() const noexcept {
        return right - left;
    }

    T height() const noexcept {
        return bottom - top;
    }

    explicit operator bool() const noexcept {
        return (left < right) && (top < bottom);
    }

    T left, top, right, bottom;
};

using irect = rect_t<int>;

} // namespace bklib
////////////////////////////////////////////////////////////////////////////////////////////////////
