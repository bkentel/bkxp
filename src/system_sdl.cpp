#include "system_sdl.hpp"

namespace {

//----------------------------------------------------------------------------------------------
bkrl::sdl_window init_sdl_window()
{
    auto const result = SDL_CreateWindow("BKRL"
        , SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
        , 1024, 768, SDL_WINDOW_RESIZABLE);

    if (!result) {
        throw "TODO";
    }

    return bkrl::sdl_window(result, &SDL_DestroyWindow);
}

//----------------------------------------------------------------------------------------------
bkrl::sdl_renderer init_sdl_renderer(bkrl::sdl_window const& w)
{
    auto const result = SDL_CreateRenderer(w.get(), -1, SDL_RENDERER_ACCELERATED);
    
    if (!result) {
        throw "TODO";
    }

    return bkrl::sdl_renderer(result, &SDL_DestroyRenderer);
}

} //namespace

//----------------------------------------------------------------------------------------------
bkrl::detail::system_impl::system_impl(system* sys)
  : sys_      {sys}
  , window_   {init_sdl_window()}
  , renderer_ {init_sdl_renderer(window_)}
{
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::system_impl::do_events(bool const wait)
{
    auto const function = wait ? &SDL_WaitEvent : &SDL_PollEvent;
    
    SDL_Event event;
    while (function(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN :
            break;
        case SDL_KEYUP :
            break;
        case SDL_TEXTEDITING :
            break;
        case SDL_TEXTINPUT :
            if (sys_->on_text_input) {
                sys_->on_text_input(event.text.text);
            }
            break;
        default :
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::system_impl::delay(std::chrono::nanoseconds const ns)
{
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::system_impl::handle_keyboard_(SDL_KeyboardEvent const& event)
{
}
