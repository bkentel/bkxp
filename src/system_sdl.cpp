#if !defined(BK_NO_SDL)

#include "system.hpp"
#include "renderer.hpp"

#include "bklib/scope_guard.hpp"
#include "bklib/exception.hpp"

#include <SDL2/SDL.h>
#include <memory>

namespace bkrl { class sdl_state; }
namespace bkrl { class sdl_window; }
namespace bkrl { class sdl_texture; }
namespace bkrl { class system_sdl_impl; }
namespace bkrl { class renderer_sdl_impl; }

//--------------------------------------------------------------------------------------------------
//! SDL surface RAII wrapper
//--------------------------------------------------------------------------------------------------
namespace bkrl {
    using sdl_surface = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;
}

//--------------------------------------------------------------------------------------------------
//! Global SDL state.
//--------------------------------------------------------------------------------------------------
class bkrl::sdl_state {
public:
    sdl_state(sdl_state const&) = delete;
    sdl_state(sdl_state&&) = default;
    sdl_state& operator=(sdl_state const&) = delete;
    sdl_state& operator=(sdl_state&&) = default;

    sdl_state() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            BOOST_THROW_EXCEPTION(bklib::platform_error {}
              << boost::errinfo_api_function {"SDL_Init"});
        }
    }

    ~sdl_state() {
        SDL_Quit();
    }
};

//--------------------------------------------------------------------------------------------------
//! SDL Window interaction.
//--------------------------------------------------------------------------------------------------
class bkrl::sdl_window {
public:

    sdl_window() = default;
    sdl_window(sdl_window const&) = delete;
    sdl_window(sdl_window&&) = default;
    sdl_window& operator=(sdl_window const&) = delete;
    sdl_window& operator=(sdl_window&&) = default;

    SDL_Window* handle()       noexcept { return handle_.get(); }
    SDL_Window* handle() const noexcept { return handle_.get(); }
private:
    using handle_t = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;

    static handle_t create_() {
        auto const result = SDL_CreateWindow("BKRL"
            , SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
            , 1024, 768, SDL_WINDOW_RESIZABLE);

        if (!result) {
            BOOST_THROW_EXCEPTION(bklib::platform_error {}
              << boost::errinfo_api_function {"SDL_CreateWindow"});
        }

        return handle_t(result, &SDL_DestroyWindow);
    }

    handle_t handle_ = create_();
};

//--------------------------------------------------------------------------------------------------
//! SDL texture interaction.
//--------------------------------------------------------------------------------------------------
class bkrl::sdl_texture {
public:
    sdl_texture(renderer_sdl_impl& r, bklib::utf8_string const& filename)
      : handle_ {load_bmp_(r, filename)}
    {
    }

    sdl_texture(sdl_texture const&) = delete;
    sdl_texture(sdl_texture&&) = default;
    sdl_texture& operator=(sdl_texture const&) = delete;
    sdl_texture& operator=(sdl_texture&&) = default;

    SDL_Texture* handle()       noexcept { return handle_.get(); }
    SDL_Texture* handle() const noexcept { return handle_.get(); }

    std::pair<int, int> get_size(sdl_texture const& texture) const {
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
private:
    using handle_t = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

    static handle_t load_bmp_(renderer_sdl_impl& r, bklib::utf8_string const& filename);

    handle_t handle_;
};

//--------------------------------------------------------------------------------------------------
//! SDL system implementation.
//--------------------------------------------------------------------------------------------------
class bkrl::system_sdl_impl final : public system {
public:
    virtual ~system_sdl_impl();

    system_sdl_impl() {
        on_window_resize = [](int, int) { };
        on_text_input    = [](bklib::utf8_string_view const&) { };
        on_request_quit  = []() { return true; };
        on_key_up        = [](int) { };
        on_key_down      = [](int) { };
        on_mouse_motion  = [](mouse_state) { };
        on_mouse_move    = [](mouse_state) { };
        on_mouse_scroll  = [](mouse_state) { };
        on_mouse_button  = [](mouse_button_state) { };
    }

    //----------------------------------------------------------------------------------------------
    int client_width() const noexcept override final {
        return x(client_size_());
    }

    int client_height() const noexcept override final {
        return y(client_size_());
    }

    //----------------------------------------------------------------------------------------------
    void quit() noexcept override final {
         is_running_ = false;
    }

    //----------------------------------------------------------------------------------------------
    bool is_running() const noexcept override final {
        return is_running_;
    }

