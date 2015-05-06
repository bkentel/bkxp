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

void bkrl::renderer::draw_filled_rect(rect_t const r)
{
    impl_->render_fill_rect(r.x, r.y, r.w, r.h);
}
