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
namespace detail { class terrain_dictionary_impl; }

class terrain_dictionary {
public:
    ~terrain_dictionary();
    explicit terrain_dictionary(bklib::utf8_string_view filename);
    terrain_def const* find(terrain_entry entry) const;
private:
    std::unique_ptr<detail::terrain_dictionary_impl> impl_;
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
struct door : terrain_data_base {
    enum class state {
        closed, open
    };

    door() noexcept;
    explicit door(terrain_entry const& entry) noexcept;

    bool set_open_close(state s) noexcept;
    bool open() noexcept;
    bool close() noexcept;

    bool is_open() const noexcept;
    bool is_closed() const noexcept;

    uint64_t to_data() const noexcept;

    struct data_t {
        uint8_t flags;
        uint8_t unused[7];
    } data;
};

//--------------------------------------------------------------------------------------------------
//! Check whether @p ter refers to a door with @p state.
//--------------------------------------------------------------------------------------------------
bool is_door(terrain_entry const& ter, door::state state) noexcept;

//--------------------------------------------------------------------------------------------------
//! Return a predicate which tests for doors matching state
//--------------------------------------------------------------------------------------------------
inline decltype(auto) find_door(door::state const state) noexcept {
    return [state](terrain_entry const& ter) noexcept {
        return is_door(ter, state);
    };
}

} //namespace bkrl
