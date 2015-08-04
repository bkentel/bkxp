#pragma once

#include <boost/utility/string_ref.hpp>

#include <string>
#include <algorithm>
#include <cstdint>

namespace bklib {
////////////////////////////////////////////////////////////////////////////////////////////////////
using utf8_string_view = boost::basic_string_ref<char, std::char_traits<char>>;
using utf8_string      = std::string;

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <size_t N>
inline constexpr utf8_string_view make_string_view(char const (&str)[N]) noexcept {
    static_assert(N > 0, "");
    return bklib::utf8_string_view {str, N - 1};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
inline uint32_t djb2_hash(char const* first, char const* last) noexcept
{
    uint32_t hash = 5381;

    while (first != last) {
        hash = (hash * 33) ^ static_cast<unsigned char>(*first++);
    }

    return hash;
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
inline uint32_t djb2_hash(char const* str) noexcept
{
    uint32_t hash = 5381;
    while (auto const c = static_cast<unsigned char>(*str++)) {
        hash = (hash * 33) ^ c;
    }
    return hash;
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
inline uint32_t djb2_hash(utf8_string_view const& str) noexcept {
    return djb2_hash(str.begin(), str.end());
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
inline constexpr uint32_t
static_djb2_hash(uint32_t const hash, char const* const str, size_t const i) noexcept {
    //return (str[i]) ? static_djb2_hash((hash * 33) ^ str[i], str, i + 1) : hash;
    return (str[i]) ? static_djb2_hash(
        static_cast<uint32_t>(uint64_t {0xFFFFFFFF} & (uint64_t {hash} * uint64_t {33}))
      ^ static_cast<unsigned char>(str[i]), str, i + 1) : hash;
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
inline constexpr uint32_t static_djb2_hash(char const* const str) noexcept {
    return static_djb2_hash(5381, str, 0);
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
inline constexpr uint32_t static_djb2_hash(utf8_string_view const str) noexcept {
    return static_djb2_hash(str.data());
}

inline namespace literals {
inline constexpr std::uint32_t operator""_hash(char const* const str, std::size_t) noexcept {
    return bklib::static_djb2_hash(str);
}
}

//--------------------------------------------------------------------------------------------------
//! Convert between an integer and an alphanumeric identifier.
//--------------------------------------------------------------------------------------------------
struct alphanum_id {
    enum alphanum_range : int {
        range0 = 0
      , range1 = range0 + ('z' - 'a') + 1
      , range2 = range1 + ('Z' - 'A') + 1
      , range3 = range2 + ('9' - '0') + 1
    };

    static constexpr int to_index(int const c) noexcept {
        return (c >= 'a' && c <= 'z') ? (range0 + (c - 'a'))
             : (c >= 'A' && c <= 'Z') ? (range1 + (c - 'A'))
             : (c >= '0' && c <= '9') ? (range2 + (c - '0'))
             : -1;
    }

    static constexpr int to_char(int const i) noexcept {
        return (i < range0) ? -1
             : (i < range1) ? (('a' + i) - range0)
             : (i < range2) ? (('A' + i) - range1)
             : (i < range3) ? (('0' + i) - range2)
             : -1;
    }
};

//--------------------------------------------------------------------------------------------------
//! Get the English ordinal suffix for an integer.
//--------------------------------------------------------------------------------------------------
constexpr inline utf8_string_view oridinal_suffix(int const i) noexcept {
    return (i < 0)            ? make_string_view("th")
         : (i % 20 > 10 && i % 20 < 20) ? make_string_view("th")
         : (i % 10 == 1)      ? make_string_view("st")
         : (i % 10 == 2)      ? make_string_view("nd")
         : (i % 10 == 3)      ? make_string_view("rd")
         :                      make_string_view("th");
}

} //namespace bklib
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace std {
template <> struct hash<bklib::utf8_string_view> {
    inline size_t operator()(bklib::utf8_string_view const& k) const noexcept {
        return bklib::djb2_hash(k);
    }
};
} //namespace std