    //----------------------------------------------------------------------------------------------
    void do_events_nowait() override final { do_events_(false); }

    void do_events_wait() override final { do_events_(true); }

    //----------------------------------------------------------------------------------------------
    void delay(std::chrono::nanoseconds) override final {
    }

    //----------------------------------------------------------------------------------------------
    key_mod_state current_key_mods() const noexcept override final;
public:
    sdl_window&       window()       noexcept { return window_; }
    sdl_window const& window() const noexcept { return window_; }
private:
    bklib::ipoint2 client_size_() const noexcept {
        int w {0};
        int h {0};

        SDL_GetWindowSize(window_.handle(), &w, &h);

        return {w, h};
    }

    void do_events_(bool wait);

    void handle_keyboard_(SDL_KeyboardEvent const& event);
    void handle_mouse_motion_(SDL_MouseMotionEvent const& event);
    void handle_mouse_wheel_(SDL_MouseWheelEvent const& event);
    void handle_mouse_button_(SDL_MouseButtonEvent const& event);
    void handle_window_(SDL_WindowEvent const& event);
    void handle_text_input_(SDL_TextInputEvent const& event);

    sdl_state   sdl_ {};
    sdl_window  window_ {};
    mouse_state last_mouse_ {};
    bool        is_running_ = true;
};

//--------------------------------------------------------------------------------------------------
//! SDL renderer implementation.
//--------------------------------------------------------------------------------------------------
class bkrl::renderer_sdl_impl final : public renderer {
public:
    void clear_clip_region() override final {
        if (SDL_RenderSetClipRect(handle(), nullptr)) {
            BOOST_THROW_EXCEPTION(bklib::platform_error {}
              << boost::errinfo_api_function {"SDL_RenderSetClipRect"});
        }
    }

    void set_clip_region(rect_t r) override final {
        if (SDL_RenderSetClipRect(handle(), reinterpret_cast<SDL_Rect const*>(&r))) {
            BOOST_THROW_EXCEPTION(bklib::platform_error {}
              << boost::errinfo_api_function {"SDL_RenderSetClipRect"});
        }
    }

    rect_t get_clip_region() override final {
        rect_t result;
        SDL_RenderGetClipRect(handle(), reinterpret_cast<SDL_Rect*>(&result));
        return result;
    }

    void set_scale(double sx, double sy) override final {
        sx_ = sx;
        sy_ = sy;
    }

    void set_scale(double scale) override final {
        set_scale(scale, scale);
    }

    void set_translation(double dx, double dy) override final {
        tx_ = dx;
        ty_ = dy;
    }

    bklib::point_t<2, double> get_scale() const override final {
        return {sx_, sy_};
    }

    bklib::point_t<2, double> get_translation() const override final {
        return {tx_, ty_};
    }

    void clear() override final {
        if (SDL_SetRenderDrawColor(handle(), 255, 0, 0, 255)) {
            BOOST_THROW_EXCEPTION(bklib::platform_error {}
              << boost::errinfo_api_function {"SDL_SetRenderDrawColor"});
        }

        if (SDL_RenderClear(handle())) {
            BOOST_THROW_EXCEPTION(bklib::platform_error {}
              << boost::errinfo_api_function {"SDL_RenderClear"});
        }
    }

    void present() override final {
        SDL_RenderPresent(handle());
    }

    void set_active_texture(texture) override final { }

    void draw_filled_rect(rect_t r) override final { }
    void draw_filled_rect(rect_t r, color4 c) override final { }

    void draw_cell(int cell_x, int cell_y, int tile_index) override final {
        auto const r = tile_tilemap_.get_bounds(tile_index);
        auto const w = tile_tilemap_.tile_w();
        auto const h = tile_tilemap_.tile_h();

        SDL_Rect const src {r.left,     r.top,      r.width(), r.height()};
        SDL_Rect const dst {cell_x * w, cell_y * h, r.width(), r.height()};

        render_copy(tile_texture_, src, dst);
    }

    void draw_cell(int cell_x, int cell_y, int tile_index, color4 color) override final {
        auto const tex_handle = tile_texture_.handle();

        color4 const old = [tex_handle] {
            color4 result;
            SDL_GetTextureColorMod(tex_handle, &result[0], &result[1], &result[2]);
            return result;
        }();

        BK_SCOPE_EXIT {
            SDL_SetTextureColorMod(tex_handle, old[0], old[1], old[2]);
        };

        SDL_SetTextureColorMod(tex_handle, color[0], color[1], color[2]);
        draw_cell(cell_x, cell_y, tile_index);
    }

