#include "game.hpp"
#include "direction.hpp"

#include "bklib/string.hpp"

//--------------------------------------------------------------------------------------------------
bkrl::game::game()
  : timer_()
  , random_()
  , system_()
  , renderer_(system_)
  , text_renderer_()
  , view_(system_.client_width(), system_.client_height(), 18, 18)
  , command_translator_()
  , creature_dictionary_ {"creatures.def"}
  , item_dictionary_ {"items.def"}
  , creature_factory_()
  , item_factory_()
  , current_map_()
  , player_(creature_factory_.create(random_, creature_def {"player"}, bklib::ipoint2 {0, 0}))

  , test_layout_ {text_renderer_, "Message.", 5, 5, 640, 200}
  , message_log_ {text_renderer_}
{
    command_translator_.push_handler([&](command const& cmd) {
        on_command(cmd);
    });

    system_.on_window_resize = [&](int const w, int const h) {
        view_.set_window_size(w, h);
    };

    system_.on_text_input = [&](bklib::utf8_string_view s) {
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

    ////

    using namespace std::chrono_literals;
    timer_message_log_ = timer_.add(5s, [&](auto& timer_record) {
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

    m.fill(bklib::irect {5, 5, 15, 20}, terrain_type::floor, terrain_type::wall);

    m.at(5, 10).type = terrain_type::door;
    m.at(5, 12).type = terrain_type::door;
    m.update_render_data(5, 10);
    m.update_render_data(5, 12);

    m.update_render_data();

    for (int i = 0; i < 10; ++i) {
        m.generate_creature(random_, creature_factory_, creature_def {"skeleton"});
        m.generate_item(random_, item_factory_, item_def {"item0"});
    }
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
    player_.draw(renderer_);   

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
    do_zoom(zy, zy);
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
        switch (cmd) {
        case command_type::yes:
            do_quit();
            return query_result::done;
        case command_type::no:
            display_message("Ok.");
            return query_result::done;
        case command_type::cancel:
            display_message("Canceled.");
            return query_result::done;
        case command_type::invalid:
            display_message("Invalid choice.");
        default:
            break;
        }

        return query_result::more;
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

    auto const p = player_.position();
    auto const state = (type == command_type::open)
        ? door::state::closed : door::state::open;

    auto const candidates = current_map_.find_around(p, find_door(state));   

    //
    // Nothing to do.
    //
    if (!candidates.count) {
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
        do_open_close(bklib::ipoint2 {candidates.x, candidates.y}, type);
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
        switch (cmd) {
        case command_type::cancel:
            display_message("Nevermind.");
            return query_result::done;
        case command_type::invalid:
            display_message("Invalid choice.");
            break;
        default:
            if (!is_direction(cmd)) {
                break;
            }

            auto const q = p + bklib::truncate<2>(direction_vector(cmd));
            if (current_map_.at(q).type != terrain_type::door) {
                if (type == command_type::open) {
                    display_message("There is nothing there to open.");
                } else if (type == command_type::close) {
                    display_message("There is nothing there to close.");
                }

                break;
            }

            do_open_close(q, type);
            return query_result::done;
        }

        return query_result::more;
    });
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_get()
{
    do_get();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_get()
{
    auto const p = player_.position();

    if (item_pile* const pile = current_map_.items_at(p)) {
        if (!player_.can_get_items(*pile)) {
            return;
        }

        player_.get_items(current_map_.remove_items_at(p));
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::do_move(bklib::ivec3 const v)
{
    if (current_map_.move_creature_by(player_, bklib::truncate<2>(v))) {
        advance();
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_move(bklib::ivec3 const v)
{
    printf("on_move %d %d\n", x(v), y(v));
    do_move(v);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_command(command const& cmd)
{
    switch (cmd.type) {
    case command_type::zoom:
        on_zoom(cmd.data0, cmd.data0);
        break;
    case command_type::dir_here:
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
    default:
        break;
    }
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

        if (auto const& cdef = creature_dictionary_[c->def()]) {
            printf("  %s\n", cdef->id_string.c_str());
        } else {
            printf("  !!unknown creature!!\n");
        }
    }

    if (auto const& ip = current_map_.items_at(p)) {
        printf("  item(s) present\n");
        for (auto const& i : *ip) {
            if (auto const& idef = item_dictionary_[i.def()]) {
                printf("  %s\n", idef->id_string.c_str());
            } else {
                printf("  !!unknown item!!\n");
            }
        }
    }
}
