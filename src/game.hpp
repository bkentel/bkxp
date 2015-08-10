#pragma once

#include "bklib/math.hpp"

namespace bkrl {

class command_translator;
struct context;
class creature;
class inventory;
class map;

void start_game();

//--------------------------------------------------------------------------------------------------
//! Display a menu to select an item to get at the location specified by @p where.
//!
//! @pre @p where is a valid location in @p current_map.
//--------------------------------------------------------------------------------------------------
void get_item(
    context&            ctx         //!< The current context.
  , creature&           subject     //!< The subject doing the 'get'.
  , map&                current_map //!< The current map.
  , bklib::ipoint2      where       //!< The location to get from.
  , inventory&          imenu       //!< The menu used to display a list of choices.
  , command_translator& commands    //!< The command translator stack.
);

//--------------------------------------------------------------------------------------------------
//! Display a menu to select an item to drop at the location specified by @p where.
//!
//! @pre @p where is a valid location in @p current_map.
//--------------------------------------------------------------------------------------------------
void drop_item(
    context&            ctx         //!< The current context.
  , creature&           subject     //!< The subject doing the 'drop'.
  , map&                current_map //!< The current map.
  , bklib::ipoint2      where       //!< The location to get from.
  , inventory&          imenu       //!< The menu used to display a list of choices.
  , command_translator& commands    //!< The command translator stack.
);

} //namespace bkrl
