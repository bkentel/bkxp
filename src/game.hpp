#pragma once

#include "system.hpp"
#include "renderer.hpp"
#include "text.hpp"
#include "message_log.hpp"
#include "commands.hpp"
#include "creature.hpp"
#include "map.hpp"
#include "random.hpp"
#include "view.hpp"

#include "bklib/string.hpp"
#include "bklib/math.hpp"
#include "bklib/timer.hpp"

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

    void on_mouse_over(int x, int y);
    void do_mouse_over(int x, int y);

    void on_zoom(double zx, double zy);
    void do_zoom(double zx, double zy);

    void on_scroll(double dx, double dy);
    void do_scroll(double dx, double dy);

    void do_quit();
    void on_quit();

    void on_open_close(command_type type);
    void do_open_close(bklib::ipoint2 p, command_type type);

    void on_get();
    void do_get();

    void do_move(bklib::ivec3 v);
    void on_move(bklib::ivec3 v);

    void on_command(command const& cmd);

    void debug_print(int x, int y) const;
private:
    bklib::timer        timer_;
    random_state        random_;
    system              system_;
    renderer            renderer_;
    text_renderer       text_renderer_;
    view                view_;
    command_translator  command_translator_;
    creature_dictionary creature_dictionary_;
    item_dictionary     item_dictionary_;
    creature_factory    creature_factory_;
    item_factory        item_factory_;
    map                 current_map_;
    creature            player_;

    bklib::ipoint2 mouse_last_pos_ {0, 0};

    text_layout test_layout_;
    message_log message_log_;

    bklib::timer::id_t timer_message_log_ {0};
};

} //namespace bkrl
