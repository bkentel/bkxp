#pragma once

#include "identifier.hpp"
#include "random.hpp"

#include "definitions.hpp"

#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include "bklib/hash.hpp"
#include "bklib/spatial_map.hpp"
#include "bklib/dictionary.hpp"

#include <forward_list>
#include <array>

namespace bkrl {

class renderer;
struct item_def;
class item;
class item_factory;

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
enum class item_type : int16_t {
    armor, weapon, ring
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
struct item_def : definition_base {
    using id_type = item_def_id;

    explicit item_def(bklib::utf8_string id_string)
      : definition_base {std::move(id_string)}
      , id {bklib::djb2_hash(this->id_string)}
    {
    }

    item_def_id id;
};

inline item_def_id get_id(item_def const& def) noexcept {
    return def.id;
}

inline item_def_id get_id(item_def_id const id) noexcept {
    return id;
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
class item {
    friend item_factory;
public:
    item(item&&) = default;
    item& operator=(item&&) = default;
    item(item const&) = delete;
    item& operator=(item const&) = delete;

    item_instance_id id()  const noexcept { return id_; }
    item_def_id      def() const noexcept { return def_; }

    void draw(renderer& render, bklib::ipoint2 p) const;
    void update();
private:
    item(item_instance_id id, item_def const& def);

    item_instance_id id_;
    item_def_id      def_;
};

//--------------------------------------------------------------------------------------------------
//! @todo use a better data structure (segmented array).
//--------------------------------------------------------------------------------------------------
class item_pile {
public:
    item_pile(item_pile const&) = delete;
    item_pile& operator=(item_pile const&) = delete;
    item_pile(item_pile&&) = default;
    item_pile& operator=(item_pile&&) = default;

    item_pile() = default;

    void insert(item_pile&& ip) {
        ip.move_items_to(*this);
    }

    void insert(item&& itm) {
        items_.push_front(std::move(itm));
    }

    bool empty() const noexcept { return items_.empty(); }

    decltype(auto) begin() const { return std::begin(items_); }
    decltype(auto) end()   const { return std::end(items_); }

    void move_items_to(item_pile& dst) {
        dst.items_.splice_after(dst.items_.before_begin(), items_);
    }

    void move_item_to(item_pile& dst, int const index) {
        BK_PRECONDITION(index >= 0);
        dst.items_.splice_after(dst.items_.before_begin(), items_, std::next(items_.before_begin(), index));
    }
private:
    std::forward_list<item> items_;
};

inline void move_items(item_pile& src, item_pile& dst) {
    src.move_items_to(dst);
}

inline void move_item(item_pile& src, item_pile& dst, int const index) {
    src.move_item_to(dst, index);
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
class item_factory {
public:
    item create(random_state& random, item_def const& def);
private:
    item_instance_id::value_type next_id_;
};

//--------------------------------------------------------------------------------------------------
using item_dictionary = bklib::dictionary<item_def>;
using item_map = bklib::spatial_map_2d<item_pile>;

void load_definitions(item_dictionary& dic, bklib::utf8_string_view data, detail::load_from_string_t);

} //namespace bkrl
