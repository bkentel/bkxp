#include "item.hpp"

bkrl::item::item(
    item_instance_id const  id
  , item_def         const& def
)
  : id_  {id}
  , def_ {idof(def)}
{
}

bkrl::item bkrl::item_factory::create(item_def const& def)
{
    return item {item_instance_id {++next_id_}, def};
}
