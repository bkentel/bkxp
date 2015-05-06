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

        node_t(size_t const parent, bklib::irect const bounds)
          : bklib::irect(bounds), parent {static_cast<index_t>(parent)}
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
        nodes_.reserve((width_ / p_.min_w) * (height_ / p_.min_h));
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// UNIT TESTS
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

TEST_CASE("bsp_layout", "[bsp_layout][mapgen]") {
    bkrl::random_t gen;
    bkrl::detail::bsp_layout_impl layout(bklib::irect {0, 0, 100, 100}, bkrl::bsp_layout::param_t {});
    auto const& p = layout.p_;

    using type = bkrl::detail::bsp_layout_impl::split_type;

    auto const ok_w = p.min_w*2;
    auto const ok_h = p.min_h*2;

    SECTION("Simple split scenarios") {
        REQUIRE(layout.get_split_type(ok_w,     ok_h)     == type::random);
        REQUIRE(layout.get_split_type(ok_w - 1, ok_h)     == type::horizonal);
        REQUIRE(layout.get_split_type(ok_w,     ok_h - 1) == type::vertical);
        REQUIRE(layout.get_split_type(ok_w - 1, ok_h - 1) == type::none);
    }

    SECTION("Aspect ratio split scenarios") {
        auto const size = std::max(ok_w, ok_h);
        auto const aspect_w = size * p.aspect.num;
        auto const aspect_h = size * p.aspect.den;

        REQUIRE(aspect_w >= ok_w);
        REQUIRE(aspect_h >= ok_h);
        REQUIRE(aspect_w >= aspect_h);

        REQUIRE(layout.get_split_type(aspect_w,     aspect_h)     == type::random);
        REQUIRE(layout.get_split_type(aspect_w + 1, aspect_h)     == type::horizonal);
        REQUIRE(layout.get_split_type(aspect_h,     aspect_w + 1) == type::vertical);
    }
}

#endif // BK_NO_UNIT_TESTS
