#pragma once

#include <boost/utility/string_ref.hpp>

#include <string>
#include <array>
#include <algorithm>
#include <cstdint>

namespace bklib {
////////////////////////////////////////////////////////////////////////////////////////////////////
using utf8_string_view = boost::basic_string_ref<char, std::char_traits<char>>;
using utf8_string      = std::string;

////////////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
inline uint32_t djb2_hash(char const* first, char const* last) noexcept
{
    uint32_t hash = 5381;

    while (first != last) {
        hash = (hash * 33) ^ *first++;
    }

    return hash;
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
inline uint32_t djb2_hash(char const* str) noexcept
{
    uint32_t hash = 5381;
    while (auto const c = *str++) {
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
    return (str[i]) ? static_djb2_hash((hash * 33) ^ str[i], str, i + 1) : hash;
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
inline constexpr uint32_t static_djb2_hash(char const* const str) noexcept {
    return static_djb2_hash(5381, str, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
struct string_id_base {
    string_id_base(utf8_string_view string)
      : hash(djb2_hash(string))
    {
        size_t const size = std::min(hash_string.size() - 1, string.size() + 1);
        std::copy_n(string.data(), size, hash_string.data());
        hash_string[size] = '\0';
    }

    uint32_t             hash;
    std::array<char, 12> hash_string;
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename Tag>
struct string_id : string_id_base {
    using string_id_base::string_id_base;
};

} //namespace bklib
////////////////////////////////////////////////////////////////////////////////////////////////////