#pragma once

#include "identifier.hpp"
#include "definitions.hpp"

#include "bklib/string.hpp"

#include <array>

namespace bklib { template <typename T> class dictionary; }

namespace bkrl {

struct color_def;
using color_dictionary = bklib::dictionary<color_def>;
using color4 = std::array<uint8_t, 4>;

struct color_def {
    using id_type = def_id_t<tag_color>;

    explicit color_def(bklib::utf8_string identifier)
      : id {identifier}
      , id_string {std::move(identifier)}
    {
    }

    id_type            id;
    bklib::utf8_string id_string;
    bklib::utf8_string short_name;
    color4             color;
};

inline auto const& get_id(color_def const& def) noexcept {
    return def.id;
}

} //namespace bkrl
