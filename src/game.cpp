#include "game.hpp"

#include "bsp_layout.hpp"
#include "color.hpp"
#include "commands.hpp"
#include "context.hpp"
#include "creature.hpp"
#include "definitions.hpp"
#include "direction.hpp"
#include "inventory.hpp"
#include "json.hpp"
#include "map.hpp"
#include "message_log.hpp"
#include "output.hpp"
#include "random.hpp"
#include "renderer.hpp"
#include "system.hpp"
#include "text.hpp"
#include "view.hpp"

#include "bklib/dictionary.hpp"
#include "bklib/math.hpp"
#include "bklib/scope_guard.hpp"
#include "bklib/string.hpp"
#include "bklib/timer.hpp"

namespace bkrl {

template <typename T>
struct find_around_result_t {
    std::array<T, 9> values;
    size_t           count;
    size_t           index;

    template <typename U>
    find_around_result_t<T>& operator=(find_around_result_t<U> const& rhs) {
        values[0] = rhs.values[0];
        values[1] = rhs.values[1];
        values[2] = rhs.values[2];
        values[3] = rhs.values[3];
        values[4] = rhs.values[4];
        values[5] = rhs.values[5];
        values[6] = rhs.values[6];
        values[7] = rhs.values[7];
        values[8] = rhs.values[8];

        count = rhs.count;
        index = rhs.index;

        return *this;
    }

    explicit operator bool() const noexcept { return !!count; }

    //! The position relative to p corresponding the last matched value.
    bklib::ipoint2 position(bklib::ipoint2 const p) const noexcept {
        return p + bklib::ivec2 {x_off[index], y_off[index]};
    }

    T const& at(bklib::ivec2 const v) const noexcept { return values[offset_to_index(v)]; }
    T&       at(bklib::ivec2 const v) noexcept       { return values[offset_to_index(v)]; }

