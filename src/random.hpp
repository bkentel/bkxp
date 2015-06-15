#pragma once

#include "bklib/assert.hpp"

#include <boost/predef.h>

#ifdef BOOST_ENDIAN_LITTLE_BYTE
#   define PCG_LITTLE_ENDIAN 1
#endif

#pragma warning( push )
#pragma warning( disable : 4127 ) //condition is constant
#include <pcg_random.hpp>
#include <pcg_extras.hpp>
#pragma warning( pop )

#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>

#include <random>
#include <array>

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

class random_state {
public:
    random_state() {
        int stream = 0;
        for (auto& g : generators_) {
            g.set_stream(stream++);
        }
    }

    template <typename Seed>
    void seed(Seed&& seed) {
        int stream = 0;
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
    std::array<random_t, static_cast<size_t>(random_stream::stream_count)> generators_;
};

//--------------------------------------------------------------------------------------------------
//! @pre lo <= hi
//--------------------------------------------------------------------------------------------------
inline int random_range(random_t& random, int const lo, int const hi) noexcept {
    return random(hi - lo + 1) + lo;
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

    for (int i = 0; i < n; ++i) {
        result += (random(sides) + 1);
    }

    return static_cast<int>(result) + mod;
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
inline bool toss_coin(random_t& random) noexcept {
    return !!random(2);
}

//--------------------------------------------------------------------------------------------------
//! @pre x <= y
//--------------------------------------------------------------------------------------------------
inline bool x_in_y_chance(random_t& random, int const x, int const y) noexcept {
    BK_PRECONDITION(x <= y);

    return static_cast<int>(random(y)) <= x;
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
