#pragma once

#include "bklib/string.hpp"
#include "bklib/hash.hpp"
#include <memory>
#include <cstdint>

namespace bkrl {

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
enum class terrain_type : int16_t {
    empty
  , rock
  , stair
  , floor
  , wall
  , door
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
struct terrain_def {
    bklib::utf8_string name;
    bklib::utf8_string description;
    bklib::utf8_string symbol;
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
struct terrain_entry {
    terrain_type type;
    uint16_t     variant;
};

inline bool operator==(terrain_entry const lhs, terrain_entry const rhs) noexcept {
    return (lhs.type == rhs.type) && (lhs.variant == rhs.variant);
}

inline bool operator!=(terrain_entry const lhs, terrain_entry const rhs) noexcept {
    return !(lhs == rhs);
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
struct terrain_render_data {
    int16_t index;
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
namespace detail { class terrain_dictionary_impl; }

class terrain_dictionary {
public:
    ~terrain_dictionary();
    explicit terrain_dictionary(bklib::utf8_string_view filename);
    terrain_def const* find(terrain_entry entry) const;
private:
    std::unique_ptr<detail::terrain_dictionary_impl> impl_;
};

} //namespace bkrl

namespace std {
template <> struct hash<bkrl::terrain_entry> {
    size_t operator()(const bkrl::terrain_entry& k) const noexcept {
        return bklib::hash_value(k.type, k.variant);
    }
};
} //namespace std
