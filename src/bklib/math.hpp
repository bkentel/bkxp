#pragma once

#include <type_traits>
#include <array>
#include <cstdlib>

namespace bklib {
////////////////////////////////////////////////////////////////////////////////////////////////////

struct transform_float_none  {};
struct transform_float_floor {};
struct transform_float_ceil  {};
struct transform_float_round {};

namespace detail {

template <typename Transform, typename T>
inline T transform_float(T const n, Transform, std::false_type) noexcept {
    return n;
}

template <typename T>
inline T transform_float(T const n, transform_float_none, std::true_type) noexcept {
    return n;
}

template <typename T>
inline T transform_float(T const n, transform_float_floor, std::true_type) noexcept {
    return std::floor(n);
}

template <typename T>
inline T transform_float(T const n, transform_float_ceil, std::true_type) noexcept {
    return std::ceil(n);
}

template <typename T>
inline T transform_float(T const n, transform_float_round, std::true_type) noexcept {
    return std::round(n);
}

} //namespace detail

template <typename Transform, typename T>
inline T transform_float(T const n) noexcept {
    static_assert(std::is_arithmetic<T>::value, "Not an arithmetic type.");
    return detail::transform_float(n, Transform {}, std::is_floating_point<T> {});
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename Result = void, typename T>
inline auto floor_to(T const n) noexcept {
    using result_t = std::conditional_t<std::is_same<void, Result>::value, T, Result>;
    return static_cast<result_t>(std::floor(n));
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename Result = void, typename T>
inline auto ceil_to(T const n) noexcept {
    using result_t = std::conditional_t<std::is_same<void, Result>::value, T, Result>;
    return static_cast<result_t>(std::ceil(n));
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename Result = void, typename T>
inline auto trunc_to(T const n) noexcept {
    using result_t = std::conditional_t<std::is_same<void, Result>::value, T, Result>;
    return static_cast<result_t>(std::trunc(n));
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename Result = void, typename T>
inline auto round_to(T const n) noexcept {
    using result_t = std::conditional_t<std::is_same<void, Result>::value, T, Result>;
    return static_cast<result_t>(std::round(n));
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename T>
inline constexpr T clamp(T const n, T const lo, T const hi) noexcept {
    return (n < lo) ? lo : (n > hi) ? hi : n;
}

template <typename T>
inline constexpr T clamp_min(T const n, T const lo) noexcept {
    return (n < lo) ? lo : n;
}

template <typename T>
inline constexpr T clamp_max(T const n, T const hi) noexcept {
    return (n > hi) ? hi : n;
}

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

template <unsigned D, typename T>
inline point_t<D, T>& operator+=(point_t<D, T>& p, vector_t<D, T> const v) noexcept {
    // TODO specialize for performance
    for (int i = 0; i < D; ++i) {
        p.data[i] += v.data[i];
    }

    return p;
}

template <unsigned D, typename T>
inline bool operator==(point_t<D, T> const p, point_t<D, T> const q) noexcept {
    for (int i = 0; i < D; ++i) {
        if (p.data[i] != q.data[i]) {
            return false;
        }
    }

    return true;
}

template <unsigned D, typename T>
inline bool operator!=(point_t<D, T> const p, point_t<D, T> const q) noexcept {
    return !(p == q);
}

template <unsigned D, typename T>
inline bool operator<(point_t<D, T> const p, point_t<D, T> const q) noexcept {
    for (int i = 0; i < D; ++i) {
        if (p.data[i] != q.data[i]) {
            return p.data[0] < q.data[0];
        }
    }

    return false;
}

template <unsigned D, typename T>
inline point_t<D, T>& operator+(point_t<D, T> const p, vector_t<D, T> const v) noexcept {
    auto q = p;
    return q += v;
}

template <unsigned D, typename T>
inline vector_t<D, T> operator-(point_t<D, T> const p, point_t<D, T> const q) noexcept {
    vector_t<D, T> result {p.data};

    for (int i = 0; i < D; ++i) {
        result.data[i] -= q.data[i];
    }

    return result;
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

//--------------------------------------------------------------------------------------------------
template <typename T>
constexpr inline bool operator==(rect_t<T> const lhs, rect_t<T> const rhs) noexcept {
    return (lhs.left   == rhs.left)
        && (lhs.top    == rhs.top)
        && (lhs.right  == rhs.right)
        && (lhs.bottom == rhs.bottom);
}

//--------------------------------------------------------------------------------------------------
template <typename T>
inline rect_t<T> intersection(rect_t<T> const a, rect_t<T> const b) noexcept {
    return {
        std::max(a.left,   b.left)
      , std::max(a.top,    b.top)
      , std::min(a.right,  b.right)
      , std::min(a.bottom, b.bottom)
    };
}

//--------------------------------------------------------------------------------------------------
template <typename T>
inline bool intersects(rect_t<T> const r, point_t<2, T> const p) noexcept {
    auto const x = bklib::x(p);
    auto const y = bklib::y(p);

    return r.left <= x && x < r.right
        && r.top  <= y && y < r.bottom;
}

//--------------------------------------------------------------------------------------------------
template <typename T>
inline bool intersects(point_t<2, T> const p, rect_t<T> const r) noexcept {
    return intersects(r, p);
}

//--------------------------------------------------------------------------------------------------
template <unsigned D, typename T>
inline auto distance2(point_t<D, T> const& u, point_t<D, T> const& v) noexcept {
    T result {};

    for (size_t i = 0; i < D; ++i) {
        auto const delta = v.data[i] - u.data[i];
        result += delta * delta;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
template <typename Tag, unsigned D, typename T, typename F>
inline decltype(auto) fold(tuple_base_t<Tag, D, T> const& n, F&& f)
    noexcept (noexcept(f(n.data[0], n.data[0])))
{
    auto result = n.data[0];

    for (size_t i = 1; i < D; ++i) {
        result = f(result, n.data[i]);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
template <typename Tag, unsigned D, typename T>
inline decltype(auto) max(tuple_base_t<Tag, D, T> const& n) noexcept {
    return fold(n, [](auto&& a, auto&& b) noexcept { return std::max(a, b); });
}

//--------------------------------------------------------------------------------------------------
template <typename Tag, unsigned D, typename T>
inline decltype(auto) min(tuple_base_t<Tag, D, T> const& n) noexcept {
    return fold(n, [](auto&& a, auto&& b) noexcept { return std::min(a, b); });
}

//--------------------------------------------------------------------------------------------------
template <typename Tag, unsigned D, typename T>
inline decltype(auto) abs_max(tuple_base_t<Tag, D, T> const& n) noexcept {
    return fold(n, [](auto&& a, auto&& b) noexcept {
        return std::max(std::abs(a), std::abs(b));
    });
}

//--------------------------------------------------------------------------------------------------
template <typename Tag, unsigned D, typename T>
inline decltype(auto) abs_min(tuple_base_t<Tag, D, T> const& n) noexcept {
    return fold(n, [](auto&& a, auto&& b) noexcept {
        return std::min(std::abs(a), std::abs(b));
    });
}

//--------------------------------------------------------------------------------------------------
template <unsigned D, typename T>
inline auto distance(point_t<D, T> const& u, point_t<D, T> const& v) noexcept {
    return std::sqrt(distance2(u, v));
}

//--------------------------------------------------------------------------------------------------
template <unsigned D1, unsigned D0, typename T, typename Tag>
inline auto truncate(tuple_base_t<Tag, D0, T> const& value) noexcept {
    static_assert(D1 < D0, "bad dimension");

    tuple_base_t<Tag, D1, T> result;
    for (size_t i = 0; i < D1; ++i) {
        result.data[i] = value.data[i];
    }

    return result;
}

using irect = rect_t<int>;

} // namespace bklib
////////////////////////////////////////////////////////////////////////////////////////////////////
