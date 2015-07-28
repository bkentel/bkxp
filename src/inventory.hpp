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
class  item_pile;

namespace detail { class inventory_impl; }

class inventory {
public:
    struct row_t {
        bklib::utf8_string_view symbol;
        bklib::utf8_string_view name;
        ptrdiff_t               data;
    };

    explicit inventory(text_renderer& trender);
    ~inventory();

    static constexpr int const insert_at_end = -1;
    static constexpr int const insert_at_beg = 0;

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

    enum class action {
        select, confirm, cancel
    };

    //----------------------------------------------------------------------------------------------
    //! @returns true if the inventory is visible and the position given by @p m also intersects
    //!          the inventory; false otherwise.
    //----------------------------------------------------------------------------------------------
    bool mouse_move(mouse_state const& m);
    bool mouse_button(mouse_button_state const& m);
    bool mouse_scroll(mouse_state const& m);
    void command(bkrl::command cmd);

    using action_handler_t = std::function<
        void (action    type
            , int       index
            , ptrdiff_t data
        )>;

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
            i.command(cmd);
        }

        return command_handler_result::capture;
    };
}

} //namespace bkrl
