#include "bsp_layout.hpp"

#include "bklib/assert.hpp"
#include "random.hpp"
#include "bklib/math.hpp"

#include <vector>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////
class bkrl::detail::bsp_layout_impl {
public:
    //----------------------------------------------------------------------------------------------
    struct node_t : bklib::irect {
        using index_t = uint16_t;

        node_t() = default;

        node_t(size_t const parent_index, bklib::irect const bounds)
          : bklib::irect(bounds), parent {static_cast<index_t>(parent_index)}
        {
        }

        node_t& set_children(size_t const i) {
            child = static_cast<index_t>(i);
            return *this;
        }

        bklib::irect bounds() const {
            return *this;
        }

        index_t parent = 0;
        index_t child  = 0;
    };

    //----------------------------------------------------------------------------------------------
    using param_t = bsp_layout::param_t;

    //----------------------------------------------------------------------------------------------
    bsp_layout_impl(bklib::irect bounds, param_t params)
      : p_ {params}, width_ {bounds.width()}, height_ {bounds.height()}
    {
        BK_PRECONDITION(width_ > 0);
        BK_PRECONDITION(height_ > 0);
        BK_PRECONDITION(p_.min_w > 0);
        BK_PRECONDITION(p_.min_h > 0);

        auto const size = static_cast<size_t>((width_ / p_.min_w) * (height_ / p_.min_h));

        nodes_.reserve(size);
        nodes_.emplace_back(0, bounds);
    }

    //----------------------------------------------------------------------------------------------
    void generate(random_t& gen, bsp_layout::room_gen_t& room_gen) {
        for (size_t first = 0, last = nodes_.size();
             first < last;
             first = last, last = nodes_.size())
        {
            for (auto i = first; i < last; ++i) {
                split(gen, i);
            }
        }

        for (auto const& n : nodes_) {
            if (!n.child) {
                room_gen(n.bounds());
            }
        }
    }

    //----------------------------------------------------------------------------------------------
    enum class split_type : int {
        none, vertical, horizonal, random
    };

    //----------------------------------------------------------------------------------------------
    split_type get_split_type(int const w, int const h) const noexcept {
        switch (bklib::combine_bool(w >= p_.min_w * 2, h >= p_.min_h * 2)) {
        default : return split_type::none;
        case 0  : return split_type::none;
        case 1  : return split_type::horizonal;
        case 2  : return split_type::vertical;
        case 3  : break;
        }

        // Both splits are possible; check the aspect ratio.
        if ((w > h) && ((p_.aspect.den * w) > (p_.aspect.num * h))) {
            return split_type::vertical;
        }

        if ((h > w) && ((p_.aspect.den * h) > (p_.aspect.num * w))) {
            return split_type::horizonal;
        }

        return split_type::random;
    }

    int get_split_point(random_t& gen, int const n, int const min) {
        BK_ASSERT(n >= min * 2);

        auto const split = static_cast<int>(std::lround(((2 + split_dist_(gen)) * n) / 4));
        return bklib::clamp(min, split, n - min);
    }

    //----------------------------------------------------------------------------------------------
    bool do_split(random_t& gen, int const w, int const h) {
        double const a = p_.max_edge_size;             // max size
        double const b = std::min(p_.min_w, p_.min_h); // min size
        double const p = p_.min_split_chance;          // min chance
        double const x = std::max(w, h);
        double const q = ((x - b)*(x + b)*(1.0 - p)) / ((a - b)*(a + b)) + p;

        return split_chance_(gen) < q;
    }

    //----------------------------------------------------------------------------------------------
    void split(random_t& gen, size_t const i) {
        auto const w = nodes_[i].width();
        auto const h = nodes_[i].height();

        if (i && !do_split(gen, w, h)) {
            return;
        }

        switch (get_split_type(w, h)) {
        default                    : break;
        case split_type::none      : break;
        case split_type::vertical  : split_vert(gen, i); break;
        case split_type::horizonal : split_hori(gen, i); break;
        case split_type::random    : split_rand(gen, i); break;
        }
    }

