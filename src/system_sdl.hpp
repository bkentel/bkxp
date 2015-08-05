#pragma once
#if !defined(BK_NO_SDL)

#include "system.hpp"
#include "renderer.hpp"
#include "bklib/exception.hpp"
#include <SDL2/SDL.h>
#include <memory>

namespace bkrl {

class sdl_state;
class sdl_window;
class sdl_texture;

//----------------------------------------------------------------------------------------------
//! Global SDl state.
//----------------------------------------------------------------------------------------------
class sdl_state {
public:
    sdl_state();
    ~sdl_state();
};

//----------------------------------------------------------------------------------------------
//! SDL Window interaction.
//----------------------------------------------------------------------------------------------
class sdl_window {
public:
    sdl_window()
      : handle_(create_())
    {
    }

    ~sdl_window() = default;

    SDL_Window* handle()       noexcept { return handle_.get(); }
    SDL_Window* handle() const noexcept { return handle_.get(); }
private:
    using handle_t = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;

    static handle_t create_();

    handle_t handle_;
};

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
using sdl_surface = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;

using sdl_renderer = class detail::renderer_impl;

//----------------------------------------------------------------------------------------------
//! SDL texture interaction.
//----------------------------------------------------------------------------------------------
class sdl_texture {
public:
    sdl_texture(sdl_renderer& r, bklib::utf8_string const& filename);
    ~sdl_texture() = default;

    SDL_Texture* handle()       noexcept { return handle_.get(); }
    SDL_Texture* handle() const noexcept { return handle_.get(); }

    std::pair<int, int> get_size(sdl_texture const& texture) const;
private:
    using handle_t = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
    static handle_t load_bmp_(sdl_renderer& r, bklib::utf8_string const& filename);

    handle_t handle_;
};

//----------------------------------------------------------------------------------------------
//! SDL renderer interaction.
//----------------------------------------------------------------------------------------------
class detail::renderer_impl {
public:
    using rect_t = renderer::rect_t;

    explicit renderer_impl(system& sys);

    ~renderer_impl() = default;

    SDL_Renderer* handle()       noexcept { return handle_.get(); }
    SDL_Renderer* handle() const noexcept { return handle_.get(); }

    void   clear_clip_region();
    void   set_clip_region(rect_t r);
    rect_t get_clip_region();

    void set_scale(double sx, double sy);
    void set_scale(double scale);

    void set_translation(double dx, double dy);

    bklib::point_t<2, double> get_scale() const;
    bklib::point_t<2, double> get_translation() const;

    void clear();
    void present();

    void set_active_texture(renderer::texture tex);

    void render_copy(sdl_texture const& texture, SDL_Rect src, SDL_Rect dst);

    void render_fill_rect(int x, int y, int w, int h);
    void render_fill_rect(int x, int y, int w, int h, color4 c);

    void draw_cell(int cell_x, int cell_y, int tile_index);
    void draw_cell(int cell_x, int cell_y, int tile_index, color4 color);
    void draw_rect(rect_t src, rect_t dst);

    void draw_cells(
        int xoff, int yoff
      , size_t w, size_t h
      , void const* data
      , ptrdiff_t tex_offset, size_t tex_size
      , size_t stride
    );

    void draw_rects(
        int xoff, int yoff
      , size_t count
      , void const* data
      , ptrdiff_t src_pos_offset, size_t src_pos_size
      , ptrdiff_t dst_pos_offset, size_t dst_pos_size
      , ptrdiff_t color_offset,   size_t color_size
      , size_t stride
    );
private:
    using handle_t = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;

    static handle_t create_(sdl_window const& w);

    handle_t handle_;

    double sx_ = 1.0;
    double sy_ = 1.0;
    double tx_ = 0.0;
    double ty_ = 0.0;

    sdl_texture tile_texture_; //TODO move in the future
    tilemap     tile_tilemap_;  //TODO move in the future
};

//----------------------------------------------------------------------------------------------
class detail::system_impl {
    friend renderer_impl;
public:
    explicit system_impl(system* sys);

    bklib::ipoint2 client_size() const;
    int client_width() const;
    int client_height() const;

    void quit();

    bool is_running() const noexcept {
        return is_running_;
    }

    void do_events(bool wait);

    void delay(std::chrono::nanoseconds ns);

    key_mod_state current_key_mods() const;
private:
    void handle_keyboard_(SDL_KeyboardEvent const& event);
    void handle_mouse_motion_(SDL_MouseMotionEvent const& event);
    void handle_mouse_wheel_(SDL_MouseWheelEvent const& event);
    void handle_mouse_button_(SDL_MouseButtonEvent const& event);
    void handle_window_(SDL_WindowEvent const& event);
    void handle_text_input_(SDL_TextInputEvent const& event);

    system*     sys_ = nullptr;
    sdl_state   sdl_;
    sdl_window  window_;
    mouse_state last_mouse_ {};
    bool        is_running_ = true;
};

} // namespace bkrl

#endif // !defined(BK_NO_SDL)
