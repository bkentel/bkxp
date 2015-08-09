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

        node_t(size_t const parent_index, bklib::irect const bounds) noexcept
          : bklib::irect(bounds), parent {static_cast<index_t>(parent_index)}
        {
        }

        node_t& set_children(size_t const i) noexcept {
            child = static_cast<index_t>(i);
            return *this;
        }

        bklib::irect bounds() const noexcept {
            return *this;
        }

        index_t parent = 0;
        index_t child  = 0;
    };

    //----------------------------------------------------------------------------------------------
    using param_t = bsp_layout::param_t;

    //----------------------------------------------------------------------------------------------
    bsp_layout_impl(bklib::irect const bounds, param_t const p)
      : p_ (p), width_ {bounds.width()}, height_ {bounds.height()}
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

        auto const result = random_split(gen,  nodes_[i].bounds()
          , p_.min_w, p_.max_edge_size, p_.min_h, p_.max_edge_size, p_.aspect);

        if (std::get<2>(result)) {
            nodes_[i].set_children(nodes_.size()).bounds();
            nodes_.emplace_back(i, std::get<0>(result));
            nodes_.emplace_back(i, std::get<1>(result));
        }
    }

    param_t params() const noexcept { return p_; }

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

//----------------------------------------------------------------------------------------------
bkrl::bsp_layout::param_t bkrl::bsp_layout::params() const noexcept {
    return impl_->params();
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
    int const w,     int const h
  , int const min_w, int const max_w
  , int const min_h, int const max_h
) noexcept {
    BK_PRECONDITION((max_w >= min_w) && (min_w >= 0));
    BK_PRECONDITION((max_h >= min_h) && (min_h >= 0));

    auto const set = [](int const n, int const lo, int const hi) noexcept {
        return (n < lo)
          ? std::make_tuple(split_type::none, bklib::make_range(0, 0))
          : std::make_tuple(
                (n < lo * 2) ? split_type::degenerate :
                (n > hi)     ? split_type::must
                             : split_type::can
              , bklib::make_range(lo, n - lo));
    };

    splittable_ranges_t result;

    std::tie(result.v, result.v_range, result.h, result.h_range) = std::tuple_cat(
        set(w, min_w, max_w), set(h, min_h, max_h));

    return result;
}

//--------------------------------------------------------------------------------------------------
bkrl::splittable_ranges_t
bkrl::calculate_splittable_ranges(
    int const w,     int const h
  , int const min_w, int const max_w
  , int const min_h, int const max_h
  , double const aspect_limit
) noexcept {
    BK_PRECONDITION((max_w >= min_w) && (min_w >= 0));
    BK_PRECONDITION((max_h >= min_h) && (min_h >= 0));
    BK_PRECONDITION(aspect_limit >= 1.0);

    auto const set = [](int const n, int const lo, int const hi, double const lo_a) noexcept {
        auto const type =
            (n < lo)       ? split_type::none :
            (n > hi)       ? split_type::must :
            (n < lo   * 2) ? split_type::degenerate :
            (n < lo_a * 2) ? split_type::degenerate : split_type::can;

        auto range = bklib::make_range(
            std::max(lo, bklib::ceil_to<int>(lo_a))
          , n - std::max(lo, bklib::floor_to<int>(lo_a)));

        if (range.lo <= range.hi) {
            return std::make_tuple(type, range);
        }

        range = bklib::make_range(hi, n - hi);
        if (range.lo <= range.hi) {
            return std::make_tuple(type, range);
        }

        range = bklib::make_range(lo, n - lo);
        if (range.lo <= range.hi) {
            return std::make_tuple(type, range);
        }

        if (lo <= n) {
            return std::make_tuple(type, bklib::make_range(lo, lo));
        }

        return std::make_tuple(type, bklib::make_range(0, 0));
    };

    auto const inv_aspect = 1.0 / aspect_limit;

    splittable_ranges_t result;

    std::tie(result.v, result.v_range, result.h, result.h_range) = std::tuple_cat(
        set(w, min_w, max_w, h * inv_aspect)
      , set(h, min_h, max_h, w * inv_aspect));


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
    auto const ranges = calculate_splittable_ranges(r, min_w, max_w, min_h, max_h, aspect_limit);

    auto const split = [&](auto const& f, bklib::range<int> range) {
        return std::tuple_cat(f(r, random_range(random, range)), std::make_tuple(true));
    };

    auto const split_h = [&] { return split(split_horizontal, ranges.h_range); };
    auto const split_v = [&] { return split(split_vertical, ranges.v_range); };
    auto const split_r = [&] { return toss_coin(random) ? split_h() : split_v(); };

    using st = split_type;
    #define BK_SPLIT_CASE(a, b) ((static_cast<unsigned>(a) << 2) | static_cast<unsigned>(b))

    switch (BK_SPLIT_CASE(ranges.v, ranges.h)) {
    case BK_SPLIT_CASE(st::none, st::none):
        break;
    case BK_SPLIT_CASE(st::none, st::can) :
        return split_h();
    case BK_SPLIT_CASE(st::none, st::must) :
        return split_h();
    case BK_SPLIT_CASE(st::none, st::degenerate) :
        if (r.height() - ranges.h_range.lo >= min_h) {
            return split_h();
        }
        break;
    case BK_SPLIT_CASE(st::can, st::none) :
        return split_v();
    case BK_SPLIT_CASE(st::can, st::can) :
        return split_r();
    case BK_SPLIT_CASE(st::can, st::must) :
        return split_h();
    case BK_SPLIT_CASE(st::can, st::degenerate) :
        return split_v();
    case BK_SPLIT_CASE(st::must, st::none) :
        return split_v();
    case BK_SPLIT_CASE(st::must, st::can) :
        return split_v();
    case BK_SPLIT_CASE(st::must, st::must) :
        return split_r();
    case BK_SPLIT_CASE(st::must, st::degenerate) :
        return split_v();
    case BK_SPLIT_CASE(st::degenerate, st::none) :
        if (r.width() - ranges.v_range.lo >= min_w) {
            return split_v();
        }
        break;
    case BK_SPLIT_CASE(st::degenerate, st::can) :
        return split_h();
    case BK_SPLIT_CASE(st::degenerate, st::must) :
        return split_h();
    case BK_SPLIT_CASE(st::degenerate, st::degenerate) : {
        auto const w = r.width();
        auto const h = r.height();
        bool const v_ok = w - ranges.h_range.lo >= min_w;
        bool const h_ok = h - ranges.v_range.lo >= min_h;

        if (v_ok && h_ok) {
            return (w > h) ? split_v() :
                   (w < h) ? split_h() : split_r();
        } else if (v_ok) {
            return split_v();
        } else if (h_ok) {
            return split_h();
        }

        break;
    }
    default:
        break;
    }

    #undef BK_SPLIT_CASE

    return std::make_tuple(r, r, false);
}
