#include "renderer.hpp"
#include "system_sdl.hpp"

#if defined(BK_NO_SDL)
class bkrl::detail::renderer_impl {
public:
    using rect_t = renderer::rect_t;

    explicit renderer_impl(system&) {}

    void clear_clip_region() { }
    void set_clip_region(rect_t) { }
    rect_t get_clip_region() { return {}; }

    void set_scale(double const, double const) { }
    void set_scale(double const) { }
    void set_translation(double const, double const) { }
    bklib::point_t<2, double> get_scale() const { return {}; }
    bklib::point_t<2, double> get_translation() const { return {}; }
    void clear() { }
    void present() { }
    void set_active_texture(renderer::texture const) { }
    void render_fill_rect(int, int, int, int) { }
    void render_fill_rect(int, int, int, int, color4) { }
    void draw_cell(int const, int const, int const) { }
    void draw_cell(int const, int const, int const, color4 const) { }
    void draw_rect(renderer::rect_t const, renderer::rect_t const) { }
    void draw_cells(int, int, size_t, size_t, void const*, ptrdiff_t, size_t, size_t) { }
    void draw_rects(int, int, size_t, void const*, ptrdiff_t, size_t
                  , ptrdiff_t, size_t, ptrdiff_t, size_t, size_t) { }
};
#endif

bkrl::renderer::renderer(system& sys)
  : impl_ {std::make_unique<detail::renderer_impl>(sys)}
{
}

bkrl::renderer::~renderer() = default;

void bkrl::renderer::clear_clip_region() {
    impl_->clear_clip_region();
}

void bkrl::renderer::set_clip_region(rect_t r) {
    impl_->set_clip_region(r);
}

bkrl::renderer::rect_t bkrl::renderer::get_clip_region() {
    return impl_->get_clip_region();
}

void bkrl::renderer::set_scale(double const sx, double const sy)
{
    impl_->set_scale(sx, sy);
}

void bkrl::renderer::set_scale(double const scale)
{
    set_scale(scale, scale);
}

void bkrl::renderer::set_translation(double const dx, double const dy)
{
    impl_->set_translation(dx, dy);
}

bklib::point_t<2, double> bkrl::renderer::get_scale() const
{
    return impl_->get_scale();
}

bklib::point_t<2, double> bkrl::renderer::get_translation() const
{
    return impl_->get_translation();
}

void bkrl::renderer::clear()
{
    impl_->clear();
}

void bkrl::renderer::present()
{
    impl_->present();
}

void bkrl::renderer::set_active_texture(texture const tex)
{
    impl_->set_active_texture(tex);
}

void bkrl::renderer::draw_filled_rect(rect_t const r)
{
    impl_->render_fill_rect(r.x, r.y, r.w, r.h);
}

void bkrl::renderer::draw_filled_rect(rect_t const r, color4 const c)
{
    impl_->render_fill_rect(r.x, r.y, r.w, r.h, c);
}

void bkrl::renderer::draw_cell(int const cell_x, int const cell_y, int const tile_index)
{
    impl_->draw_cell(cell_x, cell_y, tile_index);
}

void bkrl::renderer::draw_cell(
    int const cell_x, int const cell_y
  , int const tile_index, color4 const color
) {
    impl_->draw_cell(cell_x, cell_y, tile_index, color);
}

void bkrl::renderer::draw_rect(rect_t const src, rect_t const dst)
{
    impl_->draw_rect(src, dst);
}

void bkrl::renderer::draw_cells(
    int const xoff, int const yoff
  , size_t const w, size_t const h
  , void const* const data
  , ptrdiff_t const tex_offset, size_t const tex_size
  , size_t const stride
) {
    impl_->draw_cells(xoff, yoff, w, h, data, tex_offset, tex_size, stride);
}

void bkrl::renderer::draw_rects(
    int xoff, int yoff
  , size_t count
  , void const* data
  , ptrdiff_t src_pos_offset, size_t src_pos_size
  , ptrdiff_t dst_pos_offset, size_t dst_pos_size
  , ptrdiff_t color_offset,   size_t color_size
  , size_t stride
) {
    impl_->draw_rects(xoff, yoff, count, data
                    , src_pos_offset, src_pos_size
                    , dst_pos_offset, dst_pos_size
                    , color_offset, color_size
                    , stride);
}
