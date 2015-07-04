#pragma once

#include "identifier.hpp"
#include "definitions.hpp"
#include "direction.hpp"

#include "bklib/string.hpp"

#include <array>
#include <cstdint>

namespace bkrl {

using color4 = std::array<uint8_t, 4>;

struct color_def {
    using id_type = color_def_id;

    explicit color_def(bklib::utf8_string def_id_string)
      : id {bklib::djb2_hash(def_id_string)}
      , id_string {std::move(def_id_string)}
    {
    }

    color_def_id       id;
    bklib::utf8_string id_string;
    bklib::utf8_string short_name;
    color4             color;
};

inline color_def_id get_id(color_def const& def) noexcept {
    return def.id;
}

using color_dictionary = bklib::dictionary<color_def>;

void load_definitions(color_dictionary& dic, bklib::utf8_string_view data, detail::load_from_string_t);

} //namespace bkrl
