#pragma once

#include "bklib/assert.hpp"

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

enum class random_stream {
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
//! @pre lo <= hi
//! @pre abs(lo) + abs(hi) <= std::numeric_limits<int>::max()
//--------------------------------------------------------------------------------------------------
inline int random_range(random_t& random, int const lo, int const hi) noexcept {
    return boost::random::uniform_int_distribution<int> {lo, hi}(random);
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

} // namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