    //----------------------------------------------------------------------------------------------
    void split_rand(random_t& gen, size_t const i) {
        if (gen() & 1) {
            split_vert(gen, i);
        } else {
            split_hori(gen, i);
        }
    }

    //----------------------------------------------------------------------------------------------
    void split_hori(random_t& gen, size_t const i) {
        auto const n = nodes_[i].set_children(nodes_.size()).bounds();

        int const y0 = n.top;
        int const y1 = y0 + get_split_point(gen, n.height(), p_.min_h);
        int const y2 = n.bottom;

        nodes_.emplace_back(i, bklib::irect {n.left, y0, n.right, y1});
        nodes_.emplace_back(i, bklib::irect {n.left, y1, n.right, y2});
    }

    //----------------------------------------------------------------------------------------------
    void split_vert(random_t& gen, size_t const i) {
        auto const n = nodes_[i].set_children(nodes_.size()).bounds();

        int const x0 = n.left;
        int const x1 = x0 + get_split_point(gen, n.width(), p_.min_w);
        int const x2 = n.right;

        nodes_.emplace_back(i, bklib::irect {x0, n.top, x1, n.bottom});
        nodes_.emplace_back(i, bklib::irect {x1, n.top, x2, n.bottom});
    }

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------
    std::vector<node_t> nodes_;

    param_t p_;

    normal_distribution_t<>       split_dist_   {0, 1};
    uniform_real_distribution_t<> split_chance_ {0, 1};

