#pragma once

#include "bklib/string.hpp"

namespace bkrl {
namespace id {
    using texture_source = bklib::string_id<struct tag_texture_source>;
    using texture        = bklib::string_id<struct tag_texture>;
    using terrain        = bklib::string_id<struct tag_terrain>;
} //namespace id

//----------------------------------------------------------------------------------------------
// small mix-in to allow hashing of types with an "id" member.
//----------------------------------------------------------------------------------------------
template <typename T>
struct id_hash_base {
    using argument_type = T;
    using result_type   = std::size_t;

    result_type operator()(argument_type const& arg) const noexcept {
        return arg.id.hash;
    }
};

//----------------------------------------------------------------------------------------------
// small mix-in to use for classes that have an id.
//----------------------------------------------------------------------------------------------
template <typename T>
struct id_base {
    using id_type = T;

    id_base() = default;
    explicit id_base(T id) : id(id) { }
    
    T id;
};

template <typename T>
bool operator==(id_base<T> const& lhs, id_base<T> const& rhs) noexcept {
    return lhs.id.hash == rhs.id.hash;
}

} //namespace bkrl
