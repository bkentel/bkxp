#pragma once

#include "string.hpp"

#include <vector>
#include <type_traits>
#include <cstring>

namespace bklib {
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
//! Used for strict-aliasing-safe type punning.
//! @pre sizeof(from) and sizeof(to) must match.
//! @pre from and to must be trivially copyable.
//--------------------------------------------------------------------------------------------------
template <typename From, typename To>
inline decltype(auto) pseudo_cast(From const& from, To&& to) noexcept {
    using from_base_t = std::remove_reference_t<From>;
    using to_base_t   = std::remove_reference_t<To>;

    constexpr size_t const size_from = sizeof(from_base_t);
    constexpr size_t const size_to   = sizeof(to_base_t);

    static_assert(std::is_trivially_copyable<from_base_t>::value, "invalid from type");
    static_assert(std::is_trivially_copyable<to_base_t>::value, "invalid to type");
    static_assert(size_from == size_to, "size mismatch");

    std::memcpy(std::addressof(to), std::addressof(from), size_from);
    return std::forward<To>(to);
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
template <typename T, typename Tag>
struct tagged_value {
    using value_type = T;
    using tag_type   = Tag;

    constexpr explicit tagged_value(T const initial_value) : value {initial_value} { }

    T value;
};

template <typename T, typename Tag>
bool operator<(tagged_value<T, Tag> const& lhs, tagged_value<T, Tag> const& rhs) noexcept {
    return lhs.value < rhs.value;
}

template <typename T, typename Tag>
bool operator==(tagged_value<T, Tag> const& lhs, tagged_value<T, Tag> const& rhs) noexcept {
    return lhs.value == rhs.value;
}

template <typename T, typename Tag>
bool operator!=(tagged_value<T, Tag> const& lhs, tagged_value<T, Tag> const& rhs) noexcept {
    return !(lhs.value == rhs.value);
}

template <typename T, typename Tag>
constexpr T value_cast(tagged_value<T, Tag> const& n) noexcept {
    return n.value;
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
std::vector<char> read_file_to_buffer(utf8_string_view filename);

} // namespace bklib
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace std {
template <typename T, typename Tag> struct hash<bklib::tagged_value<T, Tag>> {
    inline size_t operator()(bklib::tagged_value<T, Tag> const& k) const noexcept {
        return k.value;
    }
};
} //namespace std
