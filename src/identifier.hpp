#pragma once

#include "bklib/utility.hpp"

namespace bkrl {
//===--------------------------------------------------------------------------------------------===
//                          Tags used to discriminate identifiers.
//===--------------------------------------------------------------------------------------------===
struct tag_creature {};
struct tag_item {};
struct tag_color {};
struct tag_string_tag {};

//--------------------------------------------------------------------------------------------------
//! Simple tagged value (integer) used as unique ids.
//--------------------------------------------------------------------------------------------------
template <typename Tag, typename T = std::uint32_t>
using instance_id_t = bklib::tagged_value<Tag, T>;

//--------------------------------------------------------------------------------------------------
//! Tagged string hash with a portion of the original string.
//--------------------------------------------------------------------------------------------------
template <typename Tag, typename T = std::uint32_t>
using def_id_t = bklib::hash_id<Tag, T>;

//--------------------------------------------------------------------------------------------------
//! Get the id type associated with a given type.
//--------------------------------------------------------------------------------------------------
template <typename Tag, typename T>
inline auto get_id(instance_id_t<Tag, T> const id) noexcept {
    return id;
}

template <typename Tag, typename T>
inline auto const& get_id(def_id_t<Tag, T> const& id) noexcept {
    return id;
}

namespace detail {
bklib::utf8_string to_string(uint32_t hash, bklib::utf8_string_view str);
} // namespace detail

template <typename Tag, size_t Extra>
bklib::utf8_string to_string(bklib::hash_id<Tag, uint32_t, Extra> const& id)
{
    return detail::to_string(
        static_cast<uint32_t>(id)
      , bklib::utf8_string_view {id.c_str(), Extra}
    );
}

} //namespace bkrl
