#pragma once

#include "input.hpp"

#include "bklib/string.hpp"
#include "bklib/flag_set.hpp"

#include <functional>
#include <memory>
#include <chrono>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
//! ABC for system OS interaction.
//--------------------------------------------------------------------------------------------------
class system {
public:
    virtual ~system();

    //----------------------------------------------------------------------------------------------
    virtual int client_width() const noexcept = 0;
    virtual int client_height() const noexcept = 0;

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    virtual void quit() = 0;

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    virtual bool is_running() const noexcept = 0;

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    virtual void do_events_nowait() = 0;
    virtual void do_events_wait() = 0;

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    virtual void delay(std::chrono::nanoseconds ns) = 0;

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    virtual key_mod_state current_key_mods() const noexcept = 0;
public:
    template <typename Rep, typename Period>
    void delay(std::chrono::duration<Rep, Period> dur) {
        delay(std::chrono::duration_cast<std::chrono::nanoseconds>(dur));
    }

    std::function<void (int, int)>                on_window_resize;
    std::function<void (bklib::utf8_string_view)> on_text_input;
    std::function<bool ()>                        on_request_quit;
    std::function<void (int)>                     on_key_up;
    std::function<void (int)>                     on_key_down;
    std::function<void (mouse_state)>             on_mouse_motion;
    std::function<void (mouse_state)>             on_mouse_move;
    std::function<void (mouse_state)>             on_mouse_scroll;
    std::function<void (mouse_button_state)>      on_mouse_button;
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
std::unique_ptr<system> make_system();

////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
