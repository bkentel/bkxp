#pragma once

#include <algorithm>
#include <iterator>
#include <type_traits>

namespace bklib {

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
template <typename Container, typename Predicate>
decltype(auto) find_if(Container&& c, Predicate&& p) {
    static_assert(!std::is_rvalue_reference<Container&&>::value, "bad reference type");

    using std::begin;
    using std::end;

    return std::find_if(begin(c), end(c), std::forward<Predicate>(p));
}

//--------------------------------------------------------------------------------------------------
//! @return a pointer to the first item matching the predicate @p p in the container @p c.
//! Otherwise, return nullptr.
//--------------------------------------------------------------------------------------------------
template <typename Container, typename Predicate>
inline decltype(auto) find_maybe(Container&& c, Predicate&& p) {
    static_assert(!std::is_rvalue_reference<Container&&>::value, "bad reference type");

    using std::begin;
    using std::end;

    auto const last = end(c);
    auto const it = std::find_if(begin(c), last, std::forward<Predicate>(p));

    return (it != last) ? std::addressof(*it) : nullptr;
}

} //namespace bklib
