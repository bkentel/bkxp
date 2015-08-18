#include "commands.hpp"

#include <vector>

#include "bklib/assert.hpp"
#include "bklib/string.hpp"

namespace bkrl { class command_translator_impl; }

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
class bkrl::command_translator_impl final : public command_translator {
public:
    virtual ~command_translator_impl() final;

    //----------------------------------------------------------------------------------------------
    command_translator_impl() {
        result_handler_ = [](auto&&, auto&&) {};
        handlers_.emplace_back([](auto&&) {
            return command_handler_result::capture;
        });
    }

    //----------------------------------------------------------------------------------------------
    void push_handler(handler_t&& handler) override final {
        handlers_.emplace_back(std::move(handler));
    }

    //----------------------------------------------------------------------------------------------
    void pop_handler(int const i = 0) override final {
        BK_PRECONDITION(!handlers_.empty());

        auto const n = static_cast<int>(handlers_.size());
        BK_PRECONDITION(n > i);

        handlers_.erase(handlers_.begin() + (n - (i + 1)));
    }

    //----------------------------------------------------------------------------------------------
    int size() noexcept override final {
        return static_cast<int>(handlers_.size());
    }

    //----------------------------------------------------------------------------------------------
#if defined(BK_NO_SDL)
    void on_key_down(int, key_mod_state) override final {
    }
#else // !defined(BK_NO_SDL)
    void on_key_down(int const key, key_mod_state const mods) override final {
        using ct = command_type;

        if (!send_command_(make_command(command_raw_t {mods, key}))) {
            return;
        }

        command cmd {};

        switch (key) {
        default :
            return;
        case SDLK_KP_PLUS:
            cmd.type  = ct::zoom;
            cmd.data0 = 1;
            break;
        case SDLK_KP_MINUS:
            cmd.type  = ct::zoom;
            cmd.data0 = -1;
            break;
        case SDLK_KP_1:     cmd.type = ct::dir_s_west;     break;
        case SDLK_DOWN:     BK_FALLTHROUGH
        case SDLK_KP_2:     cmd.type = ct::dir_south;      break;
        case SDLK_KP_3:     cmd.type = ct::dir_s_east;     break;
        case SDLK_LEFT:     BK_FALLTHROUGH
        case SDLK_KP_4:     cmd.type = ct::dir_west;       break;
        case SDLK_KP_5:     cmd.type = ct::dir_here;       break;
        case SDLK_RIGHT:    BK_FALLTHROUGH
        case SDLK_KP_6:     cmd.type = ct::dir_east;       break;
        case SDLK_KP_7:     cmd.type = ct::dir_n_west;     break;
        case SDLK_UP:       BK_FALLTHROUGH
        case SDLK_KP_8:     cmd.type = ct::dir_north;      break;
        case SDLK_KP_9:     cmd.type = ct::dir_n_east;     break;
        case SDLK_c:        cmd.type = ct::close;          break;
        case SDLK_d:        cmd.type = ct::drop;           break;
        case SDLK_e:        cmd.type = ct::show_equipment; break;
        case SDLK_g:        cmd.type = ct::get;            break;
        case SDLK_o:        cmd.type = ct::open;           break;
        case SDLK_n:        cmd.type = ct::no;             break;
        case SDLK_y:        cmd.type = ct::yes;            break;
        case SDLK_q:        cmd.type = ct::quit;           break;
        case SDLK_i:        cmd.type = ct::show_inventory; break;
        case SDLK_ESCAPE:   cmd.type = ct::cancel;         break;
        case SDLK_RETURN:   BK_FALLTHROUGH
        case SDLK_RETURN2:  BK_FALLTHROUGH
        case SDLK_KP_ENTER: cmd.type = ct::confirm;        break;
        }

        send_command_(cmd);
    }
#endif // BK_NO_SDL
    //----------------------------------------------------------------------------------------------
    void on_key_up(int, key_mod_state) override final {
    }

    //----------------------------------------------------------------------------------------------
    void on_mouse_move_to(int, int) override final {
    }

    //----------------------------------------------------------------------------------------------
    void on_mouse_down(int, int, int) override final {
    }

    //----------------------------------------------------------------------------------------------
    void on_mouse_up(int, int, int) override final {
    }

    //----------------------------------------------------------------------------------------------
    void on_text(bklib::utf8_string_view const str) override final {
        send_command(make_command(str));
    }

    //----------------------------------------------------------------------------------------------
    void send_command(command const cmd) override final {
        send_command_(cmd);
    }

    //----------------------------------------------------------------------------------------------
    void set_command_result_handler(result_handler_t handler) override final {
        result_handler_ = std::move(handler);
    }

    //----------------------------------------------------------------------------------------------
    void on_command_result(command_type const command, command_result const result) override final {
        result_handler_(command, result);
    }
private:
    //----------------------------------------------------------------------------------------------
    bool send_command_(command const cmd) {
        auto const size_before = size();
        auto const result = handlers_.back()(cmd);

        if (result == command_handler_result::detach) {
            auto const n = size() - size_before;
            BK_PRECONDITION(n >= 0);
            pop_handler(n);
        } else if (result == command_handler_result::filter) {
            BK_PRECONDITION(cmd.type == command_type::raw);
            return false;
        }

        return true;
    }
private:
    std::vector<handler_t> handlers_;
    result_handler_t       result_handler_;
};

//--------------------------------------------------------------------------------------------------
bkrl::command_translator_impl::~command_translator_impl() {
}

//--------------------------------------------------------------------------------------------------
bkrl::command_translator::~command_translator() {
}

//--------------------------------------------------------------------------------------------------
std::unique_ptr<bkrl::command_translator> bkrl::make_command_translator() {
    return std::make_unique<command_translator_impl>();
}
