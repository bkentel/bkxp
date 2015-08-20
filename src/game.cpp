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

    //----------------------------------------------------------------------------------------------
    creature& get_player() {
        return *current_map().find_creature([&](creature const& c) {
            return c.is_player();
        });
    }

    creature const& get_player() const {
        return const_cast<game*>(this)->get_player();
    }

    //----------------------------------------------------------------------------------------------
    void display_message(bklib::utf8_string_view msg);

    void on_mouse_over(int x, int y);
    void do_mouse_over(int x, int y);

    void on_zoom(double zx, double zy);
    void do_zoom(double zx, double zy);

    void on_scroll(double dx, double dy);
    void do_scroll(double dx, double dy);

    //----------------------------------------------------------------------------------------------
    void on_center_on_player() {
        auto const p = get_player().position();
        view_.center_on_world(x(p) + 0.5, y(p) + 0.5);
    }

    //----------------------------------------------------------------------------------------------
    void do_quit() {
        system_->quit();
    }

    //----------------------------------------------------------------------------------------------
    void on_quit() {
        display_quit_prompt(ctx_, *command_translator_);
    }

    //----------------------------------------------------------------------------------------------
    void on_open() {
        open_around(ctx_, *command_translator_, get_player(), current_map(), *inventory_);
    }

    //----------------------------------------------------------------------------------------------
    void on_close() {
        close_around(ctx_, *command_translator_, get_player(), current_map());
    }

    //----------------------------------------------------------------------------------------------
    void on_get() {
        get_item_at(ctx_, *command_translator_, get_player(), current_map(), *inventory_);
    }

    //----------------------------------------------------------------------------------------------
    void on_drop() {
        drop_item(ctx_, *command_translator_, get_player(), current_map(), *inventory_);
    }

    void do_wait(int turns);

    void do_move(bklib::ivec3 v);
    void on_move(bklib::ivec3 v);

    void on_show_inventory();

    void on_show_equipment();
    void do_show_equipment();

    command_handler_result on_command(command const& cmd);
    void on_command_result(command_type cmd, command_result result);

    context make_context();

    void debug_print(int x, int y);

    //----------------------------------------------------------------------------------------------
    map& current_map() noexcept {
        BK_PRECONDITION(current_map_);
        return *current_map_;
    }

    map const& current_map() const noexcept {
        return const_cast<game*>(this)->current_map();
    }
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

    command_translator_->set_command_result_handler([&](command_type const cmd, command_result const result) {
        on_command_result(cmd, result);
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

    on_center_on_player();
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
void bkrl::game::display_message(bklib::utf8_string_view const msg) {
    printf("%.*s\n", static_cast<int>(msg.size()), msg.data());

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
    auto const w = system_->client_width();
    auto const h = system_->client_height();
    auto const p = view_.screen_to_world_as(w / 2.0, h / 2.0);
    auto const z = view_.get_zoom();

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

    if (bklib::distance2(view_.get_zoom(), z) > 0) {
        view_.center_on_world(x(p), y(p));
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
        auto& player = get_player();
        display_item_list(ctx_, *command_translator_, player, current_map()
                        , player.item_list(), *inventory_, "Inventory");
    }
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
bkrl::equip_result_t bkrl::equip_item(
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
    case status_t::not_held:   BK_FALLTHROUGH
    default:                   BK_UNREACHABLE;
    }

    ctx.out.write(out);
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
    case command_type::center_on_player:
        on_center_on_player();
        break;
    default:
        break;
    }

    return command_handler_result::capture;
}

//--------------------------------------------------------------------------------------------------
void bkrl::game::on_command_result(command_type const cmd, command_result const result)
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
    case command_type::center_on_player: break;
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
        if (result == command_result::ok_advance) {
            advance();
        }
        break;
    case command_type::close:
        if (result == command_result::ok_advance) {
            advance();
        }
        break;
    case command_type::quit:
        if (result == command_result::ok_advance) {
            do_quit();
        }
        break;
    case command_type::get:
        if (result == command_result::ok_advance) {
            advance();
        }
        break;
    case command_type::drop:
        if (result == command_result::ok_advance) {
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
          , c->friendly_name(ctx)
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
bkrl::command_result bkrl::drop_item_at(
    context&                  ctx
  , creature&                 subject
  , map&                      current_map
  , item_pile&                src_pile
  , item_pile::iterator const src_pos
) {
    auto const p = subject.position();
    bool const is_held = &subject.item_list() == &src_pile;
    auto const name = src_pos->friendly_name(ctx);

    auto const result = with_pile_at(ctx.data, current_map, p, [&](item_pile& dst_pile) {
        if (is_held) {
            ctx.out.write("You drop the {} beneath yourself.", name);
            subject.drop_item(dst_pile, src_pos);
        } else {
            ctx.out.write("You move the {} to the ground beneath yourself.", name);
            move_item(src_pile, dst_pile, src_pos);
        }
    });

    if (result) {
        return command_result::ok_advance;
    }

    if (is_held) {
        ctx.out.write("You couldn't drop the {} beneath yourself.", name);
    } else {
        ctx.out.write("You couldn't move the {} to the ground beneath yourself.", name);
    }

    return command_result::failed;
}

//--------------------------------------------------------------------------------------------------
bkrl::command_result bkrl::drop_item_at(
    context&                  ctx
  , creature&                 subject
  , map&                      current_map
  , item_pile&                src_pile
  , item_pile::iterator const src_pos
  , bklib::ipoint2      const where
) {
    auto const p = subject.position();
    auto const d = abs_max(where - p);

    if (d == 1) {
        return drop_item_at(ctx, subject, current_map, src_pile, src_pos);
    } else if (d > 1) {
        ctx.out.write("The {} is too far away from you.", src_pos->friendly_name(ctx));
        return command_result::out_of_range;
    }

    bool const is_held = &subject.item_list() == &src_pile;
    auto const name = src_pos->friendly_name(ctx);

    auto const result = with_pile_at(ctx.data, current_map, where, [&](item_pile& dst_pile) {
        if (is_held) {
            ctx.out.write("You drop the {}.", name);
            subject.drop_item(dst_pile, src_pos);
        } else {
            ctx.out.write("You move the {}.", name);
            move_item(src_pile, dst_pile, src_pos);
        }
    });

    if (result) {
        return command_result::ok_advance;
    }

    if (is_held) {
        ctx.out.write("You couldn't drop the {}.", name);
    } else {
        ctx.out.write("You couldn't move the {}.", name);
    }

    return command_result::failed;
}

//--------------------------------------------------------------------------------------------------
void bkrl::display_item_list(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , item_pile&          pile
  , inventory&          imenu
  , bklib::utf8_string_view const title
) {
    bkrl::populate_item_list(ctx, imenu, pile, title);
    imenu.set_on_action([&](inventory::action const action, int const index) {
        switch (action) {
        case inventory::action::cancel:
            dismiss_item_list(imenu, commands, command_type::show_inventory, command_result::canceled);
            break;
        case inventory::action::drop: {
            if (imenu.empty()) {
                ctx.out.write("The container is empty.");
                commands.on_command_result(command_type::drop, command_result::none_present);
                break;
            }

            auto const result = drop_item_at(ctx, subject, current_map, pile, imenu.data());
            if (command_succeeded(result)) {
                imenu.remove(index);
            }

            commands.on_command_result(command_type::drop, result);
            break;
        }
        case inventory::action::equip: {
            auto const result = equip_item(ctx, subject, *imenu.data());
            if (result == equip_result_t::ok) {
                item_pile::iterator it = imenu.data();

                imenu.remove(index);

                item::format_flags flags;
                flags.use_color = true;
                flags.override_color = "r";

                imenu.insert(inventory::row_t {"?", it->friendly_name(ctx, flags), it}, index);
            }
            break;
        }
        case inventory::action::get: {
            item_pile::iterator it = imenu.data();
            item& itm = *it;

            if (!subject.can_get_item(itm)) {
                ctx.out.write("You can't get the {}.", itm.friendly_name(ctx));
                commands.on_command_result(command_type::get, command_result::failed);
                break;
            }

            ctx.out.write("You got the {}.", itm.friendly_name(ctx));

            move_item(pile, subject.item_list(), it);
            commands.on_command_result(command_type::get, command_result::ok_advance);
            imenu.remove(index);

            if (imenu.empty()) {
                dismiss_item_list(imenu, commands, command_type::get, command_result::none_present);
                break;
            }

            break;
        }
        case inventory::action::select:  BK_FALLTHROUGH
        case inventory::action::confirm: break;
        default:                         BK_UNREACHABLE;
        }
    });

    commands.on_command_result(command_type::show_inventory, command_result::ok_no_advance);
    commands.push_handler(default_item_list_handler(imenu));
}

//--------------------------------------------------------------------------------------------------
void bkrl::display_item_list(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , item&               container
  , inventory&          imenu
) {
    BK_PRECONDITION(has_flag(container, item_flag::is_container));

    auto const pile = get_item_data<item_data_type::container>(container);
    BK_PRECONDITION(pile);

    if (pile->empty()) {
        ctx.out.write("The {} is empty.", container.friendly_name(ctx));
        commands.on_command_result(command_type::open, command_result::none_present);
        return;
    }

    display_item_list(ctx, commands, subject, current_map, *pile, imenu, container.friendly_name(ctx));
}

//--------------------------------------------------------------------------------------------------
void bkrl::open_nothing(context& ctx, command_translator& commands, creature& subject)
{
    ctx.out.write("There is nothing to open here.");
    commands.on_command_result(command_type::open, command_result::none_present);
}

//--------------------------------------------------------------------------------------------------
void bkrl::open_cancel(context& ctx, command_translator& commands, creature& subject)
{
    ctx.out.write("Nevermind.");
    commands.on_command_result(command_type::open, command_result::canceled);
}

//--------------------------------------------------------------------------------------------------
void bkrl::open_door_at(
    context&             ctx
  , command_translator&  commands
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
) {
    if (set_door_state(current_map, where, door::state::open)) {
        ctx.out.write_if(subject.is_player(), "You open the door.")
          || ctx.out.write("The {} opens the door.", subject.friendly_name(ctx));

        commands.on_command_result(command_type::open, command_result::ok_advance);
    } else {
        ctx.out.write_if(subject.is_player(), "You fail to open the door.")
          || ctx.out.write("The {} fails to open the door.", subject.friendly_name(ctx));

        commands.on_command_result(command_type::open, command_result::failed);
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::open_doors(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , find_around_result_t<terrain_entry*> const& doors
) {
    ctx.out.write("Open in which direction?");
    commands.push_handler(filter_text_and_raw([&, doors](command const& cmd) {
        if (is_direction(cmd.type)) {
            auto const v = direction_vector<2>(cmd.type);
            if (doors.at(v)) {
                open_door_at(ctx, commands, subject, current_map, subject.position() + v);
            } else {
                open_nothing(ctx, commands, subject);
            }
        } else if (cmd.type == command_type::cancel) {
            open_cancel(ctx, commands, subject);
        } else {
            ctx.out.write("Invalid choice. Choose a direction, or cancel.");
            return command_handler_result::capture;
        }

        return command_handler_result::detach;
    }));
}

//--------------------------------------------------------------------------------------------------
void bkrl::open_containers_at(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , std::pair<item_pile*, item_pile::iterator> const pair
  , inventory&          imenu
) {
    item_pile::iterator const first = pair.second;
    item_pile::iterator       it    = pair.second;
    item_pile::iterator const last  = pair.first->end();

    auto const get_next = [&] {
        return std::find_if(++it, last, find_by_flag(item_flag::is_container));
    };

    auto next = get_next();
    if (next == last) {
        ctx.out.write("You open the {}.", first->friendly_name(ctx));
        commands.on_command_result(command_type::open, command_result::ok_advance);
        display_item_list(ctx, commands, subject, current_map, *pair.second, imenu);
        return;
    }

    imenu.clear();
    imenu.set_title("Choose a container");

    imenu.insert(inventory::row_t {"?", first->friendly_name(ctx), first});
    do {
        imenu.insert(inventory::row_t {"?", next->friendly_name(ctx), next});
    } while ((next = get_next()) != last);

    imenu.show(true);
    imenu.set_on_action([&](inventory::action const action, int) {
        switch (action) {
        case inventory::action::cancel:
            dismiss_item_list(imenu, commands, command_type::open, command_result::canceled);
            break;
        case inventory::action::confirm:
            ctx.out.write("You open the {}.", imenu.data()->friendly_name(ctx));
            dismiss_item_list(imenu, commands, command_type::open, command_result::ok_advance);
            display_item_list(ctx, commands, subject, current_map, *imenu.data(), imenu);
            break;
        case inventory::action::select: BK_FALLTHROUGH
        case inventory::action::equip:  BK_FALLTHROUGH
        case inventory::action::get:    BK_FALLTHROUGH
        case inventory::action::drop:   break;
        default:                        BK_UNREACHABLE;
        }
    });

    commands.push_handler(default_item_list_handler(imenu));
}

//--------------------------------------------------------------------------------------------------
void bkrl::open_containers(
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
                open_containers_at(ctx, commands, subject, current_map, pair, imenu);
            }
        } else if (cmd.type == command_type::cancel) {
            open_cancel(ctx, commands, subject);
        } else {
            ctx.out.write("Invalid choice. Choose a direction, or cancel.");
            return command_handler_result::capture;
        }

        return command_handler_result::detach;
    }));
}

