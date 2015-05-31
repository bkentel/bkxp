#include "item.hpp"

bkrl::item::item(
    item_instance_id const  id
  , item_def         const& def
  , bklib::ipoint2   const  p
)
  : id_  {id}
  , def_ {idof(def)}
  , owner_ {0}
  , pos_ {p}
{
}

bkrl::item bkrl::item_factory::create(random_state& random, item_def const& def, bklib::ipoint2 p)
{
    return item {item_instance_id {++next_id_}, def, p};
}
