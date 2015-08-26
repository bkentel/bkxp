#pragma once

#include "bklib/assert.hpp"
#include "bklib/math.hpp"
#include "bklib/string.hpp"

#include <boost/predef.h>

#ifdef BOOST_ENDIAN_LITTLE_BYTE
#   define PCG_LITTLE_ENDIAN 1
#endif

#if BOOST_COMP_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wdate-time"
#endif

#if defined(BOOST_COMP_MSVC_AVAILABLE)
#   pragma warning( push )
#   pragma warning( disable : 4127 ) //condition is constant
#endif

#include <pcg_random.hpp>
#include <pcg_extras.hpp>

#if defined(BOOST_COMP_MSVC_AVAILABLE)
#   pragma warning( pop )
#endif

#if BOOST_COMP_CLANG
#   pragma clang diagnostic pop
#endif

#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>

#include <random>
#include <array>
#include <limits>

namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

using random_t = pcg32;

template <typename T = int>
using uniform_int_distribution_t = boost::random::uniform_int_distribution<T>;

template <typename T = double>
using normal_distribution_t = boost::random::normal_distribution<T>;

template <typename T = double>
using uniform_real_distribution_t = boost::random::uniform_real_distribution<T>;

enum class random_stream : int {
    substantive, item, creature,
    stream_count
};

//--------------------------------------------------------------------------------------------------
//! Bundle up a few random streams into one object.
//--------------------------------------------------------------------------------------------------
class random_state {
public:
    random_state(random_state const&) = delete;
    random_state(random_state&&) = default;
    random_state& operator=(random_state const&) = delete;
    random_state& operator=(random_state&&) = default;

    using stream_itype = decltype(std::declval<random_t>().stream());
    static constexpr auto const stream_count = static_cast<size_t>(random_stream::stream_count);

    random_state() {
        stream_itype stream {0};
        for (auto& g : generators_) {
            g.set_stream(stream++);
        }
    }

    template <typename Seed>
    void seed(Seed&& seed) {
        stream_itype stream {0};
        for (auto& g : generators_) {
            g.seed(std::forward<Seed>(seed));
            g.set_stream(stream++);
        }
    }

    //----------------------------------------------------------------------------------------------
    //! @pre stream != random_stream::stream_count
    //----------------------------------------------------------------------------------------------
    random_t& operator[](random_stream const stream) noexcept {
        return generators_[static_cast<size_t>(stream)];
    }
private:
    std::array<random_t, stream_count> generators_;
};

//--------------------------------------------------------------------------------------------------
//! @returns a uniform random value in [lo, hi].
//! @pre lo <= hi
//! @pre abs(hi - lo) <= std::numeric_limits<int>::max()
//--------------------------------------------------------------------------------------------------
inline int random_range(random_t& random, int const lo, int const hi) noexcept {
    return lo + boost::random::uniform_int_distribution<int> {0, hi - lo}(random);
}

inline int random_range(random_t& random, bklib::range<int> const r) noexcept {
    return random_range(random, r.lo, r.hi);
}

//--------------------------------------------------------------------------------------------------
//! @pre r is a proper rectangle
//! @pre r.width()  <= std::numeric_limits<int>::max()
//! @pre r.height() <= std::numeric_limits<int>::max()
//--------------------------------------------------------------------------------------------------
inline bklib::ipoint2 random_point(random_t& random, bklib::irect const r) noexcept {
    return {
        random_range(random, r.left, r.right - 1)
      , random_range(random, r.top,  r.bottom - 1)
    };
}

