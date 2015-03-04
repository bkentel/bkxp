#pragma once

#include "bklib/string.hpp"

#include <functional>
#include <memory>
#include <chrono>

namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail { class system_impl; }

class system {
public:
    system();
    ~system();

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    bool is_running() const noexcept;

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    void do_events(bool wait = false);
    
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
private:
    std::unique_ptr<detail::system_impl> impl_;
};

} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
