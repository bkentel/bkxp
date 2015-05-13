#include "renderer.hpp"
#include "system_sdl.hpp"

bkrl::renderer::renderer(system& sys)
  : impl_ {std::make_unique<detail::renderer_impl>(sys)}
{
}

bkrl::renderer::~renderer() = default;

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

void bkrl::renderer::draw_cell(int const cell_x, int const cell_y, int const tile_index)
{
    impl_->draw_cell(cell_x, cell_y, tile_index);
}
