#pragma once

#include "bklib/string.hpp"
#include "bklib/flag_set.hpp"

#include <functional>
#include <memory>
#include <chrono>

namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail { class system_impl; }
namespace detail { class renderer_impl; }

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

class system {
    friend class detail::renderer_impl;
public:
    system();
    ~system();

    //----------------------------------------------------------------------------------------------
    int client_width() const;
    int client_height() const;

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    void quit();

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    bool is_running() const noexcept;

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    void do_events_nowait();
    void do_events_wait();

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    void delay(std::chrono::nanoseconds ns);

    template <typename Rep, typename Period>
    void delay(std::chrono::duration<Rep, Period> dur) {
        delay(std::chrono::duration_cast<std::chrono::nanoseconds>(dur));
    }

    mouse_state const& mouse_history(int i) const;

    key_mod_state current_key_mods() const;
public:
    //! Window resize (w, h)
    std::function<void (int, int)> on_window_resize;

    std::function<void (bklib::utf8_string_view)> on_text_input;
    std::function<bool ()>                        on_request_quit;
    std::function<void (int)>                     on_key_up;
    std::function<void (int)>                     on_key_down;

    //! Mouse motion delta (dx, dy)
    std::function<void (mouse_state)> on_mouse_motion;
    //! Mouse position update (x, y)
    std::function<void (mouse_state)> on_mouse_move;
    //! Mouse position update (x, y)
    std::function<void (mouse_state)> on_mouse_scroll;
    //! Mouse button update
    std::function<void (mouse_button_state)> on_mouse_button;
private:
    std::unique_ptr<detail::system_impl> impl_;
};

} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
