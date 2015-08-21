#pragma once

#include "commands.hpp"
#include "item.hpp"

#include "bklib/math.hpp"
#include "bklib/string.hpp"

#include <memory>
#include <functional>
#include <cstddef>

namespace bkrl {

struct context;
class  renderer;
class  text_renderer;
struct color_def;
struct mouse_state;
struct mouse_button_state;

class inventory {
public:
    using data_t = item_pile::iterator;

    struct row_t {
        bklib::utf8_string_view symbol;
        bklib::utf8_string_view name;
        data_t                  data;
    };

    virtual ~inventory();

    static constexpr int const at_end = -1;
    static constexpr int const at_beg = 0;
    static constexpr int const selection_current = -1;
    static constexpr int const selection_none = -1;

    virtual void insert(row_t&& r, int where = at_end) = 0;
    virtual void remove(int where = selection_current) = 0;
    virtual void clear() = 0;

    virtual void set_title(bklib::utf8_string_view title) = 0;
    virtual void draw(renderer& render) = 0;
    virtual int count() const noexcept = 0;
    virtual bool empty() const noexcept = 0;

    virtual void show(bool visible) = 0;
    virtual bool is_visible() const noexcept = 0;

    virtual void resize(bklib::irect bounds) = 0;

    virtual bklib::irect bounds() const = 0;
    virtual bklib::irect client_area() const = 0;
    virtual int row_height() const = 0;

    virtual void move_by(bklib::ivec2 v) = 0;
    virtual void move_to(bklib::ipoint2 p) = 0;

    virtual bklib::ipoint2 position() = 0;

    virtual int selection() const noexcept = 0;

    virtual data_t data(int index = selection_current) const = 0;

    enum class action {
        cancel, select, confirm, equip, get, drop
    };

    //----------------------------------------------------------------------------------------------
    //! @returns true if the inventory is visible and the position given by @p m also intersects
    //!          the inventory; false otherwise.
    //----------------------------------------------------------------------------------------------
    virtual bool on_mouse_move(mouse_state const& m) = 0;
    virtual bool on_mouse_button(mouse_button_state const& m) = 0;
    virtual bool on_mouse_scroll(mouse_state const& m) = 0;
    virtual command_handler_result on_command(bkrl::command cmd) = 0;

    using action_handler_t = std::function<void (action type, int index)>;

    virtual void set_on_action(action_handler_t handler) = 0;
};

std::unique_ptr<inventory> make_item_list(text_renderer& trender);

inventory& populate_item_list(
    context& ctx
  , inventory& imenu
  , item_pile& pile
  , bklib::utf8_string_view title
);

inventory& populate_equipment_list(
    context& ctx
  , inventory& imenu
  , item_pile& pile
  , bklib::utf8_string_view title
);

inline decltype(auto) default_item_list_handler(inventory& imenu) {
    return [&imenu, skip = true](command const& cmd) mutable {
        if (skip && cmd.type == command_type::text) {
            skip = false; // skips the text message to follow
        } else {
            return imenu.on_command(cmd);
        }

        return command_handler_result::capture;
    };
}

inline void dismiss_item_list(
    inventory&           imenu
  , command_translator&  commands
  , command_type   const ct
  , command_result const cr
) {
    imenu.show(false);
    commands.pop_handler();
    commands.on_command_result(ct, cr);
}

} //namespace bkrl
