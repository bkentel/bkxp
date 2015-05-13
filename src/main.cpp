#include "bklib/assert.hpp"
#include "bklib/string.hpp"
#include "bklib/utility.hpp"
#include "bklib/math.hpp"
#include "bklib/exception.hpp"
#include "identifier.hpp"
#include "map.hpp"
#include "system.hpp"
#include "renderer.hpp"
#include "random.hpp"
#include "bsp_layout.hpp"
#include "commands.hpp"
#include "creature.hpp"

#include <unordered_set>

namespace bklib {

template <typename T>
using optional = boost::optional<T>;

struct icolor {
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

} // namespace bklib

namespace bkrl {

using symbol_t = bklib::tagged_value<uint32_t, struct tag_symbol_t>;
using color_t  = bklib::icolor;

constexpr color_t color_black {0xFF, 0x00, 0x00, 0x00};

struct terrain_def_t {
    bklib::utf8_string name   {"nothing"};
    bklib::utf8_string flavor {"nothingness"};
    map_terrain_t type;
    symbol_t      symbol {' '};
    color_t       color = color_black;
};


struct room_generator {
    void generate(bkrl::random_t& gen, bklib::irect const& bounds) {

    }
};

} //namespace bkrl

template <typename T>
struct closed_interval {
    closed_interval(T const lo, T const hi, std::true_type) noexcept
      : lo {lo < hi ? lo : hi}
      , hi {lo < hi ? hi : lo}
    {
    }

    closed_interval(T const lo, T const hi, std::false_type) noexcept
      : lo {lo}, hi {hi}
    {
    }

    closed_interval(T const lo = T {0}, T const hi = {0}) noexcept
      : closed_interval(lo, hi, std::false_type {})
    {
    }

    closed_interval intersect(closed_interval const other) const noexcept {
        return {std::max(lo, other.lo), std::min(hi, other.hi)};
    }

    closed_interval normalize() const noexcept {
        return {lo, hi, std::true_type {}};
    }

    explicit operator bool() const noexcept {
        return lo <= hi;
    }

    T lo;
    T hi;
};


namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////

using weight_value_t = bklib::tagged_value<int, struct tag_weight_value_t>;
using worth_value_t = bklib::tagged_value<int, struct tag_worth_value_t>;

////////////////////////////////////////////////////////////////////////////////////////////////////
// terrain
////////////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------------
struct texture_def : id_base<id::texture> {
    id::texture_source source;
    int                off_x {0};
    int                off_y {0};
};

//----------------------------------------------------------------------------------------------
struct terrain_def : id_base<id::terrain> {
    id::texture        texture;
    bklib::utf8_string name;
    bklib::utf8_string description;
    bklib::utf8_string symbol;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// entities
////////////////////////////////////////////////////////////////////////////////////////////////////
using entity_id = bklib::tagged_value<int, struct tag_entity_id>;
using entity_def_id = bklib::tagged_value<int, struct tag_entity_def_id>;

class entity_def {
public:
private:
    bklib::utf8_string name_;
};

class entity {
public:
    enum class flags : unsigned {
        is_player
    };
private:
    entity_id      id_;
    entity_def_id  definition_;
    flags          flags_;
    bklib::ipoint2 pos_;
};

class entity_attribute {
public:
private:
    using value_t = int;

    value_t value_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// items
////////////////////////////////////////////////////////////////////////////////////////////////////
using item_id = bklib::tagged_value<int, struct tag_item_id>;
using item_def_id = bklib::tagged_value<int, struct tag_item_def_id>;

class item {
public:
private:
    item_id     id_;
    item_def_id definition_;
};

////////////////////////////////////////////////////////////

constexpr bklib::ivec3 vec_here   { 0,  0,  0};
constexpr bklib::ivec3 vec_north  { 0, -1,  0};
constexpr bklib::ivec3 vec_south  { 0,  1,  0};
constexpr bklib::ivec3 vec_east   { 1,  0,  0};
constexpr bklib::ivec3 vec_west   {-1,  0,  0};
constexpr bklib::ivec3 vec_n_east { 1, -1,  0};
constexpr bklib::ivec3 vec_n_west {-1, -1,  0};
constexpr bklib::ivec3 vec_s_east { 1,  1,  0};
constexpr bklib::ivec3 vec_s_west {-1,  1,  0};
constexpr bklib::ivec3 vec_up     { 0,  0, -1};
constexpr bklib::ivec3 vec_down   { 0,  0,  1};

bklib::ivec3 direction_vector(command_type const cmd) {
    switch (cmd) {
    case command_type::dir_here:   return vec_here;
    case command_type::dir_north:  return vec_north;
    case command_type::dir_south:  return vec_south;
    case command_type::dir_east:   return vec_east;
    case command_type::dir_west:   return vec_west;
    case command_type::dir_n_west: return vec_n_west;
    case command_type::dir_n_east: return vec_n_east;
    case command_type::dir_s_west: return vec_s_west;
    case command_type::dir_s_east: return vec_s_east;
    case command_type::dir_up:     return vec_up;
    case command_type::dir_down:   return vec_down;
    }

    return vec_here;
}

//--------------------------------------------------------------------------------------------------
// Game simulation state.
//--------------------------------------------------------------------------------------------------
class game {
public:
    game()
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

        while (system_.is_running()) {
            system_.do_events_wait();
            render();
        }

    }

    void render() {
        renderer_.clear();

        current_map_.draw(renderer_);

        renderer_.present();
    }

    void update() {

    }

    void display_message(bklib::utf8_string_view const msg) {
        printf("%s\n", msg.data());
    }

    void do_quit() {
        system_.quit();
    }

    void on_quit() {
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

    void do_open() {
        update();
    }

    void on_open() {
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

    void do_move() {

    }

    void on_move(bklib::ivec3 v) {
        printf("on_move %d %d\n", x(v), y(v));
    }

    void on_command(command const& cmd) {
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
private:
    system             system_;
    renderer           renderer_;
    command_translator command_translator_;

    map    current_map_;
    player player_;
};

} //namespace bkrl

namespace std {
template <> struct hash<bkrl::texture_def> : bkrl::id_hash_base<bkrl::texture_def> {};
template <> struct hash<bkrl::terrain_def> : bkrl::id_hash_base<bkrl::terrain_def> {};
}


int run_unit_tests();

void main() try {
    run_unit_tests();

    bkrl::game game;

    return;
} catch (bklib::exception_base const&) {
} catch (boost::exception const&) {
} catch (std::exception const&) {
} catch (...) {
}
