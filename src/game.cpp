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
  , message_log_ {text_renderer_}
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
    text_renderer_.set_colors(&color_dictionary_);

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

    auto const check_key_mods_changed = [&] {
        auto const cur = system_.current_key_mods();
        if (cur == prev_key_mods_) {
            return;
        }

        BK_SCOPE_EXIT { prev_key_mods_ = cur; };

        show_inspect_text_ = cur.test(key_mod::shift);
    };

    system_.on_key_up = [&](int const key) {
        check_key_mods_changed();
        command_translator_.on_key_up(key, prev_key_mods_);
    };

    system_.on_key_down = [&](int const key) {
        check_key_mods_changed();
        command_translator_.on_key_down(key, prev_key_mods_);
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

    message_log_.draw(renderer_);
    inventory_.draw(renderer_);

    if (show_inspect_text_) {
        auto const r = make_renderer_rect(add_border(inspect_text_.extent(), 4));
        renderer_.draw_filled_rect(r, make_color(200, 200, 200, 200));
        inspect_text_.draw(renderer_);
    }

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
    mouse_last_pos_screen_ = bklib::ipoint2 {mx, my};

    auto const p = view_.screen_to_world(mx, my);

    if (p == mouse_last_pos_) {
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

    inventory_.on_action([this, &subject, where](inventory::action const type, int const sel) {
        BK_NAMED_SCOPE_EXIT(close) {
            inventory_.show(false);
            command_translator_.pop_handler(); //TODO
        };

        if (sel < 0 || type == inventory::action::cancel) {
            return;
        }

        if (type != inventory::action::confirm) {
            close.dismiss();
            return;
        }

        auto const i = from_inventory_data(inventory_.data()).second;

        auto ctx = make_context();
        auto const result = with_pile_at(definitions_, current_map(), where, [&](item_pile& pile) {
            subject.drop_item(pile, i);
            display_message("You dropped the %s.", pile.begin()->friendly_name(ctx));
        });

        if (!result) {
            display_message("You can't drop that here.");
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

    inventory_.on_action([this, &subject, where](inventory::action const type, int const i) {
        BK_NAMED_SCOPE_EXIT(close) {
            inventory_.show(false);
            command_translator_.pop_handler(); //TODO
        };

        if (i < 0 || type == inventory::action::cancel) {
            return;
        }

        if (type != inventory::action::confirm) {
            close.dismiss();
            return;
        }

        auto const data = from_inventory_data(inventory_.data());
        auto&      itm   = data.first;
        auto const index = data.second;

        auto ctx = make_context();

        if (!subject.can_get_item(itm)) {
            display_message("You can't get the %s.", itm.friendly_name(ctx));
            return;
        }

        display_message("You got the %s.", itm.friendly_name(ctx));
        subject.get_item(current_map().remove_item_at(where, index));
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
    inventory_.on_action([this](inventory::action const type, int const i) {
        BK_NAMED_SCOPE_EXIT(close) {
            inventory_.show(false);
            command_translator_.pop_handler(); //TODO
        };

        if (i < 0 || type == inventory::action::cancel) {
            return;
        }

        auto& itm = from_inventory_data(inventory_.data()).first;

        switch (type) {
        case inventory::action::confirm: {
            close.dismiss();

            auto ctx = make_context();
            display_message("You chose the %d%s item -- %s"
              , i + 1
              , bklib::oridinal_suffix(i + 1).data()
              , itm.friendly_name(ctx));

            break;
        }
        case inventory::action::equip: {
            close.dismiss();
            do_equip_item(itm);

            break;
        }
        case inventory::action::select:
            close.dismiss();
            break;
        case inventory::action::cancel: BK_FALLTHROUGH
        default:
            break;
        }
    });

    auto const& player = get_player();
    auto ctx = make_context();

    command_translator_.push_handler(make_item_list(ctx, inventory_, player.item_list(), "Items"));
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
void bkrl::game::do_equip_item(item& i)
{
    auto ctx = make_context();

    auto& eq = get_player().equip_list();
    auto const result = eq.equip(i);

    using status_t = equipment::result_t::status_t;

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
        return i.friendly_name(ctx, flags);
    };

    switch (result.status) {
    case status_t::ok:
        out.write("You equip {}.", iname());
        print_slots(eq.where(i));
        break;
    case status_t::not_equippable:
        flags.capitalize = true;
        out.write("{} cannot be equipped.", iname());
        break;
    case status_t::slot_occupied:
        flags.capitalize = true;
        out.write("{} can't be equipped.", iname());
        print_slots(static_cast<item_slots>(result));
        break;
    case status_t::slot_not_present:
        out.write("You can't seem to find an appropriate place to equip %s.", iname());
        break;
    case status_t::already_equipped: {
        out.write("You already have {} equipped.", iname());
        print_slots(eq.where(i));
        break;
    }
    case status_t::slot_empty: BK_FALLTHROUGH
    default:
        break;
    }

    display_message(out.c_str());
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
    case command_type::open: BK_FALLTHROUGH
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
bkrl::context bkrl::game::make_context()
{
    return {random_, definitions_, output_, item_factory_, creature_factory_};
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::debug_print(int const mx, int const my)
{
    if (!system_.current_key_mods().test(key_mod::shift)) {
        return;
    }

    auto ctx = const_cast<game*>(this)->make_context();

    bklib::ipoint2 const p {mx, my};

    std::ostringstream str;

    auto const& ter = current_map().at(p);
    str << fmt::sprintf("[%3.3d, %3.3d] (%d::%d)"
        , x(p), y(p), static_cast<uint16_t>(ter.type), ter.variant);

    if (auto const& c = current_map().creature_at(p)) {
        str << fmt::sprintf(
            "\nCreature (%#08x)\n"
            "  Def  : %s (%#08x)\n"
            "  Name : <color=o>%s</color>"
          , static_cast<uint32_t>(c->id())
          , c->def().c_str(), static_cast<uint32_t>(c->def())
          , c->friendly_name(definitions_)
        );
    }

    if (auto const& ip = current_map().items_at(p)) {
        str << fmt::sprintf("\nItem(s)");
        for (auto const& i : *ip) {
            str << fmt::sprintf("\n  %s", i.friendly_name(ctx));
        }
    }

    inspect_text_.set_text(text_renderer_, str.str());
    inspect_text_.set_position(x(mouse_last_pos_screen_), y(mouse_last_pos_screen_));

    auto const ext = inspect_text_.extent();
    auto const dr  = system_.client_width() - ext.right;

    if (dr < 0) {
        inspect_text_.set_position(x(mouse_last_pos_screen_) + dr, y(mouse_last_pos_screen_));
    }

    show_inspect_text_ = true;
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
