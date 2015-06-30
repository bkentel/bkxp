#include "commands.hpp"

#include <vector>

#include "bklib/string.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
class bkrl::detail::command_translator_impl {
public:
    void push_handler(command_handler_t&& handler) {
        handlers_.emplace_back(std::move(handler));
    }

    void pop_handler() {
        handlers_.pop_back();
    }

#if defined(BK_NO_SDL)
    void on_key_down(int const key) {}
#else
    void on_key_down(int const key) {
        if (handlers_.empty()) {
            return;
        }

        command cmd {};

        switch (key) {
        default :
            return;
        case SDLK_KP_PLUS:
            cmd.type  = command_type::zoom;
            cmd.data0 = 1;
            break;
        case SDLK_KP_MINUS:
            cmd.type  = command_type::zoom;
            cmd.data0 = -1;
            break;
        case SDLK_KP_1:   cmd.type = command_type::dir_s_west; break;
        case SDLK_DOWN:
        case SDLK_KP_2:   cmd.type = command_type::dir_south;  break;
        case SDLK_KP_3:   cmd.type = command_type::dir_s_east; break;
        case SDLK_LEFT:
        case SDLK_KP_4:   cmd.type = command_type::dir_west;   break;
        case SDLK_KP_5:   cmd.type = command_type::dir_here;   break;
        case SDLK_RIGHT:
        case SDLK_KP_6:   cmd.type = command_type::dir_east;   break;
        case SDLK_KP_7:   cmd.type = command_type::dir_n_west; break;
        case SDLK_UP:
        case SDLK_KP_8:   cmd.type = command_type::dir_north;  break;
        case SDLK_KP_9:   cmd.type = command_type::dir_n_east; break;
        case SDLK_c:      cmd.type = command_type::close;      break;
        case SDLK_d:      cmd.type = command_type::drop;       break;
        case SDLK_g:      cmd.type = command_type::get;        break;
        case SDLK_o:      cmd.type = command_type::open;       break;
        case SDLK_n:      cmd.type = command_type::no;         break;
        case SDLK_y:      cmd.type = command_type::yes;        break;
        case SDLK_q:      cmd.type = command_type::quit;       break;
        case SDLK_i:      cmd.type = command_type::show_inventory; break;
        case SDLK_ESCAPE: cmd.type = command_type::cancel;     break;
        }

        if (handlers_.back()(cmd) == command_handler_result::detach) {
            handlers_.pop_back();
        }
    }
#endif

    void on_key_up(int) {
    }

    void on_mouse_move_to(int, int) {
    }

    void on_mouse_down(int, int, int) {
    }

    void on_mouse_up(int, int, int) {
    }
private:
    std::vector<command_handler_t> handlers_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------
bkrl::command_translator::command_translator()
  : impl_(std::make_unique<detail::command_translator_impl>())
{
}

//----------------------------------------------------------------------------------------------
bkrl::command_translator::~command_translator() noexcept = default;

//----------------------------------------------------------------------------------------------
void bkrl::command_translator::push_handler(command_handler_t&& handler)
{
    impl_->push_handler(std::move(handler));
}

//----------------------------------------------------------------------------------------------
void bkrl::command_translator::pop_handler()
{
    impl_->pop_handler();
}

//----------------------------------------------------------------------------------------------
void bkrl::command_translator::on_key_down(int const key)
{
    impl_->on_key_down(key);
}

//----------------------------------------------------------------------------------------------
void bkrl::command_translator::on_key_up(int const key)
{
    impl_->on_key_up(key);
}

//----------------------------------------------------------------------------------------------
void bkrl::command_translator::on_mouse_move_to(int const x, int const y)
{
    impl_->on_mouse_move_to(x, y);
}

//----------------------------------------------------------------------------------------------
void bkrl::command_translator::on_mouse_down(int const x, int const y, int const button)
{
    impl_->on_mouse_down(x, y, button);
}

//----------------------------------------------------------------------------------------------
void bkrl::command_translator::on_mouse_up(int const x, int const y, int const button)
{
    impl_->on_mouse_up(x, y, button);
}
