#pragma once

#include "random.hpp"
#include "math.hpp"

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
    explicit bsp_layout(bklib::irect bounds, param_t params = param_t {});

    using room_gen_t = std::function<void (bklib::irect const&)>;

    void generate(random_t& gen, room_gen_t room_gen);
private:
    std::unique_ptr<detail::bsp_layout_impl> impl_;
};

} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
