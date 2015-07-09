#pragma once

#include "bklib/utility.hpp"
#include <cstdint>

namespace bkrl {

template <typename Tag>
using id_t = bklib::tagged_value<uint32_t, Tag>;

using def_tag_id           = id_t<struct tag_def_tag_id>;
using color_def_id         = id_t<struct tag_color_def_id>;
using item_def_id          = id_t<struct tag_item_def_id>;
using item_instance_id     = id_t<struct tag_item_instance_id>;
using creature_def_id      = id_t<struct tag_creature_def_id>;
using creature_instance_id = id_t<struct tag_creature_instance_id>;

template <typename Tag>
inline id_t<Tag> get_id(id_t<Tag> const id) noexcept {
    return id;
}

} //namespace bkrl
