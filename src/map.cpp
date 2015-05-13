#include "map.hpp"
#include "renderer.hpp"

void bkrl::map::move_by(creature& critter, int dx, int dy) {
}

void bkrl::map::draw(renderer& render) {

    for_each_cell(base_, [&](int const x, int const y, map_cell_t const& cell) {
        render.draw_cell(x, y, 10);
    });
}
