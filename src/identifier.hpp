#pragma once

#include "bklib/utility.hpp"
#include <cstdint>

namespace bkrl {

using item_def_id          = bklib::tagged_value<uint32_t, struct tag_item_def_id>;
using item_instance_id     = bklib::tagged_value<uint32_t, struct tag_item_instance_id>;
using creature_def_id      = bklib::tagged_value<uint32_t, struct tag_creature_def_id>;
using creature_instance_id = bklib::tagged_value<uint32_t, struct tag_creature_instance_id>;

template <typename T>
inline auto idof(const T& value) noexcept {
    return typename T::id_t {std::hash<T> {}(value)};
}

} //namespace bkrl
