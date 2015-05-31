#pragma once

#include "system.hpp"
#include "renderer.hpp"
#include "commands.hpp"
#include "creature.hpp"
#include "map.hpp"
#include "random.hpp"
#include "view.hpp"

#include "bklib/string.hpp"
#include "bklib/math.hpp"

namespace bkrl {
//--------------------------------------------------------------------------------------------------
// Game simulation state.
//--------------------------------------------------------------------------------------------------
class game {
public:
    game();

    void generate_map();

    void render();
    void advance();

    void display_message(bklib::utf8_string_view msg);

    void on_zoom(double zx, double zy);
    void do_zoom(double zx, double zy);

    void on_scroll(double dx, double dy);
    void do_scroll(double dx, double dy);

    void do_quit();
    void on_quit();

    void do_open();
    void on_open();

    void do_move(bklib::ivec3 v);
    void on_move(bklib::ivec3 v);

    void on_command(command const& cmd);
private:
    random_state       random_;
    system             system_;
    renderer           renderer_;
    view               view_;
    command_translator command_translator_;
    creature_factory   creature_factory_;
    item_factory       item_factory_;
    map                current_map_;
    creature           player_;
};

} //namespace bkrl
