#pragma once

#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>

#include <random>

namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

using random_t = std::mt19937;

template <typename T = int>
using uniform_int_distribution_t = boost::random::uniform_int_distribution<T>;

template <typename T = double>
using normal_distribution_t = boost::random::normal_distribution<T>;

template <typename T = double>
using uniform_real_distribution_t = boost::random::uniform_real_distribution<T>;

} // namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
