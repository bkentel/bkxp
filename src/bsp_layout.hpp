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

    using room_gen_t = std::function<void (bklib::irect)>;

    void generate(random_t& gen, room_gen_t room_gen);

    param_t params() const noexcept;
private:
    std::unique_ptr<detail::bsp_layout_impl> impl_;
};

//--------------------------------------------------------------------------------------------------
std::pair<bklib::irect, bklib::irect>
split_vertical(bklib::irect r, int offset) noexcept;

//--------------------------------------------------------------------------------------------------
std::pair<bklib::irect, bklib::irect>
split_horizontal(bklib::irect r, int offset) noexcept;

//--------------------------------------------------------------------------------------------------
enum class split_type {
    none //!< Cannot be split.
  , can  //!< Can be split.
  , must //!< Must be split.
  , degenerate //!< Can be split, but cannot maintain a proper aspect ratio.
};

//--------------------------------------------------------------------------------------------------
struct splittable_ranges_t {
    split_type v {split_type::none};
    split_type h {split_type::none};
    bklib::range<int> v_range {0, 0};
    bklib::range<int> h_range {0, 0};
};

//--------------------------------------------------------------------------------------------------
splittable_ranges_t calculate_splittable_ranges(
    bklib::irect r, int min_w, int min_h, double aspect_limit = 0.0
);

//--------------------------------------------------------------------------------------------------
std::tuple<bklib::irect, bklib::irect, bool>
random_split(random_t& random, bklib::irect r
  , int min_w = std::numeric_limits<int>::min()
  , int max_w = std::numeric_limits<int>::max()
  , int min_h = std::numeric_limits<int>::min()
  , int max_h = std::numeric_limits<int>::max()
  , double aspect_limit = 0.0
);

//--------------------------------------------------------------------------------------------------
inline auto random_split(random_t& random, bklib::irect const r
  , int const min_w = std::numeric_limits<int>::min()
  , int const max_w = std::numeric_limits<int>::max()
  , int const min_h = std::numeric_limits<int>::min()
  , int const max_h = std::numeric_limits<int>::max()
    , bklib::aspect_ratio<int> const aspect_limit = {0, 1}
) {
    return random_split(random, r, min_w, max_w, min_h, max_h, aspect_limit.as<double>());
}

} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
