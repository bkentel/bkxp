#include "game.hpp"

#include "bsp_layout.hpp"
#include "color.hpp"
#include "commands.hpp"
#include "context.hpp"
#include "creature.hpp"
#include "definitions.hpp"
#include "direction.hpp"
#include "inventory.hpp"
#include "json.hpp"
#include "map.hpp"
#include "message_log.hpp"
#include "output.hpp"
#include "random.hpp"
#include "renderer.hpp"
#include "system.hpp"
#include "text.hpp"
#include "view.hpp"

#include "bklib/dictionary.hpp"
#include "bklib/math.hpp"
#include "bklib/scope_guard.hpp"
#include "bklib/string.hpp"
#include "bklib/timer.hpp"

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

    void on_open();
    void on_close();

    void on_get();
    void on_drop();

    void do_wait(int turns);

    void do_move(bklib::ivec3 v);
    void on_move(bklib::ivec3 v);

    void on_show_inventory();

    void on_show_equipment();
    void do_show_equipment();

    command_handler_result on_command(command const& cmd);
    void on_command_result(command_type cmd, size_t data);

    context make_context();

    void debug_print(int x, int y);

    map&       current_map()       noexcept;
    map const& current_map() const noexcept;
private:
    void set_inspect_message_position_();
private:
    bklib::timer        timer_;
    random_state        random_;
    std::unique_ptr<system>        system_;
    std::unique_ptr<renderer>      renderer_;
    std::unique_ptr<text_renderer> text_renderer_;
    view                view_;
    std::unique_ptr<command_translator> command_translator_;
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

    key_mod_state  prev_key_mods_ = system_->current_key_mods();
    bklib::ipoint2 mouse_last_pos_ = bklib::ipoint2 {0, 0};
    bklib::ipoint2 mouse_last_pos_screen_ = bklib::ipoint2 {0, 0};

    message_log message_log_;

    bklib::timer::id_t timer_message_log_ {0};

    text_layout inspect_text_ {*text_renderer_, ""};
    bool show_inspect_text_ = false;

    context ctx_;
};

//--------------------------------------------------------------------------------------------------
template <typename T>
inline decltype(auto) random_definition(random_t& random, bklib::dictionary<T> const& dic) {
    BK_PRECONDITION(!dic.empty());
    return random_element(random, dic);
}

} //namespace bkrl

namespace {

//--------------------------------------------------------------------------------------------------
bkrl::definitions load_definitions(
    bkrl::creature_dictionary& creatures
  , bkrl::item_dictionary&     items
  , bkrl::color_dictionary&    colors
) {
    using namespace bkrl;

    load_definitions(colors,    "./data/core.colors.json",    load_from_file);
    load_definitions(items,     "./data/core.items.json",     load_from_file);
    load_definitions(creatures, "./data/core.creatures.json", load_from_file);

    return definitions {&creatures, &items, &colors};
}

//--------------------------------------------------------------------------------------------------
std::unique_ptr<bkrl::map> generate_map(bkrl::context& ctx)
{
    using namespace bkrl;
    return std::make_unique<map>(ctx);
}

} //namespace

//--------------------------------------------------------------------------------------------------
void bkrl::start_game()
{
    bkrl::game game;
}

