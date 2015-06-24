#pragma once

#include "random.hpp"
#include "bklib/math.hpp"

#include <memory>
#include <functional>

namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail { class bsp_layout_impl; }

class bsp_layout {
public:
    struct param_t {
        param_t() = default;

        //! Aspect ration above which to force a split.
        bklib::aspect_ratio<int> aspect {16, 9};
        //! Minimum cell width.
        int min_w = 5;
        //! Minimum cell height.
        int min_h = 5;
        //! Maximum cell dimension.
        int max_edge_size = 25;
        //! Minimum chance to split a cell.
        double min_split_chance = 0.1;
    };

    ~bsp_layout();
    bsp_layout(bklib::irect bounds, param_t params);
    explicit bsp_layout(bklib::irect bounds);

    using room_gen_t = std::function<void (bklib::irect const&)>;

    void generate(random_t& gen, room_gen_t room_gen);
private:
    std::unique_ptr<detail::bsp_layout_impl> impl_;
};

//--------------------------------------------------------------------------------------------------
inline std::pair<bklib::irect, bklib::irect>
split_vertical(bklib::irect const r, int const offset) noexcept {
    BK_PRECONDITION(!!r);
    BK_PRECONDITION(offset > 0);
    BK_PRECONDITION(offset < r.width());

    return std::make_pair(
        bklib::irect {r.left,          r.top, r.left + offset, r.bottom}
      , bklib::irect {r.left + offset, r.top, r.right,         r.bottom}
    );
}

//--------------------------------------------------------------------------------------------------
inline std::pair<bklib::irect, bklib::irect>
split_horizontal(bklib::irect const r, int const offset) noexcept {
    BK_PRECONDITION(!!r);
    BK_PRECONDITION(offset > 0);
    BK_PRECONDITION(offset < r.height());

    return std::make_pair(
        bklib::irect {r.left, r.top,          r.right, r.top + offset}
      , bklib::irect {r.left, r.top + offset, r.right, r.bottom}
    );
}

//--------------------------------------------------------------------------------------------------
enum class split_type {
    none //!< Cannot be split.
  , can  //!< Can be split.
  , must //!< Must be split.
};

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
namespace detail {
inline constexpr split_type classify_split_(
    int const num
  , int const den
  , int const min
  , int const max
  , bklib::aspect_ratio<int> const aspect_limit
) noexcept {
    return (num <= min) ? split_type::none
         : (num >  max) ? split_type::must
         : (aspect_limit.as<double>() < static_cast<double>(num) / static_cast<double>(den)) ? split_type::must
         : split_type::can;
}

inline constexpr split_type classify_split_(
    int const num
  , int const min
  , int const max
) noexcept {
    return (num <= min) ? split_type::none
         : (num >  max) ? split_type::must
         : split_type::can;
}
} //namespace detail

//--------------------------------------------------------------------------------------------------
inline constexpr split_type classify_split_vertical(
    bklib::irect const r
  , int const min = std::numeric_limits<int>::min()
  , int const max = std::numeric_limits<int>::max()
  , bklib::aspect_ratio<int> const aspect_limit = {0, 1}
) noexcept {
    return (aspect_limit.den)
      ? detail::classify_split_(r.width(), r.height(), min, max, aspect_limit)
      : detail::classify_split_(r.width(), min, max);
}

//--------------------------------------------------------------------------------------------------
inline constexpr split_type classify_split_horizontal(
    bklib::irect const r
  , int const min = std::numeric_limits<int>::min()
  , int const max = std::numeric_limits<int>::max()
  , bklib::aspect_ratio<int> const aspect_limit = {0, 1}
) noexcept {
    return (aspect_limit.den)
      ? detail::classify_split_(r.height(), r.width(), min, max, aspect_limit)
      : detail::classify_split_(r.height(), min, max);
}

//--------------------------------------------------------------------------------------------------
struct classify_split_result_t {
    split_type vertical;
    split_type horizontal;
};

//--------------------------------------------------------------------------------------------------
inline constexpr classify_split_result_t classify_split(
    bklib::irect const r
  , int const min_w = std::numeric_limits<int>::min()
  , int const max_w = std::numeric_limits<int>::max()
  , int const min_h = std::numeric_limits<int>::min()
  , int const max_h = std::numeric_limits<int>::max()
  , bklib::aspect_ratio<int> const aspect_limit = {0, 1}
) noexcept {
    return {
        classify_split_vertical(r, min_w, max_w, aspect_limit)
      , classify_split_horizontal(r, min_h, max_h, aspect_limit)
    };
}

inline auto random_split(random_t& random, bklib::irect const r
  , int const min_w = std::numeric_limits<int>::min()
  , int const max_w = std::numeric_limits<int>::max()
  , int const min_h = std::numeric_limits<int>::min()
  , int const max_h = std::numeric_limits<int>::max()
  , bklib::aspect_ratio<int> const aspect_limit = {0, 1}
) {
    auto const split = [&](auto const& f, int const min, int const value) {
        return std::tuple_cat(f(r, random_range(random, min, value - min)), std::make_tuple(true));
    };

    auto const split_h = [&] { return split(split_horizontal, min_h, r.height()); };
    auto const split_v = [&] { return split(split_vertical,   min_w, r.width());  };

    auto const type = classify_split(r, min_w, max_w, min_h, max_h, aspect_limit);

    if (type.horizontal == split_type::must && type.vertical == split_type::must
     || type.horizontal == split_type::can  && type.vertical == split_type::can
    ) {
        return toss_coin(random) ? split_h() : split_v();
    } else if (type.horizontal != split_type::none) {
        return split_h();
    } else if (type.vertical != split_type::none) {
        return split_v();
    }

    return std::make_tuple(r, r, false);
}

} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
