#include "system_sdl.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
// sdl_state
////////////////////////////////////////////////////////////////////////////////////////////////////
bkrl::sdl_state::sdl_state()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
          << boost::errinfo_api_function {"SDL_Init"});
    }
}

bkrl::sdl_state::~sdl_state()
{
    SDL_Quit();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// sdl_window
////////////////////////////////////////////////////////////////////////////////////////////////////
bkrl::sdl_window::handle_t bkrl::sdl_window::create_()
{
    auto const result = SDL_CreateWindow("BKRL"
        , SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
        , 1024, 768, SDL_WINDOW_RESIZABLE);

    if (!result) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
          << boost::errinfo_api_function {"SDL_CreateWindow"});
    }

    return handle_t(result, &SDL_DestroyWindow);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// sdl renderer_impl
////////////////////////////////////////////////////////////////////////////////////////////////////
bkrl::detail::renderer_impl::handle_t bkrl::detail::renderer_impl::create_(sdl_window const& w)
{
    auto const result = SDL_CreateRenderer(w.handle(), -1, SDL_RENDERER_ACCELERATED);
    
    if (!result) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
          << boost::errinfo_api_function {"SDL_CreateRenderer"});
    }

    return handle_t(result, &SDL_DestroyRenderer);
}

namespace {
bkrl::tilemap make_tilemap(bkrl::sdl_texture const& tex) {
    auto const size = tex.get_size(tex);
    return {18, 18, size.first, size.second};
}
} //namespace

//----------------------------------------------------------------------------------------------
bkrl::detail::renderer_impl::renderer_impl(system& sys)
  : handle_(create_(sys.impl_->window_))
  , tile_texture_(*this, "data/tiles.bmp")
  , tile_tilemap_(make_tilemap(tile_texture_))
{
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::renderer_impl::clear()
{
    if (SDL_SetRenderDrawColor(handle(), 255, 0, 0, 255)) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
          << boost::errinfo_api_function {"SDL_SetRenderDrawColor"});
    }

    if (SDL_RenderClear(handle())) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
          << boost::errinfo_api_function {"SDL_RenderClear"});
    }
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::renderer_impl::present() {
    SDL_RenderPresent(handle());
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::renderer_impl::set_active_texture(renderer::texture const tex)
{
    
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::renderer_impl::render_copy(
    sdl_texture const& texture
  , SDL_Rect const src, SDL_Rect const dst
) {
    if (SDL_RenderCopy(handle(), texture.handle(), &src, &dst)) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
          << boost::errinfo_api_function {"SDL_RenderCopy"});
    }
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::renderer_impl::render_fill_rect(
    int const x, int const y
  , int const w, int const h
) {
    SDL_Rect const r {x, y, w, h};
    if (SDL_RenderFillRect(handle(), &r)) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
          << boost::errinfo_api_function {"SDL_RenderFillRect"});
    }
}

void bkrl::detail::renderer_impl::draw_cell(
    int const cell_x, int const cell_y, int const tile_index
) {
    auto const r = tile_tilemap_.get_bounds(tile_index);
    auto const w = tile_tilemap_.tile_w();
    auto const h = tile_tilemap_.tile_h();

    SDL_Rect const src {r.left,     r.top,      r.width(), r.height()};
    SDL_Rect const dst {cell_x * w, cell_y * h, r.width(), r.height()};

    render_copy(tile_texture_, src, dst);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////

bkrl::detail::system_impl::system_impl(system* const sys)
  : sys_    {sys}
  , sdl_    {}
  , window_ {}
{
    sys_->on_text_input   = [](bklib::utf8_string_view const&) { };
    sys_->on_request_quit = []() { return true; };
    sys_->on_key_up       = [](int) { };
    sys_->on_key_down     = [](int) { };
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::system_impl::quit() {
    is_running_ = false;
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::system_impl::do_events(bool const wait)
{
    SDL_Event event;

    auto const process_event = [&] {
        switch (event.type) {
        case SDL_QUIT :
            if (sys_->on_request_quit()) {
                is_running_ = false;
            }
            break;
        case SDL_KEYDOWN :
        case SDL_KEYUP :
            handle_keyboard_(event.key);
            break;
        case SDL_TEXTEDITING :
            break;
        case SDL_TEXTINPUT :
            sys_->on_text_input(event.text.text);
            break;
        default :
            break;
        }
    };

    auto const do_pending = [&] {
        int n = 0;
        while (is_running_ && SDL_PollEvent(&event)) {
            ++n;
            process_event();
        };

        return n;
    };

    if (is_running_ && wait) {
        if (!SDL_WaitEvent(&event)) {
            BOOST_THROW_EXCEPTION(bklib::platform_error {}
              << boost::errinfo_api_function {"SDL_WaitEvent"});
        }

        process_event();
    }

    do_pending();
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::system_impl::delay(std::chrono::nanoseconds const ns)
{
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::system_impl::handle_keyboard_(SDL_KeyboardEvent const& event)
{
    if (event.state == SDL_PRESSED) {
        sys_->on_key_down(event.keysym.sym);
    } else if (event.state == SDL_RELEASED) {
        sys_->on_key_up(event.keysym.sym);
    }
}

//----------------------------------------------------------------------------------------------
bkrl::sdl_texture::sdl_texture(sdl_renderer& r, bklib::utf8_string const& filename)
  : handle_ {load_bmp_(r, filename)}
{
}

//----------------------------------------------------------------------------------------------
bkrl::sdl_texture::handle_t bkrl::sdl_texture::load_bmp_(
    sdl_renderer& r
  , bklib::utf8_string const& filename
) {
    sdl_surface surface {SDL_LoadBMP(filename.c_str()), &SDL_FreeSurface};
    if (!surface) {
        BOOST_THROW_EXCEPTION(bklib::io_error {}
          << boost::errinfo_api_function {"SDL_LoadBMP"}
          << boost::errinfo_file_name {filename});
    }

    handle_t result {SDL_CreateTextureFromSurface(r.handle(), surface.get()), &SDL_DestroyTexture};
    if (!result) {
        BOOST_THROW_EXCEPTION(bklib::io_error {}
          << boost::errinfo_api_function {"SDL_CreateTextureFromSurface"}
          << boost::errinfo_file_name {filename});
    }

    return result;
}

std::pair<int, int> bkrl::sdl_texture::get_size(sdl_texture const& texture) const
{
    Uint32 format;
    int    access;
    int    w;
    int    h;

    if (SDL_QueryTexture(texture.handle(), &format, &access, &w, &h)) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
          << boost::errinfo_api_function {"SDL_QueryTexture"});
    }

    return {w, h};
}
