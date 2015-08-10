#pragma once

#include "bklib/math.hpp"
#include <type_traits>

namespace bkrl {

class command_translator;
struct context;
class creature;
class inventory;
class item;
class map;

enum class equip_result_t : int;

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
  , no_items     //!< nothing to get
  , out_of_range //!< target location is out of range
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
  , no_items     //!< nothing to drop
  , out_of_range //!< target location is out of range
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
//! Equip the item given by @itm.
//--------------------------------------------------------------------------------------------------
equip_item_result_t equip_item(
    context&  ctx     //!< The current context.
  , creature& subject //!< The subject doing the 'drop'.
  , item&     itm     //!< The item to equip.
);

} //namespace bkrl
