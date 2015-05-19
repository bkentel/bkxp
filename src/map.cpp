#include "map.hpp"
#include "renderer.hpp"


void bkrl::map::draw(renderer& render) {

    for_each_cell(terrain_render_data_, [&](int const x, int const y, terrain_render_data const& cell) {
        if (cell.index) {
            render.draw_cell(x, y, cell.index);
        }
    });
}

void bkrl::map::fill(bklib::irect const r, terrain_type const value)
{
    for (int y = r.top; y < r.bottom; ++y) {
        for (int x = r.left; x < r.right; ++x) {
            auto& cell = terrain_entries_.block_at(x, y).cell_at(x, y);
            cell.type = value;
            cell.variant = 0;
        }
    }
}

void bkrl::map::fill(bklib::irect r, terrain_type const value, terrain_type const border)
{
    for (int y = r.top; y < r.bottom; ++y) {
        for (int x = r.left; x < r.right; ++x) {
            auto& cell = at(x, y);
            
            if (y == r.top  || y == r.bottom - 1
             || x == r.left || x == r.right - 1
            ) {
                cell.type = border;
            } else {
                cell.type = value;
            }

            cell.variant = 0;
        }
    }
}

void bkrl::map::update_render_data(int const x, int const y)
{
    auto& index = terrain_render_data_.block_at(x, y).cell_at(x, y).index;

    switch (terrain_entries_.block_at(x, y).cell_at(x, y).type) {
    default :
    case terrain_type::empty : index = 0;   break;
    case terrain_type::rock  : index = '*'; break;
    case terrain_type::stair : index = '>'; break;
    case terrain_type::floor : index = '.'; break;
    case terrain_type::wall  : index = '#'; break;
    }
}

void bkrl::map::update_render_data()
{
    for (int y = 0; y < size_chunk; ++y) {
        for (int x = 0; x < size_chunk; ++x) {
            update_render_data(x, y);
        }
    }
}
