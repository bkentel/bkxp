#pragma once

#include "input.hpp"

#include "bklib/string.hpp"
#include "bklib/flag_set.hpp"

#include <functional>
#include <memory>
#include <chrono>

namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail { class system_impl; }
namespace detail { class renderer_impl; }

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
