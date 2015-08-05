#pragma once

#include "commands.hpp"
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

class item;
class item_pile;

namespace detail { class inventory_impl; }

class inventory {
public:
    struct data_t {
        int64_t   data0; // at least big enough to store a pointer
        ptrdiff_t data1; // arch dependent type (4 or 8 bytes)
    };

    struct row_t {
        bklib::utf8_string_view symbol;
        bklib::utf8_string_view name;
        data_t                  data;
    };

    explicit inventory(text_renderer& trender);
    ~inventory();

    static constexpr int const insert_at_end = -1;
    static constexpr int const insert_at_beg = 0;
    static constexpr int const current_selection = -1;
    static constexpr int const selection_none = -1;

    void insert(row_t&& r, int where = insert_at_end);
    void remove(int where = insert_at_end);
    void clear();

    void set_title(bklib::utf8_string_view title);
    void draw(renderer& render);
    int count() const;

    void show(bool visible);
    bool is_visible() const;

    void resize(bklib::irect bounds);

    bklib::irect bounds() const;
    bklib::irect client_area() const;
    int row_height() const;

    void move_by(bklib::ivec2 v);
    void move_to(bklib::ipoint2 p);

    bklib::ipoint2 position();

    int selection() const;
    data_t data(int index = current_selection) const;

    enum class action {
        cancel, select, confirm, equip
    };

    //----------------------------------------------------------------------------------------------
    //! @returns true if the inventory is visible and the position given by @p m also intersects
    //!          the inventory; false otherwise.
    //----------------------------------------------------------------------------------------------
    bool mouse_move(mouse_state const& m);
    bool mouse_button(mouse_button_state const& m);
    bool mouse_scroll(mouse_state const& m);
    command_handler_result command(bkrl::command cmd);

    using action_handler_t = std::function<
        void (action type, int index)>;

    void on_action(action_handler_t handler);
private:
    std::unique_ptr<detail::inventory_impl> impl_;
};

namespace detail {
void make_item_list(
    context& ctx
  , inventory& i
  , item_pile const& pile
  , bklib::utf8_string_view title
);

} //namespace detail

inline decltype(auto) make_item_list(
    context& ctx
  , inventory& i
  , item_pile const& pile
  , bklib::utf8_string_view title = ""
) {
    detail::make_item_list(ctx, i, pile, title);

    return [&i, skip = true](command const& cmd) mutable {
        if (skip && cmd.type == command_type::text) {
            skip = false; // skips the text message to follow
        } else {
            return i.command(cmd);
        }

        return command_handler_result::capture;
    };
}

inventory::data_t to_inventory_data(item& itm, int index) noexcept;
std::pair<item&, int> from_inventory_data(inventory::data_t data) noexcept;

} //namespace bkrl
