#pragma once

#include <algorithm>
#include <iterator>
#include <type_traits>

namespace bklib {

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
template <typename C0, typename C1>
inline bool equal(C0 const& c0, C1 const& c1) {
    using std::begin;
    using std::end;

    return std::equal(begin(c0), end(c0), begin(c1), end(c1));
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
template <typename Container, typename Predicate>
inline bool all_of(Container&& c, Predicate&& p) {
    using std::begin;
    using std::end;

    return std::all_of(begin(c), end(c), std::forward<Predicate>(p));
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
template <typename Container, typename Predicate>
inline decltype(auto) find_if(Container&& c, Predicate&& p) {
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