    T const& last_found() const noexcept { return values[index]; }
    T&       last_found() noexcept       { return values[index]; }
};

template <typename Predicate>
find_around_result_t<std::pair<item_pile*, item_pile::iterator>>
find_neighboring_items(map& m, bklib::ipoint2 const where, Predicate&& pred) {
    std::array<item_pile*, 9> const piles {
        m.items_at(where + bklib::ivec2 {x_off[0], y_off[0]})
      , m.items_at(where + bklib::ivec2 {x_off[1], y_off[1]})
      , m.items_at(where + bklib::ivec2 {x_off[2], y_off[2]})
      , m.items_at(where + bklib::ivec2 {x_off[3], y_off[3]})
      , m.items_at(where + bklib::ivec2 {x_off[4], y_off[4]})
      , m.items_at(where + bklib::ivec2 {x_off[5], y_off[5]})
      , m.items_at(where + bklib::ivec2 {x_off[6], y_off[6]})
      , m.items_at(where + bklib::ivec2 {x_off[7], y_off[7]})
      , m.items_at(where + bklib::ivec2 {x_off[8], y_off[8]})
    };

    auto const last = item_pile::iterator {};

    using pair_t = std::pair<item_pile*, item_pile::iterator>;
    std::array<pair_t, 9> result {
        pair_t {piles[0], piles[0] ? piles[0]->find_if(pred) : last}
      , pair_t {piles[1], piles[1] ? piles[1]->find_if(pred) : last}
      , pair_t {piles[2], piles[2] ? piles[2]->find_if(pred) : last}
      , pair_t {piles[3], piles[3] ? piles[3]->find_if(pred) : last}
      , pair_t {piles[4], piles[4] ? piles[4]->find_if(pred) : last}
      , pair_t {piles[5], piles[5] ? piles[5]->find_if(pred) : last}
      , pair_t {piles[6], piles[6] ? piles[6]->find_if(pred) : last}
      , pair_t {piles[7], piles[7] ? piles[7]->find_if(pred) : last}
      , pair_t {piles[8], piles[8] ? piles[8]->find_if(pred) : last}
    };

    size_t count = 0;
    size_t index = 0;

    for (auto i = 0u; i < 9u; ++i) {
        if (piles[i] && piles[i]->end() != result[i].second) {
            ++count;
            index = i;
        }
    }

    return {std::move(result), count, index};
}

template <typename Predicate>
find_around_result_t<std::pair<item_pile const*, item_pile::const_iterator>>
find_neighboring_items(map const& m, bklib::ipoint2 const where, Predicate&& pred) {
    return find_neighboring_items(const_cast<map&>(m), where, std::forward<Predicate>(pred));
}

template <typename Predicate>
find_around_result_t<terrain_entry*>
find_neighboring_terrain(map& m, bklib::ipoint2 const where, Predicate&& pred) {
    std::array<bklib::ipoint2, 9> const points {
        where + index_to_offset(0)
      , where + index_to_offset(1)
      , where + index_to_offset(2)
      , where + index_to_offset(3)
      , where + index_to_offset(4)
      , where + index_to_offset(5)
      , where + index_to_offset(6)
      , where + index_to_offset(7)
      , where + index_to_offset(8)
    };

    auto const bounds = m.bounds();
    std::array<bool, 9> const matches {
        bklib::intersects(bounds, points[0]) && pred(m.at(points[0]))
      , bklib::intersects(bounds, points[1]) && pred(m.at(points[1]))
      , bklib::intersects(bounds, points[2]) && pred(m.at(points[2]))
      , bklib::intersects(bounds, points[3]) && pred(m.at(points[3]))
      , bklib::intersects(bounds, points[4]) && pred(m.at(points[4]))
      , bklib::intersects(bounds, points[5]) && pred(m.at(points[5]))
      , bklib::intersects(bounds, points[6]) && pred(m.at(points[6]))
      , bklib::intersects(bounds, points[7]) && pred(m.at(points[7]))
      , bklib::intersects(bounds, points[8]) && pred(m.at(points[8]))
    };

    std::array<terrain_entry*, 9> const pointers {
        matches[0] ? &m.at(points[0]) : nullptr
      , matches[1] ? &m.at(points[1]) : nullptr
      , matches[2] ? &m.at(points[2]) : nullptr
      , matches[3] ? &m.at(points[3]) : nullptr
      , matches[4] ? &m.at(points[4]) : nullptr
      , matches[5] ? &m.at(points[5]) : nullptr
      , matches[6] ? &m.at(points[6]) : nullptr
      , matches[7] ? &m.at(points[7]) : nullptr
      , matches[8] ? &m.at(points[8]) : nullptr
    };

    size_t count = 0;
    size_t index = 0;

    for (auto i = 0u; i < 9u; ++i) {
        if (pointers[i]) {
            ++count;
            index = i;
        }
    }

    return {pointers, count, index};
}

template <typename Predicate>
find_around_result_t<terrain_entry const*>
find_neighboring_terrain(map const& m, bklib::ipoint2 const where, Predicate&& pred) {
    return find_neighboring_terrain(const_cast<map&>(m), where, std::forward<Predicate>(pred));
}

//////////////////////////////

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
void display_item_list(
    context& ctx
  , command_translator& commands
  , item& container
  , inventory& imenu
) {
    BK_PRECONDITION(has_flag(container, item_flag::is_container));

    auto const pile = get_item_data<item_data_type::container>(container);
    BK_PRECONDITION(pile);

    bkrl::populate_item_list(ctx, imenu, *pile, container.friendly_name(ctx));
    imenu.set_on_action([&](auto const action, auto const ) {
        if (action == inventory::action::confirm) {

        } else if (action == inventory::action::cancel) {
            imenu.show(false);
            commands.pop_handler();
        }
    });

    commands.push_handler(default_item_list_handler(imenu));
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
void open_nothing(context& ctx, command_translator& commands)
{
    ctx.out.write("There is nothing to open here.");
    commands.on_command_result(command_type::open, 0);
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
void open_cancel(context& ctx, command_translator& commands)
{
    ctx.out.write("Nevermind.");
    commands.on_command_result(command_type::open, 0);
}

//--------------------------------------------------------------------------------------------------
//! Open the door at the location adjacent to subject specified by where.
//--------------------------------------------------------------------------------------------------
void open_door_at(
    context&             ctx
  , command_translator&  commands
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
) {
    if (set_door_state(current_map, where, door::state::open)) {
        ctx.out.write("You open the door.");
        commands.on_command_result(command_type::open, 0);
    } else {
        ctx.out.write("You fail to open the door.");
        commands.on_command_result(command_type::open, 0);
    }
}

//--------------------------------------------------------------------------------------------------
//! Open one of the doors adjacent to subject specified by doors.
//--------------------------------------------------------------------------------------------------
void open_doors(
    context& ctx
  , command_translator& commands
  , creature& subject
  , map& current_map
  , find_around_result_t<terrain_entry*> const& doors
) {
    ctx.out.write("Open in which direction?");
    commands.push_handler(filter_text_and_raw([&, doors](command const& cmd) {
        if (is_direction(cmd.type)) {
            auto const v = direction_vector<2>(cmd.type);
            if (auto const door = doors.at(v)) {
                open_door_at(ctx, commands, subject, current_map, subject.position() + v);
            } else {
                ctx.out.write("There is nothing there to open.");
            }
        } else if (cmd.type == command_type::cancel) {
            open_cancel(ctx, commands);
        } else {
            ctx.out.write("Invalid choice. Choose a direction, or cancel.");
            return command_handler_result::capture;
        }

        return command_handler_result::detach;
    }));
}

//--------------------------------------------------------------------------------------------------
//! Open a container from the pile specified by pair.
//--------------------------------------------------------------------------------------------------
void open_containers_at(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , std::pair<item_pile*, item_pile::iterator> const pair
  , inventory&          imenu
) {
    auto       it   = pair.second;
    auto const last = pair.first->end();

    auto const get_next = [&] {
        return std::find_if(++it, last, find_by_flag(item_flag::is_container));
    };

    auto next = get_next();
    if (next == last) {
        display_item_list(ctx, commands, *pair.second, imenu);
        return;
    }

    ctx.out.write("More than one container.");

    int i = 0;
    ctx.out.write("{} {}", i++, pair.second->friendly_name(ctx));

    do {
        ctx.out.write("{} {}", i++, next->friendly_name(ctx));
    } while ((next = get_next()) != last);
}

//--------------------------------------------------------------------------------------------------
//! Open a container at one of the positions adjacent to subject specified by containers.
//--------------------------------------------------------------------------------------------------
void open_containers(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , find_around_result_t<std::pair<item_pile*, item_pile::iterator>> const& containers
  , inventory&          imenu
) {
    ctx.out.write("Open in which direction?");
    commands.push_handler(filter_text_and_raw([&, containers](command const& cmd) {
        if (is_direction(cmd.type)) {
            auto const pair = containers.at(direction_vector<2>(cmd.type));
            if (!pair.first) {
                ctx.out.write("There is nothing there to open.");
            } else {
                open_containers_at(ctx, commands, subject, pair, imenu);
            }
        } else if (cmd.type == command_type::cancel) {
            open_cancel(ctx, commands);
        } else {
            ctx.out.write("Invalid choice. Choose a direction, or cancel.");
            return command_handler_result::capture;
        }

        return command_handler_result::detach;
    }));
}

//--------------------------------------------------------------------------------------------------
//! Open a door or container adjacent to the subject.
//--------------------------------------------------------------------------------------------------
void open_around(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , inventory&          imenu
) {
    auto const p = subject.position();
    auto doors = find_neighboring_terrain(current_map, p, find_door(door::state::closed));
    auto conts = find_neighboring_items(current_map, p, find_by_flag(item_flag::is_container));

    auto const choose_type = [&] {
        if (conts.count == 1) {
            ctx.out.write("There is a {} here, open it?", conts.last_found().second->friendly_name(ctx));
        } else {
            ctx.out.write("There are containers here, open one?");
        }

        commands.push_handler([&, p, doors, conts](command const& cmd) {
            if (cmd.type == command_type::yes) {
                if (conts.count == 1) {
                    open_containers_at(ctx, commands, subject, conts.last_found(), imenu);
                } else {
                    open_containers(ctx, commands, subject, current_map, conts, imenu);
                }
            } else if (cmd.type == command_type::no) {
                if (doors.count == 1) {
                    open_door_at(ctx, commands, subject, current_map, doors.position(p));
                } else {
                    open_doors(ctx, commands, subject, current_map, doors);
                }
            } else if (cmd.type == command_type::cancel) {
                open_cancel(ctx, commands);
            } else {
                ctx.out.write("Invalid choice. Yes, no, or cancel?");
                return command_handler_result::capture;
            }

            return command_handler_result::detach;
        });
    };

    if      (doors.count == 0 && conts.count == 0) { open_nothing(ctx, commands); }
    else if (doors.count == 0 && conts.count == 1) { open_containers_at(ctx, commands, subject, conts.last_found(), imenu); }
    else if (doors.count == 0 && conts.count  > 1) { open_containers(ctx, commands, subject, current_map, conts, imenu); }
    else if (doors.count == 1 && conts.count == 0) { open_door_at(ctx, commands, subject, current_map, doors.position(p)); }
    else if (doors.count == 1 && conts.count == 1) { choose_type(); }
    else if (doors.count == 1 && conts.count  > 1) { choose_type(); }
    else if (doors.count  > 1 && conts.count == 0) { open_doors(ctx, commands, subject, current_map, doors); }
    else if (doors.count  > 1 && conts.count == 1) { choose_type(); }
    else if (doors.count  > 1 && conts.count  > 1) { choose_type(); }
    else {
        BK_PRECONDITION_SAFE(false && "unreachable");
    }
}

/////////////////////////

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
void close_nothing(context& ctx, command_translator& commands)
{
    ctx.out.write("There is nothing to close here.");
    commands.on_command_result(command_type::close, 0);
}

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
void close_cancel(context& ctx, command_translator& commands)
{
    ctx.out.write("Nevermind.");
    commands.on_command_result(command_type::close, 0);
}

//--------------------------------------------------------------------------------------------------
//! Open the door at the location adjacent to subject specified by where.
//--------------------------------------------------------------------------------------------------
void close_door_at(
    context&             ctx
  , command_translator&  commands
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
) {
    if (set_door_state(current_map, where, door::state::closed)) {
        ctx.out.write("You close the door.");
        commands.on_command_result(command_type::close, 0);
    } else {
        ctx.out.write("You fail to close the door.");
        commands.on_command_result(command_type::close, 0);
    }
}

//--------------------------------------------------------------------------------------------------
//! Open one of the doors adjacent to subject specified by doors.
//--------------------------------------------------------------------------------------------------
void close_doors(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , find_around_result_t<terrain_entry*> const& doors
) {
    ctx.out.write("Close in which direction?");
    commands.push_handler(filter_text_and_raw([&, doors](command const& cmd) {
        if (is_direction(cmd.type)) {
            auto const v = direction_vector<2>(cmd.type);
            if (auto const door = doors.at(v)) {
                close_door_at(ctx, commands, subject, current_map, subject.position() + v);
            } else {
                ctx.out.write("There is nothing there to close.");
            }
        } else if (cmd.type == command_type::cancel) {
            close_cancel(ctx, commands);
        } else {
            ctx.out.write("Invalid choice. Choose a direction, or cancel.");
            return command_handler_result::capture;
        }

        return command_handler_result::detach;
    }));
}

//--------------------------------------------------------------------------------------------------
//! Close a door (TODO or other widget?) adjacent to the subject
//--------------------------------------------------------------------------------------------------
void close_around(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
) {
    auto const p = subject.position();
    auto doors = find_neighboring_terrain(current_map, p, find_door(door::state::open));

    if      (doors.count == 0) { close_nothing(ctx, commands); }
    else if (doors.count == 1) { close_door_at(ctx, commands, subject, current_map, doors.position(p)); }
    else if (doors.count  > 1) { close_doors(ctx, commands, subject, current_map, doors); }
    else {
        BK_UNREACHABLE;
    }
}

/////////////////////////

//--------------------------------------------------------------------------------------------------
// Game simulation state.
//--------------------------------------------------------------------------------------------------
class game {
public:
    game();

    void generate_map();

    enum class render_type {
        wait, force_update
    };

    void render(render_type type = render_type::wait);
    void advance();

    creature& get_player();
    creature const& get_player() const;

    void display_message(bklib::utf8_string_view msg);

    void on_mouse_over(int x, int y);
    void do_mouse_over(int x, int y);

    void on_zoom(double zx, double zy);
    void do_zoom(double zx, double zy);

    void on_scroll(double dx, double dy);
    void do_scroll(double dx, double dy);

    void do_quit();
    void on_quit();

    void on_open();
    void on_close();

    void on_get();
    void on_drop();

    void do_wait(int turns);

    void do_move(bklib::ivec3 v);
    void on_move(bklib::ivec3 v);

    void on_show_inventory();

    void on_show_equipment();
    void do_show_equipment();

    command_handler_result on_command(command const& cmd);
    void on_command_result(command_type cmd, size_t data);

    context make_context();

    void debug_print(int x, int y);

    map&       current_map()       noexcept;
    map const& current_map() const noexcept;
private:
    void set_inspect_message_position_();
private:
    bklib::timer        timer_;
    random_state        random_;
    std::unique_ptr<system>        system_;
    std::unique_ptr<renderer>      renderer_;
    std::unique_ptr<text_renderer> text_renderer_;
    view                view_;
    std::unique_ptr<command_translator> command_translator_;
    color_dictionary    color_dictionary_;
    creature_dictionary creature_dictionary_;
    item_dictionary     item_dictionary_;
    definitions         definitions_;
    creature_factory    creature_factory_;
    item_factory        item_factory_;
    std::vector<std::unique_ptr<map>> maps_;
    map*                current_map_;
    output              output_;
    std::unique_ptr<inventory> inventory_;

    std::chrono::high_resolution_clock::time_point last_frame_;

    key_mod_state  prev_key_mods_ = system_->current_key_mods();
    bklib::ipoint2 mouse_last_pos_ = bklib::ipoint2 {0, 0};
    bklib::ipoint2 mouse_last_pos_screen_ = bklib::ipoint2 {0, 0};

    message_log message_log_;

    bklib::timer::id_t timer_message_log_ {0};

    text_layout inspect_text_ {*text_renderer_, ""};
    bool show_inspect_text_ = false;

    context ctx_;
};

//--------------------------------------------------------------------------------------------------
template <typename T>
inline decltype(auto) random_definition(random_t& random, bklib::dictionary<T> const& dic) {
    BK_PRECONDITION(!dic.empty());
    return random_element(random, dic);
}

} //namespace bkrl

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
void bkrl::start_game()
{
    bkrl::game game;
}

//--------------------------------------------------------------------------------------------------
bkrl::game::game()
  : timer_()
  , random_()
  , system_ {make_system()}
  , renderer_ {make_renderer(*system_)}
  , text_renderer_ {make_text_renderer()}
  , view_(system_->client_width(), system_->client_height(), 18, 18)
  , command_translator_ {make_command_translator()}
  , color_dictionary_ {}
  , creature_dictionary_ {}
  , item_dictionary_ {}
  , definitions_ {::load_definitions(creature_dictionary_, item_dictionary_, color_dictionary_)}
  , creature_factory_()
  , item_factory_()
  , maps_ {}
  , current_map_ {nullptr}
  , output_ {}
  , inventory_ {make_item_list(*text_renderer_)}
  , last_frame_ {std::chrono::high_resolution_clock::now()}
  , message_log_ {*text_renderer_}
  , ctx_ (make_context())
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
    text_renderer_->set_colors(&color_dictionary_);

    //
    // set up initial map
    //
    maps_.emplace_back(::generate_map(ctx_));
    current_map_ = maps_.back().get();
    generate_map();

    command_translator_->push_handler([&](command const& cmd) {
        return on_command(cmd);
    });

    command_translator_->set_command_result_handler([&](command_type const cmd, size_t const data) {
        on_command_result(cmd, data);
    });

    system_->on_window_resize = [&](int const w, int const h) {
        view_.set_window_size(w, h);
    };

    system_->on_text_input = [&](bklib::utf8_string_view const str) {
        command_translator_->on_text(str);
    };

    auto const check_key_mods_changed = [&] {
        auto const cur = system_->current_key_mods();
        if (cur == prev_key_mods_) {
            return;
        }

        BK_SCOPE_EXIT { prev_key_mods_ = cur; };

        show_inspect_text_ = cur.test(key_mod::shift);
        if (show_inspect_text_) {
            set_inspect_message_position_();
        }
    };

    system_->on_key_up = [&](int const key) {
        check_key_mods_changed();
        command_translator_->on_key_up(key, prev_key_mods_);
    };

    system_->on_key_down = [&](int const key) {
        check_key_mods_changed();
        command_translator_->on_key_down(key, prev_key_mods_);
    };

    system_->on_mouse_motion = [&](mouse_state const m) {
        if (inventory_->on_mouse_move(m)) {
            return;
        }

        if (m.is_down(mouse_button::right)) {
            on_scroll(m.dx, m.dy);
        } else {
            on_mouse_over(m.x, m.y);
        }
    };

    system_->on_mouse_scroll = [&](mouse_state const m) {
        if (inventory_->on_mouse_scroll(m)) {
            return;
        }

        if (m.sy > 0) {
            on_zoom(0.1, 0.1);
        } else if (m.sy < 0) {
            on_zoom(-0.1, -0.1);
        }
    };

    system_->on_mouse_button = [&](mouse_button_state const m) {
        if (inventory_->on_mouse_button(m)) {
            return;
        }
    };

    system_->on_request_quit = [&] {
        on_quit();
        return !system_->is_running();
    };

    ////

    using namespace std::chrono_literals;
    timer_message_log_ = timer_.add(1s, [&](auto&) {
        message_log_.show(message_log::show_type::less);
    });

    while (system_->is_running()) {
        system_->do_events_wait();
        timer_.update();
        render();
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::generate_map()
{
    auto& m = current_map();
    auto& random = random_[random_stream::substantive];

    generate_creature(ctx_, m, random_definition(random, creature_dictionary_), bklib::ipoint2 {2, 2});

    creature_def def {"player"};
    def.flags.set(creature_flag::is_player);
    def.symbol.assign("@");

    bklib::ipoint2 const p {215, 15};
    m.place_creature_at(creature_factory_.create(random, def, p), def, p);

    // For debugging inventory
    auto& player = *m.creature_at(p);

    for (auto i = 0; i < 20; ++i) {
        player.get_item(item_factory_.create(
            random
          , ctx_.data.items()
          , *ctx_.data.random_item(random_, random_stream::substantive)
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

    renderer_->clear();

    auto const scale = view_.get_zoom();
    auto const trans = view_.get_scroll();

    renderer_->set_scale(x(scale), y(scale));
    renderer_->set_translation(x(trans), y(trans));

    current_map().draw(*renderer_, view_);

    message_log_.draw(*renderer_);
    inventory_->draw(*renderer_);

    if (show_inspect_text_) {
        auto const r = make_renderer_rect(add_border(inspect_text_.extent(), 4));
        renderer_->draw_filled_rect(r, make_color(200, 200, 200, 200));
        inspect_text_.draw(*renderer_);
    }

    renderer_->present();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::advance()
{
    bkrl::advance(ctx_, current_map());
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
        if (show_inspect_text_) {
            set_inspect_message_position_();
        }
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
    system_->quit();
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_quit()
{
    display_message("Are you sure you want to quit? Y/N");

    query_yn(*command_translator_, [this](command_type const cmd) {
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
void bkrl::game::on_open()
{
    //bkrl::open(ctx_, get_player(), current_map(), inventory_, *command_translator_);
    bkrl::open_around(ctx_, *command_translator_, get_player(), current_map(), *inventory_);
    advance(); // TODO not strictly correct
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_close()
{
    //bkrl::close(ctx_, get_player(), current_map(), *command_translator_);
    bkrl::close_around(ctx_, *command_translator_, get_player(), current_map());
    advance(); // TODO not strictly correct
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_get()
{
    auto& subject = get_player();
    get_item(ctx_, subject, current_map(), subject.position(), *inventory_, *command_translator_);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_drop()
{
    auto& subject = get_player();
    drop_item(ctx_, subject, current_map(), subject.position(), *inventory_, *command_translator_);
}

//--------------------------------------------------------------------------------------------------
bkrl::drop_item_result_t bkrl::drop_item(
    context&             ctx
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
  , inventory&           imenu
  , command_translator&  commands
) {
    auto& items = subject.item_list();

    if (items.empty()) {
        ctx.out.write("You have nothing to drop.");
        return drop_item_result::no_items;
    }

    if (abs_max(where - subject.position()) > 1) {
        ctx.out.write("You can't drop that there from here.");
        return drop_item_result::out_of_range;
    }

    imenu.set_on_action([&, where](inventory::action const type, int const sel) {
        BK_NAMED_SCOPE_EXIT(close) {
            imenu.show(false);
            commands.pop_handler(); //TODO
        };

        if (sel < 0 || type == inventory::action::cancel) {
            set_command_result(commands, drop_item_result::canceled);
            return;
        }

        if (type != inventory::action::confirm) {
            close.dismiss();
            return;
        }

        auto const result = with_pile_at(ctx.data, current_map, where, [&](item_pile& pile) {
            subject.drop_item(pile, imenu.data());
            ctx.out.write("You dropped the {}.", pile.begin()->friendly_name(ctx));
        });

        if (!result) {
            ctx.out.write("You can't drop that here.");
            set_command_result(commands, drop_item_result::failed);
            return;
        }

        set_command_result(commands, drop_item_result::ok);
    });

    commands.push_handler(default_item_list_handler(populate_item_list(
        ctx, imenu, items, "Drop which item?")));
    return drop_item_result::select;
}

//--------------------------------------------------------------------------------------------------
bkrl::command_future bkrl::get_item(
    context&            ctx         //!< The current context.
  , creature&           subject     //!< The subject doing the 'get'.
  , item_pile&          source
  , inventory&          imenu       //!< The menu used to display a list of choices.
  , command_translator& commands    //!< The command translator stack.
) {
    bkrl::command_promise promise;
    auto result = promise.get_future();

    imenu.set_on_action([&, result = std::move(promise)](inventory::action const type, int const i) mutable {
        if (i < 0 || type == inventory::action::cancel) {
            result.set_value(command_result::canceled);
            imenu.show(false);
            return;
        }

        if (type != inventory::action::confirm) {
            return;
        }

        item_pile::iterator it = imenu.data();
        item& itm = *it;

        if (!subject.can_get_item(itm)) {
            ctx.out.write("You can't get the {}.", itm.friendly_name(ctx));
            result.set_value(command_result::failed);
            imenu.show(false);
            return;
        }

        ctx.out.write("You got the {}.", itm.friendly_name(ctx));
        move_item(source, subject.item_list(), it);

        result.set_value(command_result::ok);
    });

    populate_item_list(ctx, imenu, source, "Get which item?");

    commands.push_handler([&imenu, skip = true](command const& cmd) mutable {
        if (skip && cmd.type == command_type::text) {
            skip = false; // skips the text message to follow
            return command_handler_result::capture;
        }

        imenu.on_command(cmd);
        if (!imenu.is_visible()) {
            return command_handler_result::detach_passthrough;
        }

        return command_handler_result::capture;
    });

    return result;
}

//--------------------------------------------------------------------------------------------------
bkrl::get_item_result_t bkrl::get_item(
    context&             ctx
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
  , inventory&           imenu
  , command_translator&  commands
) {
    if (abs_max(where - subject.position()) > 1) {
        ctx.out.write("You can't get that from here.");
        return get_item_result::out_of_range;
    }

    item_pile* const pile = current_map.items_at(where);
    if (!pile) {
        ctx.out.write("There is nothing here to get.");
        return get_item_result::no_items;
    }

    imenu.set_on_action([&, where](inventory::action const type, int const i) {
        BK_NAMED_SCOPE_EXIT(close) {
            imenu.show(false);
            commands.pop_handler(); //TODO
        };

        if (i < 0 || type == inventory::action::cancel) {
            set_command_result(commands, get_item_result::canceled);
            return;
        }

        if (type != inventory::action::confirm) {
            close.dismiss();
            return;
        }

        item_pile::iterator const it  = imenu.data();
        item& itm = *it;

        if (!subject.can_get_item(itm)) {
            ctx.out.write("You can't get the {}.", itm.friendly_name(ctx));
            set_command_result(commands, get_item_result::failed);
            return;
        }

        ctx.out.write("You got the {}.", itm.friendly_name(ctx));
        current_map.move_item_at(where, it, subject.item_list());
        set_command_result(commands, get_item_result::ok);
    });

    commands.push_handler(default_item_list_handler(populate_item_list(
        ctx, imenu, *pile, "Get which item?")));
    return get_item_result::select;
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
    auto const p = get_player().position();

    auto const ok = move_by(ctx_, get_player(), current_map(), bklib::truncate<2>(v));
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
            ctx_.out.write("You see here {}.", i.friendly_name(ctx_));
        }
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_move(bklib::ivec3 const v)
{
    BK_PRECONDITION((x(v) || y(v)) && !z(v));
    do_move(v);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_show_inventory()
{
    auto const visible = !inventory_->is_visible();
    inventory_->show(visible);

    if (visible) {
        show_inventory(ctx_, get_player(), *inventory_, *command_translator_);
    }
}

//--------------------------------------------------------------------------------------------------
bkrl::show_inventory_result_t bkrl::show_inventory(
    context&            ctx
  , creature&           subject
  , inventory&          imenu
  , command_translator& commands
)
{
    auto& items = subject.item_list();
    if (items.empty()) {
        ctx.out.write("You have no items.");
        return show_inventory_result::no_items;
    }

    imenu.set_on_action([&](inventory::action const type, int const i) {
        BK_NAMED_SCOPE_EXIT(close) {
            imenu.show(false);
            commands.pop_handler(); //TODO
        };

        if (i < 0 || type == inventory::action::cancel) {
            set_command_result(commands, show_inventory_result::canceled);
            return;
        }

        close.dismiss();

        auto& itm = *imenu.data();

        if (type == inventory::action::confirm) {
            ctx.out.write("You chose the {}{} item -- {}"
              , i + 1
              , bklib::oridinal_suffix(i + 1).data()
              , itm.friendly_name(ctx));
        } else if (type == inventory::action::equip) {
            set_command_result(commands, equip_item(ctx, subject, itm));
        } else {
            return;
        }

        set_command_result(commands, show_inventory_result::ok);
    });

    commands.push_handler(default_item_list_handler(populate_item_list(
        ctx, imenu, items, "Items")));
    return show_inventory_result::select;
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
bkrl::equip_item_result_t bkrl::equip_item(
    context&  ctx
  , creature& subject
  , item&     itm
) {
    using status_t = equipment::result_t::status_t;

    if (!subject.has_item(itm)) {
        return bkrl::equip_result_t::not_held;
    }

    auto& eq = subject.equip_list();
    auto const result = subject.equip_item(itm);

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
        return itm.friendly_name(ctx, flags);
    };

    switch (result.status) {
    case status_t::ok:
        out.write("You equip {}.", iname());
        print_slots(eq.where(itm));
        break;
    case status_t::not_equippable:
        flags.capitalize = true;
        out.write("{} cannot be equipped.", iname());
        break;
    case status_t::slot_occupied:
        flags.capitalize = true;
        out.write("{} can't be equipped.", iname());
        print_slots(result.slots());
        break;
    case status_t::slot_not_present:
        out.write("You can't seem to find an appropriate place to equip {}.", iname());
        break;
    case status_t::already_equipped: {
        out.write("You already have {} equipped.", iname());
        print_slots(eq.where(itm));
        break;
    }
    case status_t::slot_empty: BK_FALLTHROUGH
    default:
        break;
    }

    ctx.out.write(out.c_str());
    return result.status;
}

//--------------------------------------------------------------------------------------------------
bkrl::command_handler_result bkrl::game::on_command(command const& cmd)
{
    switch (cmd.type) {
    case command_type::none:    break;
    case command_type::raw:     break;
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
        on_move(direction_vector<3>(cmd.type));
        break;
    case command_type::open:
        on_open();
        break;
    case command_type::close:
        on_close();
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
void bkrl::game::on_command_result(command_type const cmd, size_t const data)
{
    switch (cmd) {
    case command_type::none:    break;
    case command_type::raw:     break;
    case command_type::text:    break;
    case command_type::confirm: break;
    case command_type::invalid: break;
    case command_type::scroll:  break;
    case command_type::cancel:  break;
    case command_type::yes:     break;
    case command_type::no:      break;
    case command_type::use:     break;
    case command_type::zoom:    break;
    case command_type::dir_here:   BK_FALLTHROUGH
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
        break;
    case command_type::open:
        if (static_cast<open_result>(data) == open_result::ok) {
            advance();
        }
        break;
    case command_type::close:
        if (static_cast<close_result>(data) == close_result::ok) {
            advance();
        }
        break;
    case command_type::quit:
        break;
    case command_type::get:
        if (static_cast<get_item_result>(data) == get_item_result::ok) {
            advance();
        }
        break;
    case command_type::drop:
        if (static_cast<drop_item_result>(data) == drop_item_result::ok) {
            advance();
        }
        break;
    case command_type::show_equipment:
        break;
    case command_type::show_inventory:
        break;
    default:
        break;
    }
}

//--------------------------------------------------------------------------------------------------
bkrl::context bkrl::game::make_context()
{
    return {random_, definitions_, output_, item_factory_, creature_factory_};
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::set_inspect_message_position_()
{
    auto const result = move_rect_inside(
        bklib::irect {0, 0, system_->client_width(), system_->client_height()}
      , translate_to(add_border(inspect_text_.extent(), 4)
                   , x(mouse_last_pos_screen_), y(mouse_last_pos_screen_)));

    if (!result.second) {
        BK_ASSERT(false && "TODO");
    }

    inspect_text_.set_position(result.first.left, result.first.top);
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::debug_print(int const mx, int const my)
{
    if (!system_->current_key_mods().test(key_mod::shift)) {
        return;
    }

    inspect_text_.set_text(*text_renderer_
      , inspect_tile(ctx_, get_player(), current_map(), bklib::ipoint2 {mx, my}));

    set_inspect_message_position_();
    show_inspect_text_ = true;
}

//--------------------------------------------------------------------------------------------------
bklib::utf8_string bkrl::inspect_tile(
    context&              ctx
  , creature       const& subject
  , map            const& current_map
  , bklib::ipoint2 const  where
) {
    if (!intersects(current_map.bounds(), where)) {
        return {};
    }

    fmt::MemoryWriter out;

    auto const& ter = current_map.at(where);
    out.write("[{}, {}] = ({}:{})",
        x(where), y(where), static_cast<uint16_t>(ter.type), ter.variant);

    if (auto const& c = current_map.creature_at(where)) {
        out.write("\n"
            "Creature ({})\n"
            " Def : {} ({:0x})\n"
            " Name: {}"
          , static_cast<uint32_t>(c->id())
          , c->def().c_str(), static_cast<uint32_t>(c->def())
          , c->friendly_name(ctx.data)
        );
    }

    if (auto const& ip = current_map.items_at(where)) {
        out.write("\nItem(s)");
        for (auto const& i : *ip) {
            out.write("\n {}", i.friendly_name(ctx));

            if (i.flags().test(item_flag::is_container)) {
                auto const inner = get_item_data<item_data_type::container>(i);
                for (auto const& ii : *inner) {
                    out.write("\n +{}", ii.friendly_name(ctx));
                }
            }
        }
    }

    return out.str();
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

namespace {
template <typename T>
inline void set_command_result(bkrl::command_translator& commands, bkrl::command_type const cmd, T const result)
{
    static_assert(std::is_enum<T>::value, "");
    static_assert(sizeof(T) <= sizeof(size_t), "");
    commands.on_command_result(cmd, static_cast<size_t>(result));
}
} //namespace

void bkrl::set_command_result(command_translator& commands, get_item_result const result) {
    ::set_command_result(commands, command_type::get, result);
}

void bkrl::set_command_result(command_translator& commands, drop_item_result const result) {
    ::set_command_result(commands, command_type::drop, result);
}

void bkrl::set_command_result(command_translator& commands, show_inventory_result const result) {
    ::set_command_result(commands, command_type::show_inventory, result);
}

void bkrl::set_command_result(command_translator& commands, open_result const result) {
    ::set_command_result(commands, command_type::open, result);
}

void bkrl::set_command_result(command_translator& commands, close_result const result) {
    ::set_command_result(commands, command_type::close, result);
}

void bkrl::set_command_result(command_translator& commands, equip_result_t const result) {
    ::set_command_result(commands, command_type::show_equipment, result);
}
