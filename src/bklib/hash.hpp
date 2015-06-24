#pragma once

#include "bklib/string.hpp"
#include <functional>
#include <type_traits>
#include <cstddef>

namespace bklib {

// combining function from boost
template <typename T>
inline void hash_combine(std::size_t& seed, T const& val) noexcept {
    seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T, typename... Args>
inline void hash_combine(std::size_t& seed, T const& val, Args const&... args) noexcept {
    hash_combine(seed, val);
    hash_combine(seed, args...);
}

template <typename T, std::enable_if_t<!std::is_enum<T>::value>* = nullptr>
inline size_t hash_value(T const& val) noexcept {
    return std::hash<T>()(val);
}

template <typename T, std::enable_if_t<std::is_enum<T>::value>* = nullptr>
inline size_t hash_value(T const& val) noexcept {
    return std::hash<std::underlying_type_t<T>>()(static_cast<std::underlying_type_t<T>>(val));
}

template <>
inline size_t hash_value(bklib::utf8_string_view const& s) noexcept {
    return bklib::djb2_hash(s);
}

template <>
inline size_t hash_value(bklib::utf8_string const& s) noexcept {
    return bklib::djb2_hash(s);
}

template <typename T, typename... Args>
inline size_t hash_value(T const& val, Args const&... args) noexcept {
    auto seed = hash_value(val);
    hash_combine(seed, args...);
    return seed;
}

} //namespace bklib