//--------------------------------------------------------------------------------------------------
bkrl::game::game()
  : timer_()
  , random_()
  , system_ {make_system()}
  , renderer_ {make_renderer(*system_)}
  , text_renderer_ {make_text_renderer()}
  , view_(system_->client_width(), system_->client_height(), 18, 18)
  , command_translator_ {make_command_translator()}
  , color_dictionary_ {}
  , creature_dictionary_ {}
  , item_dictionary_ {}
  , definitions_ {::load_definitions(creature_dictionary_, item_dictionary_, color_dictionary_)}
  , creature_factory_()
  , item_factory_()
  , maps_ {}
  , current_map_ {nullptr}
  , output_ {}
  , inventory_ {*text_renderer_}
  , last_frame_ {std::chrono::high_resolution_clock::now()}
  , message_log_ {*text_renderer_}
  , ctx_ (make_context())
{
    //
    // set up output
    //
    output_.push([&](bklib::utf8_string_view const str) {
        display_message(str);
    });

    //
    // set text colors
    //
    text_renderer_->set_colors(&color_dictionary_);

    //
    // set up initial map
    //
    maps_.emplace_back(::generate_map(ctx_));
    current_map_ = maps_.back().get();
    generate_map();

    command_translator_->push_handler([&](command const& cmd) {
        return on_command(cmd);
    });

    command_translator_->set_command_result_handler([&](command_type const cmd, size_t const data) {
        on_command_result(cmd, data);
    });

    system_->on_window_resize = [&](int const w, int const h) {
        view_.set_window_size(w, h);
    };

    system_->on_text_input = [&](bklib::utf8_string_view const str) {
        command_translator_->on_text(str);
    };

    auto const check_key_mods_changed = [&] {
        auto const cur = system_->current_key_mods();
        if (cur == prev_key_mods_) {
            return;
        }

        BK_SCOPE_EXIT { prev_key_mods_ = cur; };

        show_inspect_text_ = cur.test(key_mod::shift);
        if (show_inspect_text_) {
            set_inspect_message_position_();
        }
    };

    system_->on_key_up = [&](int const key) {
        check_key_mods_changed();
        command_translator_->on_key_up(key, prev_key_mods_);
    };

    system_->on_key_down = [&](int const key) {
        check_key_mods_changed();
        command_translator_->on_key_down(key, prev_key_mods_);
    };

    system_->on_mouse_motion = [&](mouse_state const m) {
        if (inventory_.mouse_move(m)) {
            return;
        }

        if (m.is_down(mouse_button::right)) {
            on_scroll(m.dx, m.dy);
        } else {
            on_mouse_over(m.x, m.y);
        }
    };

    system_->on_mouse_scroll = [&](mouse_state const m) {
        if (inventory_.mouse_scroll(m)) {
            return;
        }

        if (m.sy > 0) {
            on_zoom(0.1, 0.1);
        } else if (m.sy < 0) {
            on_zoom(-0.1, -0.1);
        }
    };

    system_->on_mouse_button = [&](mouse_button_state const m) {
        if (inventory_.mouse_button(m)) {
            return;
        }
    };

    system_->on_request_quit = [&] {
        on_quit();
        return !system_->is_running();
    };

    ////

    using namespace std::chrono_literals;
    timer_message_log_ = timer_.add(1s, [&](auto&) {
        message_log_.show(message_log::show_type::less);
    });

    while (system_->is_running()) {
        system_->do_events_wait();
        timer_.update();
        render();
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::generate_map()
{
    auto& m = current_map();
    auto& random = random_[random_stream::substantive];

    generate_creature(ctx_, m, random_definition(random, creature_dictionary_), bklib::ipoint2 {2, 2});

    creature_def def {"player"};
    def.flags.set(creature_flag::is_player);
    def.symbol.assign("@");

    bklib::ipoint2 const p {0, 0};
    m.place_creature_at(creature_factory_.create(random, def, p), def, p);

    // For debugging inventory
    auto& player = *m.creature_at(p);

    for (auto i = 0; i < 20; ++i) {
        player.get_item(item_factory_.create(
            random
          , *ctx_.data.random_item(random_, random_stream::substantive)
        ));
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::render(render_type const type)
{
    using namespace std::chrono_literals;
    constexpr auto const frame_time = std::chrono::duration_cast<std::chrono::nanoseconds>(1s) / 60;

    auto const now = std::chrono::high_resolution_clock::now();
    if (type == render_type::wait && now < last_frame_ + frame_time) {
        return;
    }

    last_frame_ = now;

    renderer_->clear();

    auto const scale = view_.get_zoom();
    auto const trans = view_.get_scroll();

    renderer_->set_scale(x(scale), y(scale));
    renderer_->set_translation(x(trans), y(trans));

    current_map().draw(*renderer_, view_);

    message_log_.draw(*renderer_);
    inventory_.draw(*renderer_);

    if (show_inspect_text_) {
        auto const r = make_renderer_rect(add_border(inspect_text_.extent(), 4));
        renderer_->draw_filled_rect(r, make_color(200, 200, 200, 200));
        inspect_text_.draw(*renderer_);
    }

    renderer_->present();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::advance()
{
    bkrl::advance(ctx_, current_map());
    render(render_type::force_update);
}

//--------------------------------------------------------------------------------------------------
bkrl::creature& bkrl::game::get_player()
{
    return *current_map().find_creature([&](creature const& c) {
        return c.is_player();
    });
}

//--------------------------------------------------------------------------------------------------
bkrl::creature const& bkrl::game::get_player() const
{
    return const_cast<game*>(this)->get_player();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::display_message(bklib::utf8_string_view const msg) {
    printf("%s\n", msg.data());

    timer_.reset(timer_message_log_);
    message_log_.println(msg);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_mouse_over(int const x, int const y)
{
    do_mouse_over(x, y);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_mouse_over(int const mx, int const my)
{
    mouse_last_pos_screen_ = bklib::ipoint2 {mx, my};

    auto const p = view_.screen_to_world(mx, my);

    if (p == mouse_last_pos_) {
        if (show_inspect_text_) {
            set_inspect_message_position_();
        }
        return;
    }

    if (!intersects(p, current_map().bounds())) {
        return;
    }

    debug_print(x(p), y(p));

    mouse_last_pos_ = p;
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_zoom(double const zx, double const zy)
{
    do_zoom(zx, zy);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_zoom(double const zx, double const zy)
{
    if (zx > 0) {
        view_.zoom_in();
    } else if (zx < 0) {
        view_.zoom_out();
    }

    if (zy > 0) {
        view_.zoom_in();
    } else if (zy < 0) {
        view_.zoom_out();
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_scroll(double const dx, double const dy)
{
    do_scroll(dx, dy);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_scroll(double const dx, double const dy)
{
    view_.scroll_by_screen(dx, dy);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_quit()
{
    system_->quit();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_quit()
{
    display_message("Are you sure you want to quit? Y/N");

    query_yn(*command_translator_, [this](command_type const cmd) {
        if (cmd == command_type::yes) {
            do_quit();
            return command_handler_result::detach;
        } else if (cmd ==  command_type::no) {
            display_message("Ok.");
            return command_handler_result::detach;
        } else if (cmd ==  command_type::cancel) {
            display_message("Canceled.");
            return command_handler_result::detach;
        }

        display_message("Invalid choice.");
        return command_handler_result::capture;
    });
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_open()
{
    bkrl::open(ctx_, get_player(), current_map(), *command_translator_);
    advance(); // TODO not strictly correct
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_close()
{
    bkrl::close(ctx_, get_player(), current_map(), *command_translator_);
    advance(); // TODO not strictly correct
}

//--------------------------------------------------------------------------------------------------
bkrl::open_result_t bkrl::open_door_at(
    context&             ctx
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
) {
    if (!set_door_state(current_map, where, door::state::open)) {
        ctx.out.write("Couldn't open the door.");
        return open_result::failed;
    }

    ctx.out.write("You opened the door.");
    return open_result::ok;
}

//--------------------------------------------------------------------------------------------------
bkrl::open_result_t bkrl::open_cont_at(
    context&             ctx
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
) {
    item_pile* const pile = current_map.items_at(where);
    BK_PRECONDITION(pile);

    item* const itm = bklib::find_maybe(*pile, find_by_flag(item_flag::is_container));
    BK_PRECONDITION(itm);

    ctx.out.write("(TODO) You opened the %s.", itm->friendly_name(ctx));
    return open_result::ok;
}

//--------------------------------------------------------------------------------------------------
bkrl::open_result_t bkrl::open(
    context&            ctx
  , creature&           subject
  , map&                current_map
  , command_translator& commands
) {
    auto const p = subject.position();
    auto const door_candidates = find_around(current_map, p, find_door(door::state::closed));
    auto const item_candidates = find_items_around(current_map, p, find_container());

    //
    // Nothing to do.
    //
    if (!door_candidates && !item_candidates) {
        ctx.out.write("There is nothing here to open.");
        return open_result::nothing;
    }

    //
    // Ok.
    //
    if (door_candidates.count + item_candidates.count == 1) {
        return door_candidates
          ? open_door_at(ctx, subject, current_map, door_candidates.p)
          : open_cont_at(ctx, subject, current_map, item_candidates.p);
    }

    //
    // Have to choose a target.
    //
    ctx.out.write("Which direction?");

    query_dir(commands, [&, p, door_candidates, item_candidates](command_type const cmd) {
        if (cmd == command_type::cancel) {
            ctx.out.write("Nevermind.");
            set_command_result(commands, open_result::canceled);
            return command_handler_result::detach;
        }

        if (!is_direction(cmd)) {
            ctx.out.write("Invalid choice.");
            set_command_result(commands, open_result::select);
            return command_handler_result::capture;
        }

        auto const v = bklib::truncate<2>(direction_vector(cmd));
        bool const is_door = door_candidates.is_valid(v);
        bool const is_cont = item_candidates.is_valid(v);

        if (is_door && is_cont) {
            BK_ASSERT(false && "TODO");
        } else if (is_door) {
            set_command_result(commands
              , open_door_at(ctx, subject, current_map, p + v));
        } else if (is_cont) {
            set_command_result(commands
              , open_cont_at(ctx, subject, current_map, p + v));
        } else {
            ctx.out.write("There is nothing there to open.");
            set_command_result(commands, open_result::nothing);
        }

        return command_handler_result::detach;
    });

    return open_result::select;
}

//--------------------------------------------------------------------------------------------------
bkrl::close_result_t bkrl::close_door_at(
    context&             ctx
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
) {
    if (!set_door_state(current_map, where, door::state::closed)) {
        ctx.out.write("Couldn't close the door.");
        return close_result::failed;
    }

    ctx.out.write("You closed the door.");
    return close_result::ok;
}

//--------------------------------------------------------------------------------------------------
bkrl::close_result_t bkrl::close_cont_at(
    context&             ctx
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
) {
    item_pile* const pile = current_map.items_at(where);
    BK_PRECONDITION(pile);

    item* const itm = bklib::find_maybe(*pile, find_by_flag(item_flag::is_container));
    BK_PRECONDITION(itm);

    ctx.out.write("(TODO) You closed the %s.", itm->friendly_name(ctx));
    return close_result::ok;
}

//--------------------------------------------------------------------------------------------------
bkrl::close_result_t bkrl::close(
    context&            ctx
  , creature&           subject
  , map&                current_map
  , command_translator& commands
) {
    auto const p = subject.position();
    auto const door_candidates = find_around(current_map, p, find_door(door::state::open));
    auto const item_candidates = find_items_around(current_map, p, find_container());

    //
    // Nothing to do.
    //
    if (!door_candidates && !item_candidates) {
        ctx.out.write("There is nothing here to close.");
        return close_result::nothing;
    }

    //
    // Ok.
    //
    if (door_candidates.count + item_candidates.count == 1) {
        return door_candidates
          ? close_door_at(ctx, subject, current_map, door_candidates.p)
          : close_cont_at(ctx, subject, current_map, item_candidates.p);
    }

    //
    // Have to choose a target.
    //
    ctx.out.write("Which direction?");

    query_dir(commands, [&, p, door_candidates, item_candidates](command_type const cmd) {
        if (cmd == command_type::cancel) {
            ctx.out.write("Nevermind.");
            set_command_result(commands, close_result::canceled);
            return command_handler_result::detach;
        }

        if (!is_direction(cmd)) {
            ctx.out.write("Invalid choice.");
            set_command_result(commands, close_result::select);
            return command_handler_result::capture;
        }

        auto const v = bklib::truncate<2>(direction_vector(cmd));
        bool const is_door = door_candidates.is_valid(v);
        bool const is_cont = item_candidates.is_valid(v);

        if (is_door && is_cont) {
            BK_ASSERT(false && "TODO");
        } else if (is_door) {
            set_command_result(commands
              , close_door_at(ctx, subject, current_map, p + v));
        } else if (is_cont) {
            set_command_result(commands
              , close_cont_at(ctx, subject, current_map, p + v));
        } else {
            ctx.out.write("There is nothing there to close.");
            set_command_result(commands, close_result::nothing);
        }

        return command_handler_result::detach;
    });

    return close_result::select;
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_get()
{
    auto& subject = get_player();
    get_item(ctx_, subject, current_map(), subject.position(), inventory_, *command_translator_);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_drop()
{
    auto& subject = get_player();
    drop_item(ctx_, subject, current_map(), subject.position(), inventory_, *command_translator_);
}

//--------------------------------------------------------------------------------------------------
bkrl::drop_item_result_t bkrl::drop_item(
    context&             ctx
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
  , inventory&           imenu
  , command_translator&  commands
) {
    auto& items = subject.item_list();

    if (items.empty()) {
        ctx.out.write("You have nothing to drop.");
        return drop_item_result::no_items;
    }

    if (abs_max(where - subject.position()) > 1) {
        ctx.out.write("You can't drop that there from here.");
        return drop_item_result::out_of_range;
    }

    imenu.on_action([&, where](inventory::action const type, int const sel) {
        BK_NAMED_SCOPE_EXIT(close) {
            imenu.show(false);
            commands.pop_handler(); //TODO
        };

        if (sel < 0 || type == inventory::action::cancel) {
            set_command_result(commands, drop_item_result::canceled);
            return;
        }

        if (type != inventory::action::confirm) {
            close.dismiss();
            return;
        }

        auto const result = with_pile_at(ctx.data, current_map, where, [&](item_pile& pile) {
            subject.drop_item(pile, from_inventory_data(imenu.data()).second);
            ctx.out.write("You dropped the %s.", pile.begin()->friendly_name(ctx));
        });

        if (!result) {
            ctx.out.write("You can't drop that here.");
            set_command_result(commands, drop_item_result::failed);
            return;
        }

        set_command_result(commands, drop_item_result::ok);
    });

    commands.push_handler(make_item_list(ctx, imenu, items, "Drop which item?"));
    return drop_item_result::select;
}

//--------------------------------------------------------------------------------------------------
bkrl::get_item_result_t bkrl::get_item(
    context&             ctx
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
  , inventory&           imenu
  , command_translator&  commands
) {
    if (abs_max(where - subject.position()) > 1) {
        ctx.out.write("You can't get that from here.");
        return get_item_result::out_of_range;
    }

    item_pile* const pile = current_map.items_at(where);
    if (!pile) {
        ctx.out.write("There is nothing here to get.");
        return get_item_result::no_items;
    }

    imenu.on_action([&, where](inventory::action const type, int const i) {
        BK_NAMED_SCOPE_EXIT(close) {
            imenu.show(false);
            commands.pop_handler(); //TODO
        };

        if (i < 0 || type == inventory::action::cancel) {
            set_command_result(commands, get_item_result::canceled);
            return;
        }

        if (type != inventory::action::confirm) {
            close.dismiss();
            return;
        }

        auto const data = from_inventory_data(imenu.data());
        auto&      itm   = data.first;
        auto const index = data.second;

        if (!subject.can_get_item(itm)) {
            ctx.out.write("You can't get the %s.", itm.friendly_name(ctx));
            set_command_result(commands, get_item_result::failed);
            return;
        }

        ctx.out.write("You got the %s.", itm.friendly_name(ctx));
        subject.get_item(current_map.remove_item_at(where, index));
        set_command_result(commands, get_item_result::ok);
    });

    commands.push_handler(make_item_list(ctx, imenu, *pile, "Get which item?"));
    return get_item_result::select;
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_wait(int const turns)
{
    for (int i = turns; i > 0; --i) {
        advance();
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_move(bklib::ivec3 const v)
{
    auto const p = get_player().position();

    auto const ok = move_by(ctx_, get_player(), current_map(), bklib::truncate<2>(v));
    if (!ok) {
        return;
    }

    advance();

    auto const q = get_player().position();
    if (!distance2(p, q)) {
        return;
    }

    if (auto const pile = current_map().items_at(q)) {
        for (auto const& i : *pile) {
            display_message("You see here %s.", i.friendly_name(ctx_));
        }
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_move(bklib::ivec3 const v)
{
    BK_PRECONDITION((x(v) || y(v)) && !z(v));
    do_move(v);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_show_inventory()
{
    auto const visible = !inventory_.is_visible();
    inventory_.show(visible);

    if (visible) {
        show_inventory(ctx_, get_player(), inventory_, *command_translator_);
    }
}

//--------------------------------------------------------------------------------------------------
bkrl::show_inventory_result_t bkrl::show_inventory(
    context&            ctx
  , creature&           subject
  , inventory&          imenu
  , command_translator& commands
)
{
    auto& items = subject.item_list();
    if (items.empty()) {
        ctx.out.write("You have no items.");
        return show_inventory_result::no_items;
    }

    imenu.on_action([&](inventory::action const type, int const i) {
        BK_NAMED_SCOPE_EXIT(close) {
            imenu.show(false);
            commands.pop_handler(); //TODO
        };

        if (i < 0 || type == inventory::action::cancel) {
            set_command_result(commands, show_inventory_result::canceled);
            return;
        }

        close.dismiss();

        auto& itm = from_inventory_data(imenu.data()).first;

        if (type == inventory::action::confirm) {
            ctx.out.write("You chose the %d%s item -- %s"
              , i + 1
              , bklib::oridinal_suffix(i + 1).data()
              , itm.friendly_name(ctx));
        } else if (type == inventory::action::equip) {
            set_command_result(commands, equip_item(ctx, subject, itm));
        } else {
            return;
        }

        set_command_result(commands, show_inventory_result::ok);
    });

    commands.push_handler(make_item_list(ctx, imenu, items, "Items"));
    return show_inventory_result::select;
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_show_equipment()
{
    do_show_equipment();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_show_equipment()
{
}

//--------------------------------------------------------------------------------------------------
bkrl::equip_item_result_t bkrl::equip_item(
    context&  ctx
  , creature& subject
  , item&     itm
) {
    using status_t = equipment::result_t::status_t;

    if (!subject.has_item(itm)) {
        return bkrl::equip_result_t::not_held;
    }

    auto& eq = subject.equip_list();
    auto const result = subject.equip_item(itm);

    auto flags = item::format_flags {};
    flags.definite = true;
    flags.use_color = true;

    fmt::MemoryWriter out;

    auto const print_slots = [&](item_slots const slots) {
        BK_PRECONDITION(slots.any());

        flags.definite   = false;
        flags.capitalize = false;

        auto const print_slot = [&](equipment::slot_t const* const slot) {
            if (!slot) {
                return;
            }

            BK_PRECONDITION(slot->itm);

            out.write(" Your <color=r>{}</color> is occupied by {}."
                , slot->name
                , slot->itm->friendly_name(ctx, flags));
        };

        for_each_flag(slots, [&](equip_slot const es) {
            if (es == equip_slot::hand_any) {
                out.write(" All of your <color=r>hands</color> are occupied.");

                print_slot(eq.slot_info(equip_slot::hand_main));
                print_slot(eq.slot_info(equip_slot::hand_off));
            } else {
                print_slot(eq.slot_info(es));
            }
        });
    };

    auto const iname = [&] {
        return itm.friendly_name(ctx, flags);
    };

    switch (result.status) {
    case status_t::ok:
        out.write("You equip {}.", iname());
        print_slots(eq.where(itm));
        break;
    case status_t::not_equippable:
        flags.capitalize = true;
        out.write("{} cannot be equipped.", iname());
        break;
    case status_t::slot_occupied:
        flags.capitalize = true;
        out.write("{} can't be equipped.", iname());
        print_slots(result.slots());
        break;
    case status_t::slot_not_present:
        out.write("You can't seem to find an appropriate place to equip %s.", iname());
        break;
    case status_t::already_equipped: {
        out.write("You already have {} equipped.", iname());
        print_slots(eq.where(itm));
        break;
    }
    case status_t::slot_empty: BK_FALLTHROUGH
    default:
        break;
    }

    ctx.out.write(out.c_str());
    return result.status;
}

//--------------------------------------------------------------------------------------------------
bkrl::command_handler_result bkrl::game::on_command(command const& cmd)
{
    switch (cmd.type) {
    case command_type::none:    break;
    case command_type::raw:     break;
    case command_type::text:    break;
    case command_type::confirm: break;
    case command_type::invalid: break;
    case command_type::scroll:  break;
    case command_type::cancel:  break;
    case command_type::yes:     break;
    case command_type::no:      break;
    case command_type::use:     break;
    case command_type::zoom:
        on_zoom(cmd.data0, cmd.data0);
        break;
    case command_type::dir_here:
        do_wait(1);
        break;
    case command_type::dir_north:  BK_FALLTHROUGH
    case command_type::dir_south:  BK_FALLTHROUGH
    case command_type::dir_east:   BK_FALLTHROUGH
    case command_type::dir_west:   BK_FALLTHROUGH
    case command_type::dir_n_west: BK_FALLTHROUGH
    case command_type::dir_n_east: BK_FALLTHROUGH
    case command_type::dir_s_west: BK_FALLTHROUGH
    case command_type::dir_s_east: BK_FALLTHROUGH
    case command_type::dir_up:     BK_FALLTHROUGH
    case command_type::dir_down:
        on_move(direction_vector(cmd.type));
        break;
    case command_type::open:
        on_open();
        break;
    case command_type::close:
        on_close();
        break;
    case command_type::quit:
        on_quit();
        break;
    case command_type::get:
        on_get();
        break;
    case command_type::drop:
        on_drop();
        break;
    case command_type::show_equipment:
        on_show_equipment();
        break;
    case command_type::show_inventory:
        on_show_inventory();
        break;
    default:
        break;
    }

    return command_handler_result::capture;
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_command_result(command_type const cmd, size_t const data)
{
    switch (cmd) {
    case command_type::none:    break;
    case command_type::raw:     break;
    case command_type::text:    break;
    case command_type::confirm: break;
    case command_type::invalid: break;
    case command_type::scroll:  break;
    case command_type::cancel:  break;
    case command_type::yes:     break;
    case command_type::no:      break;
    case command_type::use:     break;
    case command_type::zoom:    break;
    case command_type::dir_here:   BK_FALLTHROUGH
    case command_type::dir_north:  BK_FALLTHROUGH
    case command_type::dir_south:  BK_FALLTHROUGH
    case command_type::dir_east:   BK_FALLTHROUGH
    case command_type::dir_west:   BK_FALLTHROUGH
    case command_type::dir_n_west: BK_FALLTHROUGH
    case command_type::dir_n_east: BK_FALLTHROUGH
    case command_type::dir_s_west: BK_FALLTHROUGH
    case command_type::dir_s_east: BK_FALLTHROUGH
    case command_type::dir_up:     BK_FALLTHROUGH
    case command_type::dir_down:
        break;
    case command_type::open:
        if (static_cast<open_result>(data) == open_result::ok) {
            advance();
        }
        break;
    case command_type::close:
        if (static_cast<close_result>(data) == close_result::ok) {
            advance();
        }
        break;
    case command_type::quit:
        break;
    case command_type::get:
        if (static_cast<get_item_result>(data) == get_item_result::ok) {
            advance();
        }
        break;
    case command_type::drop:
        if (static_cast<drop_item_result>(data) == drop_item_result::ok) {
            advance();
        }
        break;
    case command_type::show_equipment:
        break;
    case command_type::show_inventory:
        break;
    default:
        break;
    }
}

//--------------------------------------------------------------------------------------------------
bkrl::context bkrl::game::make_context()
{
    return {random_, definitions_, output_, item_factory_, creature_factory_};
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::set_inspect_message_position_()
{
    auto const result = move_rect_inside(
        bklib::irect {0, 0, system_->client_width(), system_->client_height()}
      , translate_to(add_border(inspect_text_.extent(), 4)
                   , x(mouse_last_pos_screen_), y(mouse_last_pos_screen_)));

    if (!result.second) {
        BK_ASSERT(false && "TODO");
    }

    inspect_text_.set_position(result.first.left, result.first.top);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::debug_print(int const mx, int const my)
{
    if (!system_->current_key_mods().test(key_mod::shift)) {
        return;
    }

    inspect_text_.set_text(*text_renderer_
      , inspect_tile(ctx_, get_player(), current_map(), bklib::ipoint2 {mx, my}));

    set_inspect_message_position_();
    show_inspect_text_ = true;
}

//--------------------------------------------------------------------------------------------------
bklib::utf8_string bkrl::inspect_tile(
    context&              ctx
  , creature       const& subject
  , map            const& current_map
  , bklib::ipoint2 const  where
) {
    if (!intersects(current_map.bounds(), where)) {
        return {};
    }

    fmt::MemoryWriter out;

    auto const& ter = current_map.at(where);
    out.write("[{}, {}] = ({}:{})",
        x(where), y(where), static_cast<uint16_t>(ter.type), ter.variant);

    if (auto const& c = current_map.creature_at(where)) {
        out.write("\n"
            "Creature ({})\n"
            " Def : {} ({:0x})\n"
            " Name: {}"
          , static_cast<uint32_t>(c->id())
          , c->def().c_str(), static_cast<uint32_t>(c->def())
          , c->friendly_name(ctx.data)
        );
    }

    if (auto const& ip = current_map.items_at(where)) {
        out.write("\nItem(s)");
        for (auto const& i : *ip) {
            out.write("\n {}", i.friendly_name(ctx));
        }
    }

    return out.str();
}

//--------------------------------------------------------------------------------------------------
bkrl::map& bkrl::game::current_map() noexcept
{
    BK_PRECONDITION(current_map_);
    return *current_map_;
}

//--------------------------------------------------------------------------------------------------
bkrl::map const& bkrl::game::current_map() const noexcept
{
    BK_PRECONDITION(current_map_);
    return *current_map_;
}

namespace {
template <typename T>
inline void set_command_result(bkrl::command_translator& commands, bkrl::command_type const cmd, T const result)
{
    static_assert(std::is_enum<T>::value, "");
    static_assert(sizeof(T) <= sizeof(size_t), "");
    commands.on_command_result(cmd, static_cast<size_t>(result));
}
} //namespace

void bkrl::set_command_result(command_translator& commands, get_item_result const result) {
    ::set_command_result(commands, command_type::get, result);
}

void bkrl::set_command_result(command_translator& commands, drop_item_result const result) {
    ::set_command_result(commands, command_type::drop, result);
}

void bkrl::set_command_result(command_translator& commands, show_inventory_result const result) {
    ::set_command_result(commands, command_type::show_inventory, result);
}

void bkrl::set_command_result(command_translator& commands, open_result const result) {
    ::set_command_result(commands, command_type::open, result);
}

void bkrl::set_command_result(command_translator& commands, close_result const result) {
    ::set_command_result(commands, command_type::close, result);
}

void bkrl::set_command_result(command_translator& commands, equip_result_t const result) {
    ::set_command_result(commands, command_type::show_equipment, result);
}
