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
    auto const diff = static_cast<random_t::result_type>(hi - lo + 1);

    //Check for overflow
    BK_PRECONDITION(lo <= hi);
    BK_PRECONDITION(static_cast<random_t::result_type>(std::numeric_limits<int>::max()) >= diff);

    return static_cast<int>(random(diff)) + lo;
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
    BK_PRECONDITION(n > 0);
    BK_PRECONDITION(sides > 0);
    BK_PRECONDITION(n * sides + mod <= std::numeric_limits<int>::max()); //TODO

    random_t::result_type result = 0;
    auto const s = static_cast<random_t::result_type>(sides);

    for (int i = 0; i < n; ++i) {
        result += (random(s) + random_t::result_type {1});
    }

    return static_cast<int>(result) + mod;
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
inline bool toss_coin(random_t& random) noexcept {
    return !!random(2u);
}

//--------------------------------------------------------------------------------------------------
//! @pre 0 <= x <= y
//--------------------------------------------------------------------------------------------------
inline bool x_in_y_chance(random_t& random, int const x, int const y) noexcept {
    BK_PRECONDITION(x >= 0 && x <= y);

    return static_cast<int>(random(static_cast<random_t::result_type>(y))) <= x;
}

//--------------------------------------------------------------------------------------------------
//! @pre c is not empty.
//--------------------------------------------------------------------------------------------------
template <typename Container>
decltype(auto) choose_random_element(random_t& random, Container&& c) {
    BK_PRECONDITION(!c.empty());

    return *std::next(begin(c), random(c.size()));
}

} // namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
