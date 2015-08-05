#if !defined(BK_NO_SDL)
#include "system_sdl.hpp"
#include "bklib/scope_guard.hpp"

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

void bkrl::detail::renderer_impl::clear_clip_region()
{
    if (SDL_RenderSetClipRect(handle(), nullptr)) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
          << boost::errinfo_api_function {"SDL_RenderSetClipRect"});
    }
}

void bkrl::detail::renderer_impl::set_clip_region(rect_t const r)
{
    if (SDL_RenderSetClipRect(handle(), reinterpret_cast<SDL_Rect const*>(&r))) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
          << boost::errinfo_api_function {"SDL_RenderSetClipRect"});
    }
}

bkrl::detail::renderer_impl::rect_t bkrl::detail::renderer_impl::get_clip_region()
{
    rect_t result;
    SDL_RenderGetClipRect(handle(), reinterpret_cast<SDL_Rect*>(&result));
    return result;
}

void bkrl::detail::renderer_impl::set_scale(double const sx, double const sy)
{
    sx_ = sx;
    sy_ = sy;
}

void bkrl::detail::renderer_impl::set_scale(double const scale)
{
    set_scale(scale, scale);
}

void bkrl::detail::renderer_impl::set_translation(double const dx, double const dy)
{
    tx_ = dx;
    ty_ = dy;
}

bklib::point_t<2, double> bkrl::detail::renderer_impl::get_scale() const
{
    return {sx_, sy_};
}

bklib::point_t<2, double> bkrl::detail::renderer_impl::get_translation() const
{
    return {tx_, ty_};
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
void bkrl::detail::renderer_impl::set_active_texture(renderer::texture const)
{

}

//----------------------------------------------------------------------------------------------
void bkrl::detail::renderer_impl::render_copy(
    sdl_texture const& texture
  , SDL_Rect const src, SDL_Rect dst
) {
    dst.x = bklib::floor_to<int>(dst.x * sx_ + tx_);
    dst.y = bklib::floor_to<int>(dst.y * sy_ + ty_);
    dst.w = bklib::ceil_to<int>(dst.w * sx_);
    dst.h = bklib::ceil_to<int>(dst.h * sy_);

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

//----------------------------------------------------------------------------------------------
void bkrl::detail::renderer_impl::render_fill_rect(
    int const x, int const y
  , int const w, int const h
  , color4 const c
) {
    if (c[3] != 0xFF && SDL_SetRenderDrawBlendMode(handle(), SDL_BlendMode::SDL_BLENDMODE_BLEND)) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
          << boost::errinfo_api_function {"SDL_SetRenderDrawBlendMode"});
    }

    if (SDL_SetRenderDrawColor(handle(), c[0], c[1], c[2], c[3])) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
          << boost::errinfo_api_function {"SDL_SetRenderDrawColor"});
    }

    render_fill_rect(x, y, w, h);
}

//----------------------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------------------
namespace {

template <typename T>
inline T get_integer_at(
    void const* p
  , size_t const xi, size_t const yi, size_t const w
  , ptrdiff_t const offset
  , size_t const stride
) noexcept {
    static_assert(std::is_integral<T>::value, "");

    return *reinterpret_cast<T const*>(
        static_cast<char const*>(p) + stride * (yi * w + xi) + offset
    );
}

} //namespace

//----------------------------------------------------------------------------------------------
void bkrl::detail::renderer_impl::draw_cells(
    int const xoff, int const yoff
  , size_t const w, size_t const h
  , void const* const data
  , ptrdiff_t const tex_offset, size_t const tex_size
  , size_t const stride
) {
    auto const tw = tile_tilemap_.tile_w();
    auto const th = tile_tilemap_.tile_h();

    for (auto yi = 0u; yi < h; ++yi) {
        for (auto xi = 0u; xi < w; ++xi) {
            auto const i = get_integer_at<uint16_t>(data, xi, yi, w, tex_offset, stride);
            if (i == 0) { continue; }

            auto const r = tile_tilemap_.get_bounds(i);

            auto const dx = tw * (xoff + static_cast<int>(xi));
            auto const dy = th * (yoff + static_cast<int>(yi));

            render_copy(tile_texture_
              , SDL_Rect {r.left, r.top, tw, th}
              , SDL_Rect {dx, dy, tw, th});
        }
    }
}