    void draw_rect(rect_t src, rect_t dst) override final {
        static_assert(sizeof(rect_t) == sizeof(SDL_Rect), "");
        static_assert(alignof(rect_t) == alignof(SDL_Rect), "");
        static_assert(std::is_pod<rect_t>::value, "");
        static_assert(std::is_pod<SDL_Rect>::value, "");

        render_copy(tile_texture_
            , reinterpret_cast<SDL_Rect const&>(src)
            , reinterpret_cast<SDL_Rect const&>(dst)
        );
    }

    void draw_cells(
        int xoff, int yoff
      , size_t w, size_t h
      , void const* data
      , ptrdiff_t tex_offset, size_t tex_size
      , size_t stride
    ) override final;

    void draw_rects(
        int xoff, int yoff
      , size_t count
      , void const* data
      , ptrdiff_t src_pos_offset, size_t src_pos_size
      , ptrdiff_t dst_pos_offset, size_t dst_pos_size
      , ptrdiff_t color_offset,   size_t color_size
      , size_t stride
    ) override final;
public:
    explicit renderer_sdl_impl(system_sdl_impl& sys)
      : handle_       {create_(sys.window())}
      , tile_texture_ {*this, "data/tiles.bmp"}
      , tile_tilemap_ {make_tilemap_(tile_texture_)}
    {
    }

    virtual ~renderer_sdl_impl();

    SDL_Renderer* handle()       noexcept { return handle_.get(); }
    SDL_Renderer* handle() const noexcept { return handle_.get(); }

    void render_copy(sdl_texture const& texture, SDL_Rect src, SDL_Rect dst) {
        dst.x = bklib::floor_to<int>(dst.x * sx_ + tx_);
        dst.y = bklib::floor_to<int>(dst.y * sy_ + ty_);
        dst.w = bklib::ceil_to<int>(dst.w * sx_);
        dst.h = bklib::ceil_to<int>(dst.h * sy_);

        if (SDL_RenderCopy(handle(), texture.handle(), &src, &dst)) {
            BOOST_THROW_EXCEPTION(bklib::platform_error {}
              << boost::errinfo_api_function {"SDL_RenderCopy"});
        }
    }

    void render_fill_rect(int x, int y, int w, int h) {
        SDL_Rect const r {x, y, w, h};
        if (SDL_RenderFillRect(handle(), &r)) {
            BOOST_THROW_EXCEPTION(bklib::platform_error {}
              << boost::errinfo_api_function {"SDL_RenderFillRect"});
        }
    }

