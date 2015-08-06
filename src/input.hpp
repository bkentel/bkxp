#pragma once

#include "bklib/flag_set.hpp"

#include <cstdint>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

enum class key_mod : uint32_t {
    none
  , lshift
  , rshift
  , lctrl
  , rctrl
  , lalt
  , ralt
  , lgui
  , rgui
  , num
  , caps
  , mode
  , ctrl
  , shift
  , alt
  , gui
};

using key_mod_state = bklib::flag_set<key_mod>;

enum class mouse_button : uint32_t {
    left = 1
  , middle
  , right
  , x1
  , x2
};

struct mouse_state {
    bool is_down(mouse_button const button) const noexcept {
        auto const shift = static_cast<uint32_t>(button) - 1;
        return !!(state & (1 << shift));
    }

    int x;  //!< Absolute x position
    int y;  //!< Absolute y position
    int dx; //!< Relative x motion
    int dy; //!< Relative y motion
    int sx; //!< Relative horizontal scroll
    int sy; //!< Relative vertical scroll

    uint32_t state;
    uint32_t timestamp;
};

struct mouse_button_state {
    bool was_pressed()  const noexcept { return type == 1; }
    bool was_released() const noexcept { return type == 0; }

    mouse_button button() const noexcept {
        return static_cast<mouse_button>(index);
    }

    bool was_click(int const n = 1) const noexcept {
        return static_cast<int>(clicks) == n;
    }

    int x;  //!< Absolute x position
    int y;  //!< Absolute y position

    uint32_t timestamp;

    uint8_t index;
    uint8_t type;
    uint8_t clicks;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
