#pragma once
#if !defined(BK_NO_PCH)

#include <boost/predef.h>

#if !defined(BK_NO_UNIT_TESTS)
#   include <catch/catch.hpp>
#endif

#if !defined(BK_NO_SDL)
#   if defined(BOOST_OS_WINDOWS)
#       define SDL_MAIN_HANDLED
#   endif
#   include <SDL2/SDL.h>
#endif

#include "external/format.h"

#include <boost/exception/all.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/optional.hpp>

#include <string>
#include <vector>
#include <array>

#include <algorithm>
#include <exception>
#include <memory>
#include <functional>
#include <chrono>
#include <random>
#include <type_traits>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstddef>

#endif //!defined(BK_NO_PCH)
