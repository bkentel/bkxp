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
#include "color.hpp"
#include "definitions.hpp"
#include "output.hpp"
#include "inventory.hpp"

#include "bklib/string.hpp"
#include "bklib/math.hpp"
#include "bklib/timer.hpp"
#include "bklib/dictionary.hpp"

namespace bkrl {

//--------------------------------------------------------------------------------------------------
// Game simulation state.
//--------------------------------------------------------------------------------------------------
class game {
public:
    game();

    void generate_map();

    enum class render_type {
        wait, force_update
    };

    void render(render_type type = render_type::wait);
    void advance();

    creature& get_player();
    creature const& get_player() const;

    void display_message(bklib::utf8_string_view msg);

    template <typename Arg0, typename... Args>
    void display_message(bklib::utf8_string_view const format, Arg0 const& arg0, Args const&... args) {
        output_.write(format, arg0, args...);
    }

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
    void do_get(creature& subject, bklib::ipoint2 where);

    void on_drop();
    void do_drop(creature& subject, bklib::ipoint2 where);

    void do_wait(int turns);

    void do_move(bklib::ivec3 v);
    void on_move(bklib::ivec3 v);

    void on_show_inventory();
    void do_show_inventory();

    void on_show_equipment();
    void do_show_equipment();
    void do_equip_item(item& i);

    command_handler_result on_command(command const& cmd);

    context make_context();

    void debug_print(int x, int y);

    map&       current_map()       noexcept;
    map const& current_map() const noexcept;
private:
    bklib::timer        timer_;
    random_state        random_;
    system              system_;
    renderer            renderer_;
    text_renderer       text_renderer_;
    view                view_;
    command_translator  command_translator_;
    color_dictionary    color_dictionary_;
    creature_dictionary creature_dictionary_;
    item_dictionary     item_dictionary_;
    definitions         definitions_;
    creature_factory    creature_factory_;
    item_factory        item_factory_;
    std::vector<std::unique_ptr<map>> maps_;
    map*                current_map_;
    output              output_;
    inventory           inventory_;

    std::chrono::high_resolution_clock::time_point last_frame_;

    key_mod_state  prev_key_mods_ = system_.current_key_mods();
    bklib::ipoint2 mouse_last_pos_ = bklib::ipoint2 {0, 0};
    bklib::ipoint2 mouse_last_pos_screen_ = bklib::ipoint2 {0, 0};

    message_log message_log_;

    bklib::timer::id_t timer_message_log_ {0};

    text_layout inspect_text_ {text_renderer_, ""};
    bool show_inspect_text_ = false;
};

//--------------------------------------------------------------------------------------------------
template <typename T>
inline decltype(auto) random_definition(random_t& random, bklib::dictionary<T> const& dic) {
    BK_PRECONDITION(!dic.empty());
    return random_element(random, dic);
}

} //namespace bkrl
