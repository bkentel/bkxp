#include "game.hpp"
#include "direction.hpp"
#include "json.hpp"
#include "context.hpp"

#include "bsp_layout.hpp"

#include "bklib/string.hpp"
#include "bklib/scope_guard.hpp"

namespace {

//--------------------------------------------------------------------------------------------------
bkrl::definitions load_definitions(
    bkrl::creature_dictionary& creatures
  , bkrl::item_dictionary&     items
  , bkrl::color_dictionary&    colors
) {
    using namespace bkrl;

    load_definitions(colors,    "./data/colors.def",    load_from_file);
    load_definitions(items,     "./data/items.def",     load_from_file);
    load_definitions(creatures, "./data/creatures.def", load_from_file);

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
bkrl::game::game()
  : timer_()
  , random_()
  , system_()
  , renderer_(system_)
  , text_renderer_()
  , view_(system_.client_width(), system_.client_height(), 18, 18)
  , command_translator_()
  , color_dictionary_ {}
  , creature_dictionary_ {}
  , item_dictionary_ {}
  , definitions_ {::load_definitions(creature_dictionary_, item_dictionary_, color_dictionary_)}
  , creature_factory_()
  , item_factory_()
  , maps_ {}
  , current_map_ {nullptr}
  , output_ {}
  , inventory_ {text_renderer_}
  , last_frame_ {std::chrono::high_resolution_clock::now()}

  , test_layout_ {text_renderer_, "Message.", 5, 5, 640, 200}
  , message_log_ {text_renderer_}
{
    //
    // set up output
    //
    output_.push([&](bklib::utf8_string_view const str) {
        display_message(str);
    });

    //
    // set up initial map
    //
    auto ctx = make_context();
    maps_.emplace_back(::generate_map(ctx));
    current_map_ = maps_.back().get();
    generate_map();

    command_translator_.push_handler([&](command const& cmd) {
        return on_command(cmd);
    });

    system_.on_window_resize = [&](int const w, int const h) {
        view_.set_window_size(w, h);
    };

    system_.on_text_input = [&](bklib::utf8_string_view const str) {
        command_translator_.on_text(str);
    };

    system_.on_key_up = [&](int const key) {
        command_translator_.on_key_up(key);
    };

    system_.on_key_down = [&](int const key) {
        command_translator_.on_key_down(key);
    };

    system_.on_mouse_motion = [&](mouse_state const m) {
        if (inventory_.mouse_move(m)) {
            return;
        }

        if (m.is_down(mouse_button::right)) {
            on_scroll(m.dx, m.dy);
        } else {
            on_mouse_over(m.x, m.y);
        }
    };

    system_.on_mouse_scroll = [&](mouse_state const m) {
        if (inventory_.mouse_scroll(m)) {
            return;
        }

        if (m.sy > 0) {
            on_zoom(0.1, 0.1);
        } else if (m.sy < 0) {
            on_zoom(-0.1, -0.1);
        }
    };

    system_.on_mouse_button = [&](mouse_button_state const m) {
        if (inventory_.mouse_button(m)) {
            return;
        }
    };

    system_.on_request_quit = [&] {
        on_quit();
        return !system_.is_running();
    };

    ////

    using namespace std::chrono_literals;
    timer_message_log_ = timer_.add(1s, [&](auto&) {
        message_log_.show(message_log::show_type::less);
    });

    while (system_.is_running()) {
        system_.do_events_wait();
        timer_.update();
        render();
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::generate_map()
{
    auto ctx = make_context();

    auto& m = current_map();
    auto const bounds = m.bounds();

    auto& random = random_[random_stream::substantive];

    generate_creature(ctx, m, random_definition(random, creature_dictionary_), bklib::ipoint2 {2, 2});

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
          , *ctx.data.random_item(random_, random_stream::substantive)
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

    renderer_.clear();

    auto const scale = view_.get_zoom();
    auto const trans = view_.get_scroll();

    renderer_.set_scale(x(scale), y(scale));
    renderer_.set_translation(x(trans), y(trans));

    current_map().draw(renderer_, view_);

    test_layout_.draw(renderer_);
    message_log_.draw(renderer_);
    inventory_.draw(renderer_);

    renderer_.present();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::advance()
{
    auto ctx = make_context();
    bkrl::advance(ctx, current_map());

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
    test_layout_.set_position(mx, my);

    auto const p = view_.screen_to_world(mx, my);
    if (p != mouse_last_pos_ && intersects(p, current_map().bounds())) {
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
        default:
            break;
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

    if (!set_door_state(current_map(), p, state)) {
        if (state == door::state::open) {
            display_message("Couldn't open the door.");
        } else if (state == door::state::closed) {
            display_message("Couldn't close the door.");
        }
    } else {
        if (state == door::state::open) {
            display_message("You open the door.");
        } else if (state == door::state::closed) {
            display_message("You close the door.");
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

    auto const candidates = find_around(current_map(), p, find_door(state));

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
        if (current_map().at(q).type != terrain_type::door) {
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

    inventory_.on_action([this, &subject, where](inventory::action const type, int const i, ptrdiff_t const data) {
        BK_NAMED_SCOPE_EXIT(close) {
            inventory_.show(false);
            command_translator_.pop_handler(); //TODO
        };

        switch (type) {
        case inventory::action::confirm: {
            if (i < 0 || !data) {
                return;
            }

            auto ctx = make_context();
            auto const result = with_pile_at(definitions_, current_map(), where, [&](item_pile& pile) {
                subject.drop_item(pile, i);
                display_message("You dropped the %s.", pile.begin()->friendly_name(ctx));
            });

            if (!result) {
                display_message("You can't drop that here.");
            }
            return;
        }
        case inventory::action::cancel:
            return;
        case inventory::action::select:
            close.dismiss();
            break;
        default:
            return;
        }
    });

    auto ctx = make_context();
    command_translator_.push_handler(make_item_list(
        ctx, inventory_, subject.item_list(), "Drop which item?"));
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

    item_pile* const pile = current_map().items_at(where);
    if (!pile) {
        return;
    }

    on_fail.dismiss();

    inventory_.on_action([this, &subject, where](inventory::action const type, int const i, ptrdiff_t const data) {
        BK_NAMED_SCOPE_EXIT(close) {
            inventory_.show(false);
            command_translator_.pop_handler(); //TODO
        };

        switch (type) {
        case inventory::action::confirm: {
            if (i < 0 || !data) {
                return;
            }

            auto& itm = *reinterpret_cast<item*>(data);

            auto ctx = make_context();

            if (!subject.can_get_item(itm)) {
                display_message("You can't get the %s.", itm.friendly_name(ctx));
                return;
            }

            display_message("You got the %s.", itm.friendly_name(ctx));
            subject.get_item(current_map().remove_item_at(where, i));

            return;
        }
        case inventory::action::cancel:
            return;
        case inventory::action::select:
            close.dismiss();
            break;
        default:
            return;
        }
    });

    auto ctx = make_context();
    command_translator_.push_handler(make_item_list(ctx, inventory_, *pile, "Get which item?"));
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
    auto ctx = make_context();

    auto const p = get_player().position();

    auto const ok = move_by(ctx, get_player(), current_map(), bklib::truncate<2>(v));
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
            display_message("You see here %s.", i.friendly_name(ctx));
        }
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
    auto const visible = !inventory_.is_visible();
    inventory_.show(visible);

    if (visible) {
        do_show_inventory();
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_show_inventory()
{
    inventory_.on_action([this](inventory::action const type, int const i, ptrdiff_t const data) {
        BK_NAMED_SCOPE_EXIT(close) {
            inventory_.show(false);
            command_translator_.pop_handler(); //TODO
        };

        switch (type) {
        case inventory::action::confirm: {
            if (i < 0 || !data) {
                return;
            }

            auto const& itm = *reinterpret_cast<item const*>(data);
            auto ctx = make_context();

            display_message("You chose the %d%s item -- a %s"
                , i + 1, bklib::oridinal_suffix(i + 1).data(), itm.friendly_name(ctx));

            return;
        }
        case inventory::action::cancel:
            return;
        case inventory::action::select:
            close.dismiss();
            break;
        default:
            return;
        }
    });

    auto const& player = get_player();
    auto ctx = make_context();

    command_translator_.push_handler(make_item_list(ctx, inventory_, player.item_list(), "Items"));
}

//--------------------------------------------------------------------------------------------------
bkrl::command_handler_result bkrl::game::on_command(command const& cmd)
{
    switch (cmd.type) {
    case command_type::none:    break;
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
bkrl::context bkrl::game::make_context()
{
    return {random_, definitions_, output_, item_factory_, creature_factory_};
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::debug_print(int const mx, int const my) const
{
    auto ctx = const_cast<game*>(this)->make_context();

    bklib::ipoint2 const p {mx, my};

    auto const& ter = current_map().at(p);
    printf("cell (%d, %d)\n", x(p), y(p));
    printf("  type = %d::%d\n", static_cast<int16_t>(ter.type), ter.variant);

    if (auto const& c = current_map().creature_at(p)) {
        fmt::printf("  creature present\n");
        fmt::printf("  %s\n", c->friendly_name(definitions_));
    }

    if (auto const& ip = current_map().items_at(p)) {
        fmt::printf("  item(s) present\n");
        for (auto const& i : *ip) {
            fmt::printf("  %s\n", i.friendly_name(ctx));
        }
    }
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
