#pragma once

#include "input.hpp"

#include "bklib/string.hpp"
#include "bklib/utility.hpp"

#include <functional>
#include <memory>
#include <cstdint>
#include <type_traits>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
#define BK_DECLARE_COMMAND(name) name = bklib::static_djb2_hash(#name)
enum class command_type : uint32_t {
    none = 0
  , raw  = 1
  , text = 2
  , BK_DECLARE_COMMAND(invalid)
  , BK_DECLARE_COMMAND(scroll)
  , BK_DECLARE_COMMAND(zoom)
  , BK_DECLARE_COMMAND(cancel)
  , BK_DECLARE_COMMAND(confirm)
  , BK_DECLARE_COMMAND(yes)
  , BK_DECLARE_COMMAND(no)
  , BK_DECLARE_COMMAND(dir_here)
  , BK_DECLARE_COMMAND(dir_north)
  , BK_DECLARE_COMMAND(dir_south)
  , BK_DECLARE_COMMAND(dir_east)
  , BK_DECLARE_COMMAND(dir_west)
  , BK_DECLARE_COMMAND(dir_n_west)
  , BK_DECLARE_COMMAND(dir_n_east)
  , BK_DECLARE_COMMAND(dir_s_west)
  , BK_DECLARE_COMMAND(dir_s_east)
  , BK_DECLARE_COMMAND(dir_up)
  , BK_DECLARE_COMMAND(dir_down)
  , BK_DECLARE_COMMAND(quit)
  , BK_DECLARE_COMMAND(use)
  , BK_DECLARE_COMMAND(open)
  , BK_DECLARE_COMMAND(close)
  , BK_DECLARE_COMMAND(drop)
  , BK_DECLARE_COMMAND(show_inventory)
  , BK_DECLARE_COMMAND(show_equipment)
  , BK_DECLARE_COMMAND(get)
};
#undef BK_DECLARE_COMMAND

constexpr inline bool is_direction(command_type const cmd) noexcept {
  return (cmd == command_type::dir_here)
      || (cmd == command_type::dir_north)
      || (cmd == command_type::dir_south)
      || (cmd == command_type::dir_east)
      || (cmd == command_type::dir_west)
      || (cmd == command_type::dir_n_west)
      || (cmd == command_type::dir_n_east)
      || (cmd == command_type::dir_s_west)
      || (cmd == command_type::dir_s_east)
      || (cmd == command_type::dir_up)
      || (cmd == command_type::dir_down);
}

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
struct command {
    command_type type;
    int32_t data0;

    union {
        intptr_t data1;
        int64_t  unused;
    };
};

static_assert(sizeof(command) == 16, "");
static_assert(std::is_pod<command>::value, "");

struct command_raw_t {
    key_mod_state mods;
    int32_t virtual_key;
};

template <command_type Type, std::enable_if_t<Type == command_type::raw>* = nullptr>
command_raw_t get_command_data(command const cmd) noexcept {
    return {
        *reinterpret_cast<key_mod_state const*>(&cmd.data1)
      , static_cast<int32_t>(cmd.data0)
    };
}

template <command_type Type, std::enable_if_t<Type == command_type::text>* = nullptr>
bklib::utf8_string_view get_command_data(command const cmd) noexcept {
    return {
        reinterpret_cast<char const*>(cmd.data1)
      , static_cast<size_t>(cmd.data0)
    };
}

inline command make_command(command_raw_t const raw) noexcept {
    return {
        command_type::raw
      , raw.virtual_key
      , *reinterpret_cast<intptr_t const*>(&raw.mods)
    };
}

inline command make_command(bklib::utf8_string_view const text) noexcept {
    return {
        command_type::text
      , bklib::checked_integral_cast<int32_t>(text.size())
      , reinterpret_cast<intptr_t>(text.data())
    };
}

template <command_type Type>
inline command make_command() noexcept {
    return {Type, 0, 0};
}

enum class command_handler_result {
    detach  //!< stop accepting input
  , detach_passthrough //!< stop accepting input and pass the current input on to the next handler.
  , capture //!< continue processing input
  , filter  //!< for raw commands, whether to continue to translate the combo into a command.
};

//--------------------------------------------------------------------------------------------------
//! @note This could also be implemented with CRTP or Pimpl. For now, neither ABI stability (Pimpl)
//! nor virtual function call overhead (CRTP) is an issue, so an ABC is the simplest.
//--------------------------------------------------------------------------------------------------
class command_translator {
public:
    using handler_t        = std::function<command_handler_result (command const&)>;
    using result_handler_t = std::function<void (command_type, size_t data)>;

    virtual ~command_translator();

    virtual void push_handler(handler_t&& handler) = 0;
    virtual void pop_handler() = 0;

    virtual void on_key_down(int key, key_mod_state mods) = 0;
    virtual void on_key_up(int key, key_mod_state mods) = 0;
    virtual void on_mouse_move_to(int x, int y) = 0;
    virtual void on_mouse_down(int x, int y, int button) = 0;
    virtual void on_mouse_up(int x, int y, int button) = 0;
    virtual void on_text(bklib::utf8_string_view str) = 0;

    virtual void send_command(command cmd) = 0;

    virtual void set_command_result_handler(result_handler_t handler) = 0;
    virtual void on_command_result(command_type command, size_t data) = 0;
};

//--------------------------------------------------------------------------------------------------
std::unique_ptr<command_translator> make_command_translator();

//--------------------------------------------------------------------------------------------------
//! Filter and transform commands to be one of: yes, no, cancel, or invalid.
//--------------------------------------------------------------------------------------------------
template <typename Handler>
void query_yn(command_translator& translator, Handler&& handler) {
    using ct = command_type;
    translator.push_handler([h = std::move(handler)](command const& cmd) {
        auto const type = cmd.type;
        if (type == ct::raw || type == ct::text) {
            return command_handler_result::capture;
        } else if (type == ct::yes || type == ct::no || type == ct::cancel) {
            return h(type);
        }

        return h(ct::invalid);
    });
}

//--------------------------------------------------------------------------------------------------
//! Filter and transform commands to be: is_direction(cmd) => cmd, cancel, or invalid.
//--------------------------------------------------------------------------------------------------
template <typename Handler>
void query_dir(command_translator& translator, Handler&& handler) {
    translator.push_handler([h = std::move(handler)](command const& cmd) {
        auto const type = cmd.type;
        if (type == command_type::raw || type == command_type::text) {
            return command_handler_result::capture;
        } else if (is_direction(type) || type == command_type::cancel) {
            return h(type);
        }

        return h(command_type::invalid);
    });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
