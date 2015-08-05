#pragma once

#include "input.hpp"

#include "bklib/string.hpp"

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

namespace detail { class command_translator_impl; }

enum class command_handler_result {
    detach  //!< stop accepting input
  , capture //!< continue processing input
  , filter  //!< for raw commands, whether to continue to translate the combo into a command.
};

using command_handler_t = std::function<command_handler_result (command const&)>;

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
class command_translator {
public:
    command_translator();
    ~command_translator() noexcept;

    void push_handler(command_handler_t&& handler);
    void pop_handler();

    void on_key_down(int key, key_mod_state mods);
    void on_key_up(int key, key_mod_state mods);
    void on_mouse_move_to(int x, int y);
    void on_mouse_down(int x, int y, int button);
    void on_mouse_up(int x, int y, int button);
    void on_text(bklib::utf8_string_view str);
private:
    std::unique_ptr<detail::command_translator_impl> impl_;
};

//--------------------------------------------------------------------------------------------------
//! Filter and transform commands to be one of: yes, no, cancel, or invalid.
//--------------------------------------------------------------------------------------------------
template <typename Handler>
void query_yn(command_translator& translator, Handler&& handler) {
    using ct = command_type;

    translator.push_handler([h = std::move(handler)](command const& cmd) {
        auto const type = cmd.type;
        return h(type == ct::yes || type == ct::no || type == ct::cancel
            ? type : ct::invalid);
    });
}

//--------------------------------------------------------------------------------------------------
//! Filter and transform commands to be: is_direction(cmd) => cmd, cancel, or invalid.
//--------------------------------------------------------------------------------------------------
template <typename Handler>
void query_dir(command_translator& translator, Handler&& handler) {
    using ct = command_type;

    translator.push_handler([h = std::move(handler)](command const& cmd) {
        auto const type = cmd.type;
        return h(type == ct::dir_up     || type == ct::dir_down  || type == ct::cancel
              || type == ct::dir_n_west || type == ct::dir_north || type == ct::dir_n_east
              || type == ct::dir_west   || type == ct::dir_here  || type == ct::dir_east
              || type == ct::dir_s_west || type == ct::dir_south || type == ct::dir_s_east
            ? type : ct::invalid);
    });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
