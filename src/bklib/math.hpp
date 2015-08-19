#pragma once

#include <limits>
#include <type_traits>
#include <array>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstring>

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

template <typename T, typename U>
inline constexpr T clamp_to(U const n) noexcept {
    static_assert(std::is_arithmetic<T>::value, "");
    static_assert(std::is_arithmetic<U>::value, "");

    using type = std::common_type_t<T, U>;

    return static_cast<T>(clamp(
        static_cast<type>(n)
      , static_cast<type>(std::numeric_limits<T>::min())
      , static_cast<type>(std::numeric_limits<T>::max())
    ));
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
template <typename T = int>
struct rect_t {
    constexpr rect_t() noexcept = default;

    constexpr rect_t(T const l, T const t, T const r, T const b) noexcept
        : left {l}, top {t}, right {r}, bottom {b}
    {
    }

    constexpr T width() const noexcept {
        return right - left;
    }

    constexpr T height() const noexcept {
        return bottom - top;
    }

    constexpr explicit operator bool() const noexcept {
        return (left < right) && (top < bottom);
    }

    T left, top, right, bottom;
};

//--------------------------------------------------------------------------------------------------
template <typename T>
constexpr inline rect_t<T> make_rect(T const left, T const top, T const right, T const bottom) noexcept {
    return rect_t<T> {left, top, right, bottom};
}

//--------------------------------------------------------------------------------------------------
template <typename T>
constexpr inline rect_t<T> add_border(rect_t<T> const r, T const border) noexcept {
    return rect_t<T> {r.left - border, r.top - border, r.right + border, r.bottom + border};
}

//--------------------------------------------------------------------------------------------------
template <typename T>
constexpr inline T perimeter(rect_t<T> const r) noexcept {
    return 2 * (r.width() - 1) + 2 * (r.height() - 1);
}

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
constexpr inline bool operator!=(rect_t<T> const lhs, rect_t<T> const rhs) noexcept {
    return !(lhs == rhs);
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename T>
struct aspect_ratio {
    constexpr aspect_ratio(T const a = T {1}, T const b = T {1}) noexcept
      : num {a > b ? a : b}
      , den {a > b ? b : a}
    {
    }

    template <typename U>
    constexpr U as() const noexcept {
        return static_cast<U>(num) / static_cast<U>(den);
    }

    T num;
    T den;
};

template <typename T>
inline constexpr aspect_ratio<T> make_aspect_ratio(T const num, T const den) noexcept {
    return aspect_ratio<T> {num, den};
}

template <typename T>
inline constexpr aspect_ratio<T> make_aspect_ratio(rect_t<T> const r) noexcept {
    return aspect_ratio<T> {r.width(), r.height()};
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename T>
struct range {
    constexpr T size() const noexcept {
        return hi - lo + 1;
    }

    T lo;
    T hi;
};

template <typename T>
constexpr inline bool operator==(range<T> const lhs, range<T> const rhs) noexcept {
    return (lhs.lo == rhs.lo) && (lhs.hi == rhs.hi);
}

template <typename T>
constexpr inline range<T> make_range(T const lo, T const hi) noexcept {
    return {lo, hi};
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
namespace detail {
template <unsigned N, typename T>
constexpr inline unsigned combine_bool_impl(T const head) noexcept {
    return (((!!head) ? 1u : 0u) << (N));
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
    for (auto i = 0u; i < D; ++i) {
        p.data[i] += v.data[i];
    }

    return p;
}

template <unsigned D, typename T>
inline bool operator==(point_t<D, T> const p, point_t<D, T> const q) noexcept {
    for (auto i = 0u; i < D; ++i) {
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

namespace detail {

template <unsigned D, typename T>
inline bool less(point_t<D, T> const p, point_t<D, T> const q) noexcept {
    return std::lexicographical_compare(begin(p.data), end(p.data), begin(q.data), end(q.data));
}

template <typename T>
constexpr inline bool less(point_t<2, T> const p, point_t<2, T> const q) noexcept {
    return q.data[0] <  p.data[0] ? false
         : q.data[0] != p.data[0] ? true
         : q.data[1] <  p.data[1] ? false
         : q.data[1] != p.data[1] ? true
         :                          false;
}

}

template <unsigned D, typename T>
constexpr inline bool operator<(point_t<D, T> const p, point_t<D, T> const q) noexcept {
    return detail::less(p, q);
}

template <unsigned D, typename T>
inline point_t<D, T> operator+(point_t<D, T> const p, vector_t<D, T> const v) noexcept {
    auto q = p;
    return q += v;
}

template <typename T>
inline constexpr point_t<2, T> operator+(point_t<2, T> const p, vector_t<2, T> const v) noexcept {
    return {x(p) + x(v), y(p) + y(v)};
}

template <unsigned D, typename T>
inline vector_t<D, T> operator-(point_t<D, T> const p, point_t<D, T> const q) noexcept {
    vector_t<D, T> result {p.data};

    for (auto i = 0u; i < D; ++i) {
        result.data[i] -= q.data[i];
    }

    return result;
}

using ipoint2 = point_t<2, int>;
using ivec2   = vector_t<2, int>;
using ipoint3 = point_t<3, int>;
using ivec3   = vector_t<3, int>;

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

    for (auto i = 0u; i < D; ++i) {
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

    for (auto i = 1u; i < D; ++i) {
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

    static_assert(std::is_trivially_destructible<T>::value &&
                  std::is_trivially_copy_constructible<T>::value, "");

    tuple_base_t<Tag, D1, T> result;
    std::memcpy(&result.data[0], &value.data[0], sizeof(result.data));
    return result;
}

//--------------------------------------------------------------------------------------------------
template <typename T>
inline constexpr rect_t<T> translate(rect_t<T> const r, T const dx, T const dy) noexcept {
    return {
        r.left  + dx, r.top    + dy
      , r.right + dx, r.bottom + dy
    };
}

//--------------------------------------------------------------------------------------------------
template <typename T>
inline constexpr rect_t<T> translate_to(rect_t<T> const r, T const x, T const y) noexcept {
    return translate(r, x - r.left, y - r.top);
}

//--------------------------------------------------------------------------------------------------
template <typename T>
std::pair<rect_t<T>, bool> move_rect_inside(rect_t<T> const bounds, rect_t<T> const r) noexcept {
    if (r.width()  > bounds.width()
     || r.height() > bounds.height()
    ) {
        return {r, false};
    }

    auto const dx = (r.left  < bounds.left ) ? bounds.left  - r.left
                  : (r.right > bounds.right) ? bounds.right - r.right
                  : 0;

    auto const dy = (r.top    < bounds.top   ) ? bounds.top    - r.top
                  : (r.bottom > bounds.bottom) ? bounds.bottom - r.bottom
                  : 0;

    return {translate(r, dx, dy), true};
}

using irect = rect_t<int>;

} // namespace bklib
////////////////////////////////////////////////////////////////////////////////////////////////////
