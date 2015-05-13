#pragma once

#include "bklib/string.hpp"

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
public:
    std::function<void (bklib::utf8_string_view)> on_text_input;
    std::function<bool ()>                        on_request_quit;
    std::function<void (int)>                     on_key_up;
    std::function<void (int)>                     on_key_down;
private:
    std::unique_ptr<detail::system_impl> impl_;
};

} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