//--------------------------------------------------------------------------------------------------
//! @pre r is a proper rectangle
//! @pre 1 < r.width()  <= std::numeric_limits<int>::max()
//! @pre 1 < r.height() <= std::numeric_limits<int>::max()
//--------------------------------------------------------------------------------------------------
inline bklib::ipoint2 random_point_border(random_t& random, bklib::irect const r) noexcept {
    auto const w0 = r.width();
    auto const h0 = r.height();

    BK_PRECONDITION(w0 > 1 && h0 > 1);

    auto const w = w0 - 1;
    auto const h = h0 - 1;
    auto const n = random_range(random, 0,  2*w + 2*h - 1);

    auto const n0 =      w;
    auto const n1 = n0 + h;
    auto const n2 = n1 + w;

    return bklib::ipoint2 {r.left, r.top} + (
        (n <= n0) ? bklib::ivec2 {n,             0           }
      : (n <= n1) ? bklib::ivec2 {w,             n - n0      }
      : (n <= n2) ? bklib::ivec2 {w - (n - n1),  h           }
                  : bklib::ivec2 {0,             h - (n - n2)}
    );
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename Container>
inline decltype(auto) random_element(random_t& random, Container&& c) noexcept
{
    using std::begin;
    using std::end;

    auto const first = begin(c);
    auto const last = end(c);

    static_assert(std::is_same<
        std::random_access_iterator_tag
      , typename std::iterator_traits<decltype(first)>::iterator_category>::value, "");

    auto const size = std::distance(first, last);

    BK_PRECONDITION(size > 0 && size < std::numeric_limits<int>::max());

    return *(first + random_range(random, 0, static_cast<int>(size - 1)));
}

//--------------------------------------------------------------------------------------------------
//! @pre n > 0
//! @pre sides > 0
//! @pre n * sides + mod < std::numeric_limits<int>::max()
//--------------------------------------------------------------------------------------------------
inline int roll_dice(random_t& random, int const n, int const sides, int const mod = 0) noexcept {
    int result = 0;
    boost::random::uniform_int_distribution<int> dist {1, sides};

    for (int i = 0; i < n; ++i) {
        result += dist(random);
    }

    return result + mod;
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
inline bool toss_coin(random_t& random) noexcept {
    return !!random_range(random, 0, 1);
}

//--------------------------------------------------------------------------------------------------
//! @pre 0 <= x <= y
//--------------------------------------------------------------------------------------------------
inline bool x_in_y_chance(random_t& random, int const x, int const y) noexcept {
    BK_PRECONDITION(x >= 0);
    return random_range(random, 0, y - 1) < x;
}

template <typename T>
inline T random_gaussian(random_t& random, T const mean, T const variance) noexcept {
    static_assert(std::is_arithmetic<T>::value, "");

    return bklib::clamp_to<T>(std::round(normal_distribution_t<double>{
        static_cast<double>(mean), static_cast<double>(variance)
    }(random)));
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
struct random_integer {
    enum class distribution_type {
        dice, uniform, gaussian
    };

    struct dice_t {
        int number;
        int sides;
        int modifier;
    };

    struct uniform_range_t {
        int lo;
        int hi;
    };

    struct gaussian_t {
        int variance;
        int mean;
    };

    random_integer() noexcept = default;

    random_integer(dice_t const d) noexcept
      : data(d), type {distribution_type::dice}
    {
    }

    random_integer(uniform_range_t const u) noexcept
      : data(u), type {distribution_type::uniform}
    {
    }

    random_integer(gaussian_t const g) noexcept
      : data(g), type {distribution_type::gaussian}
    {
    }

    int generate(random_t& random) const noexcept {
        switch (type) {
        case distribution_type::uniform:
            return random_range(random, data.u.lo, data.u.hi);
        case distribution_type::dice:
            return roll_dice(random, data.d.number, data.d.sides, data.d.modifier);
        case distribution_type::gaussian:
            return random_gaussian(random, data.g.mean, data.g.variance);
        default:
            BK_UNREACHABLE;
        }

        BK_UNREACHABLE;
    }

    union data_t {
        data_t()                             noexcept : u {0, 0}  {}
        data_t(dice_t          const params) noexcept : d(params) {}
        data_t(uniform_range_t const params) noexcept : u(params) {}
        data_t(gaussian_t      const params) noexcept : g(params) {}

        dice_t          d;
        uniform_range_t u;
        gaussian_t      g;
    } data;

    distribution_type type = distribution_type::uniform;
};

random_integer make_random_integer(bklib::utf8_string_view str);

} // namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
