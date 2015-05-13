#pragma once

#include <functional>
#include <memory>
#include <cstdint>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
enum class command_type : int32_t {
    none
  , invalid
  , cancel
  , yes
  , no
  , dir_here
  , dir_north
  , dir_south
  , dir_east
  , dir_west
  , dir_n_west
  , dir_n_east
  , dir_s_west
  , dir_s_east
  , dir_up
  , dir_down
  , quit
  , use
  , open
  , close
};

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
static_assert(alignof(command) >= alignof(double), "");

namespace detail { class command_translator_impl; }

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
class command_translator {
public:
    using handler_t = std::function<void (command const&)>;

    command_translator();
    ~command_translator();

    void push_handler(handler_t handler);
    void pop_handler();

    void on_key_down(int key);
    void on_key_up(int key);
    void on_mouse_move_to(int x, int y);
    void on_mouse_down(int x, int y, int button);
    void on_mouse_up(int x, int y, int button);
private:
    std::unique_ptr<detail::command_translator_impl> impl_;
};

enum class query_result {
    done, more
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
void query_yn(command_translator& translator, std::function<query_result (command_type)> handler);

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
void query_dir(command_translator& translator, std::function<query_result (command_type)> handler);


////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
