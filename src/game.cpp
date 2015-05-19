#include "game.hpp"
#include "direction.hpp"

#include "bklib/string.hpp"

//--------------------------------------------------------------------------------------------------
bkrl::game::game()
  : system_()
  , renderer_(system_)
  , command_translator_()
{
    command_translator_.push_handler([&](command const& cmd) {
        on_command(cmd);
    });

    system_.on_text_input = [&](bklib::utf8_string_view s) {
    };

    system_.on_key_up = [&](int const key) {
        command_translator_.on_key_up(key);
    };

    system_.on_key_down = [&](int const key) {
        command_translator_.on_key_down(key);
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
    current_map_.fill(bklib::irect {5, 5, 15, 20}
    , terrain_type::floor, terrain_type::wall);

    current_map_.update_render_data();

}

//--------------------------------------------------------------------------------------------------
void bkrl::game::render()
{
    renderer_.clear();

    current_map_.draw(renderer_);
    player_.draw(renderer_);

    renderer_.present();
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
void bkrl::game::do_open()
{
    update();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_open()
{
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
void bkrl::game::do_move(bklib::ivec3 const v)
{
    auto const p = player_.position();
    auto const bounds = current_map_.bounds();

    auto const q = p + bklib::ivec2 {x(v), y(v)};

    if (x(q) < bounds.left || x(q) >= (bounds.right)
     || y(q) < bounds.top  || y(q) >= (bounds.bottom)
    ) {
        return;
    }

    auto const& ter = current_map_.at(x(q), y(q));
    if (ter.type != terrain_type::empty && ter.type != terrain_type::floor) {
        return;
    }

    player_.move_by(x(v), y(v));
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
    default:
        break;
    }
}
