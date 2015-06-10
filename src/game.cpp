#include "game.hpp"
#include "direction.hpp"

#include "bklib/string.hpp"

//--------------------------------------------------------------------------------------------------
bkrl::game::game()
  : system_()
  , renderer_(system_)
  , view_(system_.client_width(), system_.client_height(), 18, 18)
  , command_translator_()
  , creature_dictionary_ {"creatures.def"}
  , item_dictionary_ {"items.def"}
  , creature_factory_()
  , current_map_()
  , player_(creature_factory_.create(random_, creature_def {"player"}, bklib::ipoint2 {0, 0}))

  , test_layout_ {text_renderer_, "Message.", 5, 5, 640, 200}
{
    command_translator_.push_handler([&](command const& cmd) {
        on_command(cmd);
    });

    system_.on_window_resize = [&](int const w, int const h) {
        printf("resize %d %d\n", w, h);
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

    generate_map();

    while (system_.is_running()) {
        system_.do_events_wait();
        render();
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::generate_map()
{
    auto& m = current_map_;

    m.fill(bklib::irect {5, 5, 15, 20}, terrain_type::floor, terrain_type::wall);
    m.at(5, 10).type = terrain_type::door;
    m.update_render_data(5, 10);

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
void bkrl::game::do_open(bklib::ipoint2 const p)
{
    auto& ter_door = current_map_.at(p);

    door d {ter_door};
    if (!d.open()) {
        display_message("Couldn't open the door.");
    } else {
        ter_door = d;
        current_map_.update_render_data(x(p), y(p));
    }

    advance();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_open()
{
    auto const candidates = current_map_.find_around(player_.position(), [](terrain_entry const& ter) {
        if (ter.type != terrain_type::door) {
            return false;
        }

        return door {ter}.is_closed();
    });

    if (!candidates.count) {
        display_message("There is nothing here to open.");
        return;
    } else if (candidates.count == 1) {
        do_open(bklib::ipoint2 {candidates.x, candidates.y});
        return;
    }

    display_message("Open in which direction?");

    query_dir(command_translator_, [this](command_type const cmd) {
        switch (cmd) {
        case command_type::cancel:
            display_message("Nevermind.");
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
        on_open();
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