//--------------------------------------------------------------------------------------------------
void bkrl::open_around(
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

        commands.push_handler(filter_text_and_raw([&, p, doors, conts](command const& cmd) {
            if (cmd.type == command_type::yes) {
                if (conts.count == 1) {
                    open_containers_at(ctx, commands, subject, current_map, conts.last_found(), imenu);
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
                open_cancel(ctx, commands, subject);
            } else {
                ctx.out.write("Invalid choice. Yes, no, or cancel?");
                return command_handler_result::capture;
            }

            return command_handler_result::detach;
        }));
    };

    if      (doors.count == 0 && conts.count == 0) { open_nothing(ctx, commands, subject); }
    else if (doors.count == 0 && conts.count == 1) { open_containers_at(ctx, commands, subject, current_map, conts.last_found(), imenu); }
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

//--------------------------------------------------------------------------------------------------
void bkrl::close_nothing(context& ctx, command_translator& commands)
{
    ctx.out.write("There is nothing to close here.");
    commands.on_command_result(command_type::close, command_result::none_present);
}

//--------------------------------------------------------------------------------------------------
void bkrl::close_cancel(context& ctx, command_translator& commands)
{
    ctx.out.write("Nevermind.");
    commands.on_command_result(command_type::close, command_result::canceled);
}

//--------------------------------------------------------------------------------------------------
void bkrl::close_door_at(
    context&             ctx
  , command_translator&  commands
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
) {
    if (set_door_state(current_map, where, door::state::closed)) {
        ctx.out.write("You close the door.");
        commands.on_command_result(command_type::close, command_result::ok_advance);
    } else {
        ctx.out.write("You fail to close the door.");
        commands.on_command_result(command_type::close, command_result::ok_no_advance);
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::close_doors(
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
            if (doors.at(v)) {
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
void bkrl::close_around(
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

//--------------------------------------------------------------------------------------------------
void bkrl::drop_item(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , inventory&          imenu
) {
    drop_item_at(ctx, commands, subject, current_map, subject.position(), imenu);
}

//--------------------------------------------------------------------------------------------------
void bkrl::drop_item_at(
    context&             ctx
  , command_translator&  commands
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
  , inventory&           imenu
) {
    auto& items = subject.item_list();

    if (items.empty()) {
        ctx.out.write("You have nothing to drop.");
        commands.on_command_result(command_type::drop, command_result::none_present);
        return;
    }

    if (abs_max(where - subject.position()) > 1) {
        ctx.out.write("You can't drop that there from here.");
        commands.on_command_result(command_type::drop, command_result::out_of_range);
        return;
    }

    imenu.set_on_action([&, where](inventory::action const action, int) {
        switch (action) {
        case inventory::action::cancel:
            dismiss_item_list(imenu, commands, command_type::drop, command_result::canceled);
            break;
        case inventory::action::confirm: {
            auto const result = drop_item_at(ctx, subject, current_map, items, imenu.data());
            dismiss_item_list(imenu, commands, command_type::drop, result);
            break;
        }
        case inventory::action::select: BK_FALLTHROUGH
        case inventory::action::equip:  BK_FALLTHROUGH
        case inventory::action::get:    BK_FALLTHROUGH
        case inventory::action::drop:   break;
        default:                        BK_UNREACHABLE;
        }
    });

    commands.push_handler(default_item_list_handler(populate_item_list(
        ctx, imenu, items, "Drop which item?")));
}

//--------------------------------------------------------------------------------------------------
void bkrl::get_item_at(
    context&             ctx
  , command_translator&  commands
  , creature&            subject
  , map&                 current_map
  , bklib::ipoint2 const where
  , inventory&           imenu
) {
    if (abs_max(where - subject.position()) > 1) {
        ctx.out.write("You can't get that there from here.");
        commands.on_command_result(command_type::get, command_result::out_of_range);
        return;
    }

    item_pile* const pile = current_map.items_at(where);
    if (!pile) {
        ctx.out.write("There is nothing there to get.");
        commands.on_command_result(command_type::get, command_result::none_present);
        return;
    }

    imenu.set_on_action([&, where](inventory::action const action, int const index) {
        switch (action) {
        case inventory::action::cancel:
            dismiss_item_list(imenu, commands, command_type::get, command_result::canceled);
            break;
        case inventory::action::confirm: {
            item_pile::iterator it = imenu.data();
            item& itm = *it;

            if (!subject.can_get_item(itm)) {
                ctx.out.write("You can't get the {}.", itm.friendly_name(ctx));
                commands.on_command_result(command_type::get, command_result::failed);
                break;
            }

            ctx.out.write("You got the {}.", itm.friendly_name(ctx));
            current_map.move_item_at(where, it, subject.item_list());
            commands.on_command_result(command_type::get, command_result::ok_advance);
            imenu.remove(index);

            if (imenu.empty()) {
                dismiss_item_list(imenu, commands, command_type::get, command_result::none_present);
                break;
            }

            break;
        }
        case inventory::action::select: BK_FALLTHROUGH
        case inventory::action::equip:  BK_FALLTHROUGH
        case inventory::action::get:    BK_FALLTHROUGH
        case inventory::action::drop:   break;
        default:                        BK_UNREACHABLE;
        }
    });

    commands.push_handler(default_item_list_handler(populate_item_list(
        ctx, imenu, *pile, "Get which item?")));
}

//--------------------------------------------------------------------------------------------------
void bkrl::get_item_at(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , inventory&          imenu
) {
    get_item_at(ctx, commands, subject, current_map, subject.position(), imenu);
}

//--------------------------------------------------------------------------------------------------
void bkrl::display_quit_prompt(context& ctx, command_translator& commands)
{
    ctx.out.write("Are you sure you want to quit? Y/N");
    commands.push_handler(filter_text_and_raw([&](command const& cmd) {
        if (cmd.type == command_type::yes) {
            commands.on_command_result(command_type::quit, command_result::ok_advance);
        } else if (cmd.type == command_type::no) {
            commands.on_command_result(command_type::quit, command_result::canceled);
            ctx.out.write("Ok.");
        } else if (cmd.type ==  command_type::cancel) {
            ctx.out.write("Canceled.");
            commands.on_command_result(command_type::quit, command_result::canceled);
        } else {
            ctx.out.write("Invalid choice. Yes, no, or cancel.");
            return command_handler_result::capture;
        }

        return command_handler_result::detach;
    }));
}
