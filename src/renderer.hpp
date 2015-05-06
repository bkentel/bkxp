#pragma once

#include <memory>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {

class system;

namespace detail { class renderer_impl; }

class renderer {
public:
    struct rect_t {
        int x, y;
        int w, h;
    };

    explicit renderer(system& sys);
    ~renderer();

    void clear();
    void present();

    void draw_filled_rect(rect_t r);
private:
    std::unique_ptr<detail::renderer_impl> impl_;
};

} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////