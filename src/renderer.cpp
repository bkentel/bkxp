#include "renderer.hpp"

namespace bkrl { class renderer_stub_impl; }

class bkrl::renderer_stub_impl final : public renderer {
public:
    virtual ~renderer_stub_impl();
    explicit renderer_stub_impl(system&) {}

    void clear_clip_region() override final { }
    void set_clip_region(rect_t) override final { }
    rect_t get_clip_region() override final { return {}; }

    void set_scale(double, double) override final { }
    void set_scale(double) override final { }
    void set_translation(double, double) override final { }

    bklib::point_t<2, double> get_scale() const override final { return {}; }
    bklib::point_t<2, double> get_translation() const override final { return {}; }

    void clear() override final { }
    void present() override final { }

    void set_active_texture(texture) override final { }

    void draw_filled_rect(rect_t) override final { }
    void draw_filled_rect(rect_t, color4) override final { }

    void draw_cell(int, int, int) override final { }
    void draw_cell(int, int, int, color4) override final { }

    void draw_rect(rect_t, rect_t) override final { }

    void draw_cells(int, int, size_t, size_t, void const*, ptrdiff_t
                  , size_t, size_t) override final { }
    void draw_rects(int, int, size_t, void const*, ptrdiff_t, size_t
                  , ptrdiff_t, size_t, ptrdiff_t, size_t, size_t) override final { }
};

bkrl::renderer::~renderer() {
}

bkrl::renderer_stub_impl::~renderer_stub_impl() {
}

#if defined(BK_NO_SDL)
std::unique_ptr<bkrl::renderer> bkrl::make_renderer(system& sys) {
    return std::make_unique<renderer_stub_impl>(sys);
}
#endif // BK_NO_SDL
