#include "item.hpp"
#include "renderer.hpp"

#include <unordered_map>

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::item
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bkrl::item::item(
    item_instance_id const  id
  , item_def         const& def
)
  : id_  {id}
  , def_ {def.id}
{
}

//--------------------------------------------------------------------------------------------------
void bkrl::item::draw(renderer& render, bklib::ipoint2 const p) const
{
    render.draw_cell(x(p), y(p), 4);
}

//--------------------------------------------------------------------------------------------------
bkrl::item bkrl::item_factory::create(random_state& random, item_def const& def)
{
    return item {item_instance_id {++next_id_}, def};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::item_dictionary
////////////////////////////////////////////////////////////////////////////////////////////////////

class bkrl::detail::item_dictionary_impl {
public:
    explicit item_dictionary_impl(bklib::utf8_string_view filename);

    item_def const* operator[](item_def_id id) const;
private:
    std::unordered_map<item_def_id, item_def> defs_;
};

//--------------------------------------------------------------------------------------------------
bkrl::detail::item_dictionary_impl::item_dictionary_impl(bklib::utf8_string_view const)
{
    auto const add_item = [&](bklib::utf8_string id) {
        item_def def {id};
        defs_.insert(std::make_pair(def.id, std::move(def)));
    };

    add_item("item0");
    add_item("item1");
    add_item("item2");
    add_item("item3");
}

//--------------------------------------------------------------------------------------------------
bkrl::item_def const*
bkrl::detail::item_dictionary_impl::operator[](item_def_id const id) const
{
    auto const it = defs_.find(id);
    return (it != end(defs_)) ? std::addressof(it->second) : nullptr;
}

//--------------------------------------------------------------------------------------------------
bkrl::item_dictionary::~item_dictionary() = default;

//--------------------------------------------------------------------------------------------------
bkrl::item_dictionary::item_dictionary(bklib::utf8_string_view const filename)
  : impl_ {std::make_unique<detail::item_dictionary_impl>(filename)}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::item_def const*
bkrl::item_dictionary::operator[](item_def_id const id) const
{
    return (*impl_)[id];
}

