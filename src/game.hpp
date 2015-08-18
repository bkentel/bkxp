#pragma once

#include "map.hpp"
#include "commands.hpp"

#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include "bklib/simple_future.hpp"

#include <type_traits>
#include <cstdint>

namespace bkrl {

class command_translator;
struct context;
class inventory;

enum class equip_result_t : int;
enum class command_type : uint32_t;

//--------------------------------------------------------------------------------------------------
template <typename T>
struct result_t {
    // Check that T is a scoped enum with a member named ok.
    static_assert(std::is_enum<T>::value, "");
    static_assert(!std::is_convertible<T, std::underlying_type_t<T>>::value, "");
    static_assert(static_cast<std::underlying_type_t<T>>(T::ok) || true, "");

    constexpr result_t(T const result) noexcept
      : value {result}
    {
    }

    constexpr explicit operator bool() const noexcept {
        return value == T::ok;
    }

    T value;
};

template <typename T>
constexpr bool operator==(result_t<T> const lhs, result_t<T> const rhs) noexcept {
    return lhs.value == rhs.value;
}

template <typename T>
constexpr bool operator==(result_t<T> const lhs, T const rhs) noexcept {
    return lhs.value == rhs;
}

template <typename T>
constexpr bool operator==(T const lhs, result_t<T> const rhs) noexcept {
    return lhs == rhs.value;
}

template <typename T, typename U>
constexpr bool operator!=(result_t<T> const lhs, U const rhs) noexcept {
    return !(lhs == rhs);
}

template <typename T, typename U>
constexpr bool operator!=(U const lhs, result_t<T> const rhs) noexcept {
    return !(lhs == rhs);
}

void start_game();

//--------------------------------------------------------------------------------------------------
bklib::utf8_string inspect_tile(
    context&        ctx
  , creature const& subject
  , map const&      current_map
  , bklib::ipoint2  where
);

//--------------------------------------------------------------------------------------------------
//! Display a menu to select an item to get at the location specified by @p where.
//!
//! @pre @p where is a valid location in @p current_map.
//--------------------------------------------------------------------------------------------------
void get_item_at(
    context&            ctx         //!< The current context.
  , command_translator& commands    //!< The command translator stack.
  , creature&           subject     //!< The subject doing the 'get'.
  , map&                current_map //!< The current map.
  , bklib::ipoint2      where       //!< The location to get from.
  , inventory&          imenu       //!< The menu used to display a list of choices.
);

void get_item_at(
    context&            ctx         //!< The current context.
  , command_translator& commands    //!< The command translator stack.
  , creature&           subject     //!< The subject doing the 'get'.
  , map&                current_map //!< The current map.
  , inventory&          imenu       //!< The menu used to display a list of choices.
);

//--------------------------------------------------------------------------------------------------
//! Display a menu to select an item to drop at the location specified by @p where.
//!
//! @pre @p where is a valid location in @p current_map.
//--------------------------------------------------------------------------------------------------
void drop_item_at(
    context&            ctx         //!< The current context.
  , command_translator& commands    //!< The command translator stack.
  , creature&           subject     //!< The subject doing the 'drop'.
  , map&                current_map //!< The current map.
  , bklib::ipoint2      where       //!< The location to get from.
  , inventory&          imenu       //!< The menu used to display a list of choices.
);

void drop_item(
    context&            ctx         //!< The current context.
  , command_translator& commands    //!< The command translator stack.
  , creature&           subject     //!< The subject doing the 'drop'.
  , map&                current_map //!< The current map.
  , inventory&          imenu       //!< The menu used to display a list of choices.
);

//--------------------------------------------------------------------------------------------------
//! Equip the item given by @p itm.
//--------------------------------------------------------------------------------------------------
equip_result_t equip_item(
    context&  ctx     //!< The current context.
  , creature& subject //!< The subject doing the 'drop'.
  , item&     itm     //!< The item to equip.
);

/////////////////////////////////

//--------------------------------------------------------------------------------------------------
//! Display a list of items in the container.
//!
//! @pre @p container has item_flag::is_container.
//! @pre @p container has its data set to tem_data_type::container.
//--------------------------------------------------------------------------------------------------
void display_item_list(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , item&               container
  , inventory&          imenu
);

void display_item_list(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , item_pile&          pile
  , inventory&          imenu
  , bklib::utf8_string_view const title
);

//--------------------------------------------------------------------------------------------------
//! Display a message when there is nothing to open.
//!
//! @pre @p subject must be the player.
//--------------------------------------------------------------------------------------------------
void open_nothing(context& ctx, command_translator& commands, creature& subject);

//--------------------------------------------------------------------------------------------------
//! Display a message when opening is canceled.
//!
//! @pre @p subject must be the player.
//--------------------------------------------------------------------------------------------------
void open_cancel(context& ctx, command_translator& commands, creature& subject);

//--------------------------------------------------------------------------------------------------
//! Open the door at the location by where.
//--------------------------------------------------------------------------------------------------
void open_door_at(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , bklib::ipoint2      where
);

//--------------------------------------------------------------------------------------------------
//! Open one of the doors adjacent to subject specified by doors.
//--------------------------------------------------------------------------------------------------
void open_doors(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , find_around_result_t<terrain_entry*> const& doors
);

//--------------------------------------------------------------------------------------------------
//! Open a container from the pile specified by pair.
//--------------------------------------------------------------------------------------------------
void open_containers_at(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , std::pair<item_pile*, item_pile::iterator> const pair
  , inventory&          imenu
);

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
);

//--------------------------------------------------------------------------------------------------
//! Open a door or container adjacent to the subject.
//--------------------------------------------------------------------------------------------------
void open_around(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , inventory&          imenu
);

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
void close_nothing(context& ctx, command_translator& commands);

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
void close_cancel(context& ctx, command_translator& commands);

//--------------------------------------------------------------------------------------------------
//! Open the door at the location adjacent to subject specified by where.
//--------------------------------------------------------------------------------------------------
void close_door_at(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , bklib::ipoint2      where
);

//--------------------------------------------------------------------------------------------------
//! Open one of the doors adjacent to subject specified by doors.
//--------------------------------------------------------------------------------------------------
void close_doors(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
  , find_around_result_t<terrain_entry*> const& doors
);

//--------------------------------------------------------------------------------------------------
//! Close a door (TODO or other widget?) adjacent to the subject
//--------------------------------------------------------------------------------------------------
void close_around(
    context&            ctx
  , command_translator& commands
  , creature&           subject
  , map&                current_map
);

} //namespace bkrl
