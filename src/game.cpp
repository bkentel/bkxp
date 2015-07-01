#include "game.hpp"
#include "direction.hpp"

#include "bsp_layout.hpp"

#include "bklib/string.hpp"
#include "bklib/scope_guard.hpp"

//--------------------------------------------------------------------------------------------------
bkrl::game::game()
  : timer_()
  , random_()
  , system_()
  , renderer_(system_)
  , text_renderer_()
  , view_(system_.client_width(), system_.client_height(), 18, 18)
  , command_translator_()
  , creature_dictionary_ {}
  , item_dictionary_ {}
  , creature_factory_(creature_dictionary_)
  , item_factory_()
  , current_map_()

  , test_layout_ {text_renderer_, "Message.", 5, 5, 640, 200}
  , message_log_ {text_renderer_}
{
    load_definitions(item_dictionary_, "./data/items.def", load_from_file);
    load_definitions(creature_dictionary_, "./data/creatures.def", load_from_file);

    command_translator_.push_handler([&](command const& cmd) {
        return on_command(cmd);
    });

    system_.on_window_resize = [&](int const w, int const h) {
        view_.set_window_size(w, h);
    };

    system_.on_text_input = [&](bklib::utf8_string_view) {
    };

    system_.on_key_up = [&](int const key) {
        command_translator_.on_key_up(key);
    };

    system_.on_key_down = [&](int const key) {
        command_translator_.on_key_down(key);
    };

    system_.on_mouse_motion = [&](mouse_state const m) {
        if (m.is_down(mouse_button::right)) {
            on_scroll(m.dx, m.dy);
        } else {
            on_mouse_over(m.x, m.y);
        }
    };

    system_.on_mouse_scroll = [&](mouse_state const m) {
        if (m.sy > 0) {
            on_zoom(0.1, 0.1);
        } else if (m.sy < 0) {
            on_zoom(-0.1, -0.1);
        }
    };

    ////

    using namespace std::chrono_literals;
    timer_message_log_ = timer_.add(1s, [&](auto&) {
        message_log_.show(message_log::show_type::less);
    });

    generate_map();

    while (system_.is_running()) {
        system_.do_events_wait();
        timer_.update();
        render();
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::generate_map()
{
    auto& m = current_map_;
    auto const bounds = current_map_.bounds();

    bsp_layout layout {bklib::irect {bounds.left, bounds.top, bounds.right - 1, bounds.bottom - 1}};
    layout.generate(random_[random_stream::substantive], [&](bklib::irect const r) {
        bklib::irect const r0 {r.left, r.top, r.right + 1, r.bottom + 1};
        m.fill(r0, terrain_type::floor, terrain_type::wall);
        m.at(r.left + 2, r.top).type = terrain_type::door;
    });

    m.update_render_data();

    constexpr auto const skeleton_id = creature_def_id {bklib::static_djb2_hash("skeleton")};
    constexpr auto const zombie_id   = creature_def_id {bklib::static_djb2_hash("zombie")};

    for (int i = 0; i < 10; ++i) {
        generate_creature(random_, m, creature_factory_, random_definition(random_[random_stream::substantive], creature_dictionary_));
        generate_item(random_, m, item_factory_, random_definition(random_[random_stream::substantive], item_dictionary_));
    }

    creature_def def {"player"};
    def.flags.set(creature_flag::is_player);
    def.symbol.assign("@");

    bklib::ipoint2 const p {0, 0};
    m.place_creature_at(creature_factory_.create(random_, def, p), def, p);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::render()
{
    renderer_.clear();

    auto const scale = view_.get_zoom();
    auto const trans = view_.get_scroll();

    renderer_.set_scale(x(scale), y(scale));
    renderer_.set_translation(x(trans), y(trans));

    current_map_.draw(renderer_, view_);

    test_layout_.draw(renderer_);
    message_log_.draw(renderer_);

    renderer_.present();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::advance()
{
    current_map_.advance(random_);
}

//--------------------------------------------------------------------------------------------------
bkrl::creature& bkrl::game::get_player()
{
    return *current_map_.find_creature([&](creature const& c) {
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
    test_layout_.set_position(mx, my);

    auto const p = view_.screen_to_world(mx, my);
    if (p != mouse_last_pos_ && intersects(p, current_map_.bounds())) {
        debug_print(x(p), y(p));
        mouse_last_pos_ = p;
    }
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
    system_.quit();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_quit()
{
    display_message("Are you sure you want to quit? Y/N");

    query_yn(command_translator_, [this](command_type const cmd) {
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
namespace bkrl {

//--------------------------------------------------------------------------------------------------
//! Update the state of a door at p.
//! TODO add tests
//--------------------------------------------------------------------------------------------------
bool set_door_state(map& m, bklib::ipoint2 const p, door::state const state) {
    auto& ter = m.at(p);
    door d {ter};

    if (!d.set_open_close(state)) {
        return false;
    }

    ter = d;
    m.update_render_data(p);

    return true;
}

//--------------------------------------------------------------------------------------------------
//! Return a predicate which tests for doors matching state
//! TODO add tests
//--------------------------------------------------------------------------------------------------
inline decltype(auto) find_door(door::state const state) noexcept {
    return [state](terrain_entry const& ter) noexcept {
        if (ter.type != terrain_type::door) {
            return false;
        }

        door const d {ter};

        switch (state) {
        case door::state::open :   return d.is_open();
        case door::state::closed : return d.is_closed();
        }

        return false;
    };
}

} //namespace bkrl

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_open_close(bklib::ipoint2 const p, command_type const type)
{
    BK_PRECONDITION(type == command_type::open || type == command_type::close);

    auto const state = (type == command_type::open)
        ? door::state::open : door::state::closed;

    if (!set_door_state(current_map_, p, state)) {
        if (state == door::state::open) {
            display_message("Couldn't open the door.");
        } else if (state == door::state::closed) {
            display_message("Couldn't close the door.");
        }
    }

    advance();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_open_close(command_type const type)
{
    BK_PRECONDITION(type == command_type::open || type == command_type::close);

    auto& player = get_player();

    auto const p = player.position();
    auto const state = (type == command_type::open)
        ? door::state::closed : door::state::open;

    auto const candidates = find_around(current_map_, p, find_door(state));

    //
    // Nothing to do.
    //
    if (!candidates) {
        if (type == command_type::open) {
            display_message("There is nothing here to open.");
        } else if (type == command_type::close) {
            display_message("There is nothing here to close.");
        }

        return;
    }

    //
    // Ok.
    //
    if (candidates.count == 1) {
        do_open_close(candidates.p, type);
        return;
    }

    //
    // Have to choose a target.
    //
    if (type == command_type::open) {
        display_message("Open in which direction?");
    } else if (type == command_type::close) {
        display_message("Close in which direction?");
    }

    query_dir(command_translator_, [this, p, type](command_type const cmd) {
        if (cmd == command_type::cancel) {
            display_message("Nevermind.");
            return command_handler_result::detach;
        }

        if (!is_direction(cmd)) {
            display_message("Invalid choice.");
            return command_handler_result::capture;
        }

        auto const q = p + bklib::truncate<2>(direction_vector(cmd));
        if (current_map_.at(q).type != terrain_type::door) {
            if (type == command_type::open) {
                display_message("There is nothing there to open.");
            } else if (type == command_type::close) {
                display_message("There is nothing there to close.");
            }

            return command_handler_result::capture;
        }

        do_open_close(q, type);
        return command_handler_result::detach;
    });
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_get()
{
    auto& player = get_player();
    do_get(player, player.position());
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_drop()
{
    auto& player = get_player();
    do_drop(player, player.position());
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_drop(creature& subject, bklib::ipoint2 const where)
{
    if (subject.item_list().empty()) {
        display_message("You have nothing to drop.");
        return;
    }

    with_pile_at(item_dictionary_, current_map_, where, [&](item_pile& pile) {
        subject.drop_item(pile);

        if (auto const idef = item_dictionary_.find(pile.begin()->def())) {
            if (idef->name.empty()) {
                display_message("You dropped the [%s].", idef->id_string);
            } else {
                display_message("You dropped the %s.", idef->name);
            }
        } else {
            display_message("You dropped the %s.", "<UNKNOWN>");
        }
    });
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_get(creature& subject, bklib::ipoint2 const where)
{
    BK_NAMED_SCOPE_EXIT(on_fail) {
        display_message("There is nothing here to get.");
    };

    auto const p = subject.position();

    if (abs_max(where - p) > 1) {
        return;
    }

    item_pile* const pile = current_map_.items_at(where);
    if (!pile) {
        return;
    }

    on_fail.dismiss();

    if (!subject.can_get_items(*pile)) {
        return;
    }

    for (auto const& itm : *pile) {
        if (auto const idef = item_dictionary_.find(itm.def())) {
            if (idef->name.empty()) {
                display_message("You picked up the [%s].", idef->id_string);
            } else {
                display_message("You picked up the %s.", idef->name);
            }
        } else {
            display_message("You picked up the %s.", "<UNKNOWN>");
        }
    }

    subject.get_items(current_map_.remove_items_at(p));
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
    auto const ok = move_by(get_player(), current_map_, bklib::truncate<2>(v));
    if (ok) {
        advance();
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_move(bklib::ivec3 const v)
{
    BK_PRECONDITION(x(v) || y(v) && !z(v));
    do_move(v);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_show_inventory()
{
    do_show_inventory();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_show_inventory()
{
    auto const& player = get_player();

    char i = 0;
    for (auto const& itm : player.item_list()) {
        auto const& idef = item_dictionary_.find(itm.def());
        if (idef) {
            if (!idef->name.empty()) {
                display_message("[%c] %s", 'a' + i, idef->name);
            } else {
                display_message("[%c] [%s]", 'a' + i, idef->id_string);
            }
        } else {
            display_message("[%c] <UNKNOWN>", 'a' + i);
        }

        ++i;
    }
}

//--------------------------------------------------------------------------------------------------
bkrl::command_handler_result bkrl::game::on_command(command const& cmd)
{
    switch (cmd.type) {
    case command_type::zoom:
        on_zoom(cmd.data0, cmd.data0);
        break;
    case command_type::dir_here:
        do_wait(1);
        break;
    case command_type::dir_north:
    case command_type::dir_south:
    case command_type::dir_east:
    case command_type::dir_west:
    case command_type::dir_n_west:
    case command_type::dir_n_east:
    case command_type::dir_s_west:
    case command_type::dir_s_east:
    case command_type::dir_up:
    case command_type::dir_down:
        on_move(direction_vector(cmd.type));
        break;
    case command_type::open:
    case command_type::close:
        on_open_close(cmd.type);
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
    case command_type::show_inventory:
        on_show_inventory();
        break;
    default:
        break;
    }

    return command_handler_result::capture;
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::debug_print(int const mx, int const my) const
{
    bklib::ipoint2 const p {mx, my};

    auto const& ter = current_map_.at(p);
    printf("cell (%d, %d)\n", x(p), y(p));
    printf("  type = %d::%d\n", ter.type, ter.variant);

    if (auto const& c = current_map_.creature_at(p)) {
        printf("  creature present\n");

        if (auto const& cdef = creature_dictionary_.find(c->def())) {
            printf("  %s\n", cdef->id_string.c_str());
        } else {
            printf("  !!unknown creature!!\n");
        }
    }

    if (auto const& ip = current_map_.items_at(p)) {
        printf("  item(s) present\n");
        for (auto const& i : *ip) {
            if (auto const& idef = item_dictionary_.find(i.def())) {
                printf("  %s\n", idef->id_string.c_str());
            } else {
                printf("  !!unknown item!!\n");
            }
        }
    }
}
