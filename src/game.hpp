#pragma once

#include "bklib/math.hpp"
#include <type_traits>
#include <cstdint>

namespace bkrl {

class command_translator;
struct context;
class creature;
class inventory;
class item;
class map;

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

enum class get_item_result : int {
    ok           //!< success
  , select
  , no_items     //!< nothing to get
  , out_of_range //!< target location is out of range
  , failed
  , canceled
};

using get_item_result_t = result_t<get_item_result>;

//--------------------------------------------------------------------------------------------------
//! Display a menu to select an item to get at the location specified by @p where.
//!
//! @pre @p where is a valid location in @p current_map.
//--------------------------------------------------------------------------------------------------
get_item_result_t get_item(
    context&            ctx         //!< The current context.
  , creature&           subject     //!< The subject doing the 'get'.
  , map&                current_map //!< The current map.
  , bklib::ipoint2      where       //!< The location to get from.
  , inventory&          imenu       //!< The menu used to display a list of choices.
  , command_translator& commands    //!< The command translator stack.
);

enum class drop_item_result : int {
    ok           //!< success
  , select       //!<
  , no_items     //!< nothing to drop
  , out_of_range //!< target location is out of range
  , failed
  , canceled
};

using drop_item_result_t = result_t<drop_item_result>;

//--------------------------------------------------------------------------------------------------
//! Display a menu to select an item to drop at the location specified by @p where.
//!
//! @pre @p where is a valid location in @p current_map.
//--------------------------------------------------------------------------------------------------
drop_item_result_t drop_item(
    context&            ctx         //!< The current context.
  , creature&           subject     //!< The subject doing the 'drop'.
  , map&                current_map //!< The current map.
  , bklib::ipoint2      where       //!< The location to get from.
  , inventory&          imenu       //!< The menu used to display a list of choices.
  , command_translator& commands    //!< The command translator stack.
);

enum class show_inventory_result : int {
    ok       //!< success
  , no_items //!< inventory is empty
  , select
  , canceled
};

using show_inventory_result_t = result_t<show_inventory_result>;

//--------------------------------------------------------------------------------------------------
//! Display a menu listing the @p subjects's current inventory if present.
//!
//! @note Also allows equipping of items.
//--------------------------------------------------------------------------------------------------
show_inventory_result_t show_inventory(
    context&            ctx      //!< The current context.
  , creature&           subject  //!< The subject doing the 'drop'.
  , inventory&          imenu    //!< The menu used to display the list.
  , command_translator& commands //!< The command translator stack.
);

using equip_item_result_t = result_t<equip_result_t>;

//--------------------------------------------------------------------------------------------------
//! Equip the item given by @p itm.
//--------------------------------------------------------------------------------------------------
equip_item_result_t equip_item(
    context&  ctx     //!< The current context.
  , creature& subject //!< The subject doing the 'drop'.
  , item&     itm     //!< The item to equip.
);

enum class open_result : int {
    ok      //!< success
  , select
  , nothing //!< nothing to open
  , failed  //!< couldn't open
  , canceled //! the open was canceled
};

using open_result_t = result_t<open_result>;

//--------------------------------------------------------------------------------------------------
//! Open something around @p subject.
//--------------------------------------------------------------------------------------------------
open_result_t open(
    context&            ctx         //!< The current context.
  , creature&           subject     //!< The subject doing the 'open'.
  , map&                current_map //!< The current map.
  , command_translator& commands    //!< The command translator stack.
);

//--------------------------------------------------------------------------------------------------
open_result_t open_door_at(
    context&       ctx         //!< The current context.
  , creature&      subject     //!< The subject doing the 'open'.
  , map&           current_map //!< The current map.
  , bklib::ipoint2 where
);

//--------------------------------------------------------------------------------------------------
open_result_t open_cont_at(
    context&       ctx         //!< The current context.
  , creature&      subject     //!< The subject doing the 'open'.
  , map&           current_map //!< The current map.
  , bklib::ipoint2 where
);

enum class close_result : int {
    ok      //!< success
  , select
  , nothing //!< nothing to open
  , failed  //!< couldn't open
  , canceled //! the open was canceled
};

using close_result_t = result_t<close_result>;

//--------------------------------------------------------------------------------------------------
//! Open something around @p subject.
//--------------------------------------------------------------------------------------------------
close_result_t close(
    context&            ctx         //!< The current context.
  , creature&           subject     //!< The subject doing the 'open'.
  , map&                current_map //!< The current map.
  , command_translator& commands    //!< The command translator stack.
);

//--------------------------------------------------------------------------------------------------
close_result_t close_door_at(
    context&       ctx         //!< The current context.
  , creature&      subject     //!< The subject doing the 'open'.
  , map&           current_map //!< The current map.
  , bklib::ipoint2 where
);

//--------------------------------------------------------------------------------------------------
close_result_t close_cont_at(
    context&       ctx         //!< The current context.
  , creature&      subject     //!< The subject doing the 'open'.
  , map&           current_map //!< The current map.
  , bklib::ipoint2 where
);

void set_command_result(command_translator& commands, get_item_result result);
void set_command_result(command_translator& commands, drop_item_result result);
void set_command_result(command_translator& commands, show_inventory_result result);
void set_command_result(command_translator& commands, open_result result);
void set_command_result(command_translator& commands, close_result result);
void set_command_result(command_translator& commands, equip_result_t result);

template <typename T>
inline void set_command_result(command_translator& commands, result_t<T> const result) {
    set_command_result(commands, result.value);
}

} //namespace bkrl