void bkrl::detail::renderer_impl::draw_rects(
    int const xoff, int const yoff
  , size_t const count
  , void const* const data
  , ptrdiff_t const src_pos_offset, size_t const src_pos_size
  , ptrdiff_t const dst_pos_offset, size_t const dst_pos_size
  , ptrdiff_t const color_offset,   size_t const color_size
  , size_t const stride
) {
    color4 old;
    SDL_GetTextureColorMod(tile_texture_.handle(), &old[0], &old[1], &old[2]);
    BK_SCOPE_EXIT {
        SDL_SetTextureColorMod(tile_texture_.handle(), old[0], old[1], old[2]);
    };

    auto p = static_cast<char const*>(data);

    for (auto i = 0u; i < count; ++i) {
        auto const src = reinterpret_cast<int16_t const*>(p + src_pos_offset);
        auto const dst = reinterpret_cast<int16_t const*>(p + dst_pos_offset);
        auto const col = reinterpret_cast<uint8_t const*>(p + color_offset);

        SDL_SetTextureColorMod(tile_texture_.handle(), col[0], col[1], col[2]);

        render_copy(tile_texture_
            , SDL_Rect {       src[0],        src[1], src[2], src[3]}
            , SDL_Rect {xoff + dst[0], yoff + dst[1], src[2], src[3]});

        p += stride;
    }
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::renderer_impl::draw_cell(
    int const cell_x, int const cell_y
  , int const tile_index
  , color4 const color
) {
    color4 old;
    SDL_GetTextureColorMod(tile_texture_.handle(), &old[0], &old[1], &old[2]);
    SDL_SetTextureColorMod(tile_texture_.handle(), color[0], color[1], color[2]);
    draw_cell(cell_x, cell_y, tile_index);
    SDL_SetTextureColorMod(tile_texture_.handle(), old[0], old[1], old[2]);
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::renderer_impl::draw_rect(rect_t const src, rect_t const dst)
{
    static_assert(sizeof(rect_t) == sizeof(SDL_Rect), "");
    static_assert(alignof(rect_t) == alignof(SDL_Rect), "");

    render_copy(tile_texture_
        , reinterpret_cast<SDL_Rect const&>(src)
        , reinterpret_cast<SDL_Rect const&>(dst)
    );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////

bkrl::detail::system_impl::system_impl(system* const sys)
  : sys_    {sys}
  , sdl_    {}
  , window_ {}
{
    sys_->on_window_resize = [](int, int) { };
    sys_->on_text_input    = [](bklib::utf8_string_view const&) { };
    sys_->on_request_quit  = []() { return true; };
    sys_->on_key_up        = [](int) { };
    sys_->on_key_down      = [](int) { };
    sys_->on_mouse_motion  = [](mouse_state) { };
    sys_->on_mouse_move    = [](mouse_state) { };
    sys_->on_mouse_scroll  = [](mouse_state) { };
    sys_->on_mouse_button  = [](mouse_button_state) { };
}

//----------------------------------------------------------------------------------------------
bklib::ipoint2 bkrl::detail::system_impl::client_size() const
{
    int w {0};
    int h {0};

    SDL_GetWindowSize(window_.handle(), &w, &h);

    return {w, h};
}

//----------------------------------------------------------------------------------------------
int bkrl::detail::system_impl::client_width() const
{
    return x(client_size());
}

//----------------------------------------------------------------------------------------------
int bkrl::detail::system_impl::client_height() const
{
    return y(client_size());
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
        case SDL_WINDOWEVENT :
            handle_window_(event.window);
            break;
        case SDL_MOUSEWHEEL :
            handle_mouse_wheel_(event.wheel);
            break;
        case SDL_MOUSEMOTION :
            handle_mouse_motion_(event.motion);
            break;
        case SDL_MOUSEBUTTONDOWN :
        case SDL_MOUSEBUTTONUP :
            handle_mouse_button_(event.button);
            break;
        case SDL_KEYDOWN :
        case SDL_KEYUP :
            handle_keyboard_(event.key);
            break;
        case SDL_TEXTEDITING :
            break;
        case SDL_TEXTINPUT :
            handle_text_input_(event.text);
            break;
        default :
            break;
        }
    };

    SDL_ClearError();
    if (!is_running_ || !wait || SDL_WaitEventTimeout(nullptr, 100)) {
        while (SDL_PollEvent(&event)) {
            process_event();
        }
        return;
    }

    auto const msg = SDL_GetError();
    if (*msg) {
        BOOST_THROW_EXCEPTION(bklib::platform_error {}
            << boost::errinfo_api_function {"SDL_WaitEvent"});
    }
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::system_impl::delay(std::chrono::nanoseconds const)
{
}

//----------------------------------------------------------------------------------------------
bkrl::key_mod_state bkrl::detail::system_impl::current_key_mods() const
{
    auto const mods = SDL_GetModState();

    key_mod_state result {};

    using km = bkrl::key_mod;

    auto const do_set = [&](km const m, bool const b) noexcept {
        if (b) {
            result.set(m);
        }
    };

    do_set(km::lshift, !!(mods & KMOD_LSHIFT));
    do_set(km::rshift, !!(mods & KMOD_RSHIFT));
    do_set(km::lctrl,  !!(mods & KMOD_LCTRL));
    do_set(km::rctrl,  !!(mods & KMOD_RCTRL));
    do_set(km::lalt,   !!(mods & KMOD_LALT));
    do_set(km::ralt,   !!(mods & KMOD_RALT));
    do_set(km::lgui,   !!(mods & KMOD_LGUI));
    do_set(km::rgui,   !!(mods & KMOD_RGUI));
    do_set(km::num,    !!(mods & KMOD_NUM));
    do_set(km::caps,   !!(mods & KMOD_CAPS));
    do_set(km::mode,   !!(mods & KMOD_MODE));
    do_set(km::ctrl,   !!(mods & KMOD_CTRL));
    do_set(km::shift,  !!(mods & KMOD_SHIFT));
    do_set(km::alt,    !!(mods & KMOD_ALT));
    do_set(km::gui,    !!(mods & KMOD_GUI));

    return result;
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
void bkrl::detail::system_impl::handle_mouse_wheel_(SDL_MouseWheelEvent const& event)
{
    mouse_state const state {
        last_mouse_.x, last_mouse_.y
      , 0, 0
      , event.x, event.y
      , last_mouse_.state
      , event.timestamp
    };

    sys_->on_mouse_scroll(state);
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::system_impl::handle_mouse_button_(SDL_MouseButtonEvent const& event)
{
    mouse_button_state const state {
        event.x, event.y
      , event.timestamp
      , event.button
      , event.state
      , event.clicks
    };

    sys_->on_mouse_button(state);
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::system_impl::handle_mouse_motion_(SDL_MouseMotionEvent const& event)
{
    mouse_state const state {
        event.x, event.y
      , event.xrel, event.yrel
      , 0, 0
      , event.state
      , event.timestamp
    };

    sys_->on_mouse_motion(state);

    last_mouse_ = state;
}

//----------------------------------------------------------------------------------------------
void bkrl::detail::system_impl::handle_window_(SDL_WindowEvent const& event)
{
    switch (event.event) {
    case SDL_WINDOWEVENT_NONE :
    case SDL_WINDOWEVENT_SHOWN :
    case SDL_WINDOWEVENT_HIDDEN :
    case SDL_WINDOWEVENT_EXPOSED :
    case SDL_WINDOWEVENT_MOVED :
        break;
    case SDL_WINDOWEVENT_RESIZED :
        sys_->on_window_resize(event.data1, event.data2);
        break;
    case SDL_WINDOWEVENT_SIZE_CHANGED :
    case SDL_WINDOWEVENT_MINIMIZED :
    case SDL_WINDOWEVENT_MAXIMIZED :
    case SDL_WINDOWEVENT_RESTORED :
    case SDL_WINDOWEVENT_ENTER :
    case SDL_WINDOWEVENT_LEAVE :
    case SDL_WINDOWEVENT_FOCUS_GAINED :
    case SDL_WINDOWEVENT_FOCUS_LOST :
    case SDL_WINDOWEVENT_CLOSE :
    default:
        break;
    }
}

void bkrl::detail::system_impl::handle_text_input_(SDL_TextInputEvent const& event)
{
    sys_->on_text_input(event.text);
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

#endif // !defined(BK_NO_SDL)