    void render_fill_rect(int x, int y, int w, int h, color4 c) {
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
private:
    using handle_t = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;

    static handle_t create_(sdl_window const& w) {
        auto const result = SDL_CreateRenderer(w.handle(), -1, SDL_RENDERER_ACCELERATED);

        if (!result) {
            BOOST_THROW_EXCEPTION(bklib::platform_error {}
              << boost::errinfo_api_function {"SDL_CreateRenderer"});
        }

        return handle_t(result, &SDL_DestroyRenderer);
    }

    static tilemap make_tilemap_(sdl_texture const& tex) {
        auto const size = tex.get_size(tex);
        return {18, 18, size.first, size.second};
    }

    handle_t handle_;

    double sx_ = 1.0;
    double sy_ = 1.0;
    double tx_ = 0.0;
    double ty_ = 0.0;

    sdl_texture tile_texture_; //TODO move in the future
    tilemap     tile_tilemap_;  //TODO move in the future
};

//=====----------------------------------------------------------------------------------------=====
//                                          bkrl::sdl_texture
//=====----------------------------------------------------------------------------------------=====
bkrl::sdl_texture::handle_t
bkrl::sdl_texture::load_bmp_(renderer_sdl_impl& r, bklib::utf8_string const& filename)
{
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

//=====----------------------------------------------------------------------------------------=====
//                                          bkrl::system_sdl_impl
//=====----------------------------------------------------------------------------------------=====

//----------------------------------------------------------------------------------------------
bkrl::system_sdl_impl::~system_sdl_impl() {
}

//----------------------------------------------------------------------------------------------
void bkrl::system_sdl_impl::do_events_(bool const wait)
{
    SDL_Event event;

    auto const process_event = [&] {
        switch (event.type) {
        case SDL_QUIT :
            if (on_request_quit()) {
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
bkrl::key_mod_state
bkrl::system_sdl_impl::current_key_mods() const noexcept
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
void bkrl::system_sdl_impl::handle_keyboard_(SDL_KeyboardEvent const& event)
{
    if (event.state == SDL_PRESSED) {
        on_key_down(event.keysym.sym);
    } else if (event.state == SDL_RELEASED) {
        on_key_up(event.keysym.sym);
    }
}

//----------------------------------------------------------------------------------------------
void bkrl::system_sdl_impl::handle_mouse_wheel_(SDL_MouseWheelEvent const& event)
{
    mouse_state const state {
        last_mouse_.x, last_mouse_.y
      , 0, 0
      , event.x, event.y
      , last_mouse_.state
      , event.timestamp
    };

    on_mouse_scroll(state);
}

//----------------------------------------------------------------------------------------------
void bkrl::system_sdl_impl::handle_mouse_button_(SDL_MouseButtonEvent const& event)
{
    mouse_button_state const state {
        event.x, event.y
      , event.timestamp
      , event.button
      , event.state
      , event.clicks
    };

    on_mouse_button(state);
}

//----------------------------------------------------------------------------------------------
void bkrl::system_sdl_impl::handle_mouse_motion_(SDL_MouseMotionEvent const& event)
{
    mouse_state const state {
        event.x, event.y
      , event.xrel, event.yrel
      , 0, 0
      , event.state
      , event.timestamp
    };

    on_mouse_motion(state);

    last_mouse_ = state;
}

//----------------------------------------------------------------------------------------------
void bkrl::system_sdl_impl::handle_window_(SDL_WindowEvent const& event)
{
    switch (event.event) {
    case SDL_WINDOWEVENT_NONE :
    case SDL_WINDOWEVENT_SHOWN :
    case SDL_WINDOWEVENT_HIDDEN :
    case SDL_WINDOWEVENT_EXPOSED :
    case SDL_WINDOWEVENT_MOVED :
        break;
    case SDL_WINDOWEVENT_RESIZED :
        on_window_resize(event.data1, event.data2);
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

//----------------------------------------------------------------------------------------------
void bkrl::system_sdl_impl::handle_text_input_(SDL_TextInputEvent const& event)
{
    on_text_input(event.text);
}

//=====----------------------------------------------------------------------------------------=====
//                                          bkrl::renderer_sdl_impl
//=====----------------------------------------------------------------------------------------=====

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
bkrl::renderer_sdl_impl::~renderer_sdl_impl() {
}

//----------------------------------------------------------------------------------------------
void bkrl::renderer_sdl_impl::draw_cells(
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

//----------------------------------------------------------------------------------------------
void bkrl::renderer_sdl_impl::draw_rects(
    int const xoff, int const yoff
  , size_t const count
  , void const* const data
  , ptrdiff_t const src_pos_offset, size_t const src_pos_size
  , ptrdiff_t const dst_pos_offset, size_t const dst_pos_size
  , ptrdiff_t const color_offset,   size_t const color_size
  , size_t const stride
) {
    auto const tex_handle = tile_texture_.handle();

    color4 old;
    SDL_GetTextureColorMod(tex_handle, &old[0], &old[1], &old[2]);
    BK_SCOPE_EXIT {
        SDL_SetTextureColorMod(tex_handle, old[0], old[1], old[2]);
    };

    auto p = static_cast<char const*>(data);

    for (auto i = 0u; i < count; ++i) {
        auto const src = reinterpret_cast<int16_t const*>(p + src_pos_offset);
        auto const dst = reinterpret_cast<int16_t const*>(p + dst_pos_offset);
        auto const col = reinterpret_cast<uint8_t const*>(p + color_offset);

        SDL_SetTextureColorMod(tex_handle, col[0], col[1], col[2]);

        render_copy(tile_texture_
            , SDL_Rect {       src[0],        src[1], src[2], src[3]}
            , SDL_Rect {xoff + dst[0], yoff + dst[1], src[2], src[3]});

        p += stride;
    }
}

//=====----------------------------------------------------------------------------------------=====
//                                          bkrl
//=====----------------------------------------------------------------------------------------=====

//--------------------------------------------------------------------------------------------------
std::unique_ptr<bkrl::system> bkrl::make_system() {
    return std::make_unique<system_sdl_impl>();
}

//--------------------------------------------------------------------------------------------------
std::unique_ptr<bkrl::renderer> bkrl::make_renderer(system& sys) {
    return std::make_unique<renderer_sdl_impl>(
        dynamic_cast<system_sdl_impl&>(sys));
}

#endif // !defined(BK_NO_SDL)
