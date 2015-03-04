#pragma once

#include "system.hpp"
#include <SDL2/SDL.h>
#include <memory>

namespace bkrl {

using sdl_window   = std::unique_ptr<SDL_Window,   decltype(&SDL_DestroyWindow)>;
using sdl_renderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;

//----------------------------------------------------------------------------------------------
struct sdl_state {
    sdl_state() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw "TODO";
        }
    }

    ~sdl_state() {
        SDL_Quit();
    }
};

//----------------------------------------------------------------------------------------------
class detail::system_impl {
public:
    system_impl(system* sys);

    bool is_running() const noexcept {
        return is_running_;
    }

    void do_events(bool wait);
    
    void delay(std::chrono::nanoseconds ns);
private:
    void handle_keyboard_(SDL_KeyboardEvent const& event);

    system*      sys_ = nullptr;
    sdl_state    sdl_;
    sdl_window   window_;
    sdl_renderer renderer_;
    bool         is_running_ = true;
};

} // namespace bkrl
