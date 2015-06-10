#pragma once

#include "bklib/string.hpp"
#include "bklib/hash.hpp"
#include <memory>
#include <cstdint>

namespace bkrl {

//--------------------------------------------------------------------------------------------------
//! Basic terrain type.
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
//! Detailed definition for a given terrain type.
//--------------------------------------------------------------------------------------------------
struct terrain_def {
    bklib::utf8_string name;
    bklib::utf8_string description;
    bklib::utf8_string symbol;
};

enum class terrain_flags : uint32_t {
    none
};

struct terrain_data_base { };

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
struct terrain_entry {
    //!< Terrain specific data. e.g. the data needed to track the state of a door.
    uint64_t data;
    
    //!<
    terrain_flags flags;   //!< Current flags
    terrain_type  type;
    uint16_t      variant; //!< Terrain specific variant

    template <typename T
        , std::enable_if_t<std::is_base_of<terrain_data_base, T>::value>* = nullptr>
    terrain_entry& operator=(T const& other) {
        data = other.to_data();
        return *this;
    }
};

static_assert(std::is_pod<terrain_entry>::value, "");

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

struct door : terrain_data_base {
    explicit door(terrain_entry const& entry)
      : data {entry.data}
    {
        //BK_PRECONDITION(entry.type == terrain_type::door);
    }

    bool open() noexcept {
        if (data != 1) {
            data = 1;
            return true;
        }

        return false;
    }

    bool is_open() const noexcept {
        return data == 1;
    }

    bool close() noexcept {
        if (data != 0) {
            data = 0;
            return true;
        }

        return false;
    }

    bool is_closed() const noexcept {
        return !is_open();
    }

    uint64_t to_data() const noexcept {
        return data;
    }

    uint64_t data;
};



} //namespace bkrl

namespace std {
template <> struct hash<bkrl::terrain_entry> {
    size_t operator()(const bkrl::terrain_entry& k) const noexcept {
        return bklib::hash_value(k.type, k.variant);
    }
};
} //namespace std