    int width_;
    int height_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------
bkrl::bsp_layout::~bsp_layout() = default;

//----------------------------------------------------------------------------------------------
bkrl::bsp_layout::bsp_layout(bklib::irect bounds, param_t params)
  : impl_ {std::make_unique<detail::bsp_layout_impl>(bounds, params)}
{
}

//----------------------------------------------------------------------------------------------
bkrl::bsp_layout::bsp_layout(bklib::irect bounds)
  : bsp_layout {bounds, param_t {}}
{
}

//----------------------------------------------------------------------------------------------
void bkrl::bsp_layout::generate(random_t& gen, room_gen_t room_gen)
{
    impl_->generate(gen, room_gen);
}

//--------------------------------------------------------------------------------------------------
std::pair<bklib::irect, bklib::irect>
bkrl::split_vertical(bklib::irect const r, int const offset) noexcept
{
    BK_PRECONDITION(!!r);
    BK_PRECONDITION(offset > 0);
    BK_PRECONDITION(offset < r.width());

    return std::make_pair(
        bklib::irect {r.left,          r.top, r.left + offset, r.bottom}
      , bklib::irect {r.left + offset, r.top, r.right,         r.bottom}
    );
}

//--------------------------------------------------------------------------------------------------
std::pair<bklib::irect, bklib::irect>
bkrl::split_horizontal(bklib::irect const r, int const offset) noexcept
{
    BK_PRECONDITION(!!r);
    BK_PRECONDITION(offset > 0);
    BK_PRECONDITION(offset < r.height());

    return std::make_pair(
        bklib::irect {r.left, r.top,          r.right, r.top + offset}
      , bklib::irect {r.left, r.top + offset, r.right, r.bottom}
    );
}

//--------------------------------------------------------------------------------------------------
bkrl::splittable_ranges_t
bkrl::calculate_splittable_ranges(
    bklib::irect const r
  , int const min_w, int const min_h
  , double const aspect_limit
) {
    using bklib::range;
    using bklib::make_range;
    using bklib::ceil_to;
    using bklib::floor_to;

    BK_PRECONDITION(aspect_limit >= 0.0);
    BK_PRECONDITION(min_w >= 0);
    BK_PRECONDITION(min_h >= 0);

    splittable_ranges_t result;

    //
    // the case with no aspect ratio limit.
    //
    if (aspect_limit <= 0.0) {
        auto const range_h = bklib::make_range(min_h, r.height() - min_h);
        auto const range_w = bklib::make_range(min_w, r.width() - min_w);

        if (range_h.size()) {
            result.h = split_type::can;
            result.h_range = range_h;
        }

        if (range_w.size()) {
            result.v = split_type::can;
            result.v_range = range_w;
        }

        return result;
    }

    //
    // the more complicated case with an enforced aspect ratio limit.
    //
    auto const aspect = aspect_limit;
    auto const inv_aspect = 1.0 / aspect_limit;

    auto const w = static_cast<double>(r.width());
    auto const h = static_cast<double>(r.height());

    auto const aspect_min_h = w * inv_aspect;
    auto const aspect_min_w = h * inv_aspect;
    auto const aspect_max_h = w * aspect;
    auto const aspect_max_w = h * aspect;

    auto const range_h_min = std::max<double>(min_h, aspect_min_h);
    auto const range_h_max = std::min<double>(h - aspect_min_h, aspect_max_h);

    auto const range_w_min = std::max<double>(min_w, aspect_min_w);
    auto const range_w_max = std::min<double>(w - aspect_min_w, aspect_max_w);

    if (range_h_min > h) {
        result.v = split_type::must; //have to split vertically
        result.v_range = make_range(ceil_to<int>(range_w_min), floor_to<int>(range_w_max));
    } else if (range_h_min > range_h_max) {
        if (range_h_min < h) {
            result.h = split_type::degenerate; //can split horizontally , but not well
            result.h_range = make_range(ceil_to<int>(range_h_min), ceil_to<int>(range_h_min));
        } else {
            result.h = split_type::none; //can't split horizontally
        }
    } else {
        result.h = split_type::can; //can split horizontally
        result.h_range = make_range(ceil_to<int>(range_h_min), floor_to<int>(range_h_max));
    }

    if (range_w_min > w) {
        result.h = split_type::must; //have to split vertically
        result.h_range = make_range(ceil_to<int>(range_h_min), floor_to<int>(range_h_max));
    } else if (range_w_min > range_w_max) {
        if (range_w_min < w) {
            result.v = split_type::degenerate; //can split horizontally , but not well
            result.v_range = make_range(ceil_to<int>(range_w_min), ceil_to<int>(range_w_min));
        } else {
            result.v = split_type::none; //can't split horizontally
        }
    } else {
        result.v = split_type::can; //can split horizontally
        result.v_range = make_range(ceil_to<int>(range_w_min), floor_to<int>(range_w_max));
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
std::tuple<bklib::irect, bklib::irect, bool>
bkrl::random_split(
    random_t& random, bklib::irect const r
  , int const min_w, int const max_w
  , int const min_h, int const max_h
  , double const aspect_limit
) {
    auto const ranges = calculate_splittable_ranges(r, min_w, min_h, aspect_limit);

    auto const split = [&](auto const& f, bklib::range<int> range) {
        return std::tuple_cat(f(r, random_range(random, range)), std::make_tuple(true));
    };

    auto const split_h = [&] { return split(split_horizontal, ranges.h_range); };
    auto const split_v = [&] { return split(split_vertical, ranges.v_range); };

    if (ranges.h == ranges.v) {
        auto const w = r.width();
        auto const h = r.height();

        if (ranges.h == split_type::degenerate && w != h) {
            return h > w ? split_h() : split_v();
        }

        if (ranges.h != split_type::none) {
            return (h > max_h) ? split_h() :
                   (w > max_w) ? split_v() :
                   toss_coin(random) ? split_h() : split_v();
        }
    } else if (ranges.h != split_type::none && ranges.h != split_type::degenerate) {
        return split_h();
    } else if (ranges.v != split_type::none && ranges.v != split_type::degenerate) {
        return split_v();
    }

    return std::make_tuple(r, r, false);
}
