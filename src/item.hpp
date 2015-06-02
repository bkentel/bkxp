#pragma once

#include "identifier.hpp"
#include "random.hpp"

#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include "bklib/hash.hpp"
#include "bklib/spatial_map.hpp"

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
struct item_def {
    using id_t = item_def_id;

    explicit item_def(bklib::utf8_string id)
      : id {bklib::hash_value(id)}
      , id_string {std::move(id)}
      , name {}
      , description {}
    {
    }

    item_def_id        id;
    bklib::utf8_string id_string;
    bklib::utf8_string name;
    bklib::utf8_string description;
};

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

    item_instance_id id() const noexcept { return id_; }

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

    void insert(item&& itm) {
        items_.push_front(std::move(itm));
    }

    bool empty() const noexcept { return items_.empty(); }

    decltype(auto) begin() const { return std::begin(items_); }
    decltype(auto) end()   const { return std::end(items_); }
private:
    std::forward_list<item> items_;
};

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
//
//--------------------------------------------------------------------------------------------------
namespace detail { class item_dictionary_impl; }

class item_dictionary {
public:
    ~item_dictionary();
    explicit item_dictionary(bklib::utf8_string_view filename);

    item_def const* operator[](item_def_id id) const;
private:
    std::unique_ptr<detail::item_dictionary_impl> impl_;
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
//namespace detail {
//
//struct compare_item_key {
//    bool operator()(item_instance_id const key, item const& i) const noexcept {
//        return key == i.id();
//    }
//};
//
//struct compare_item_pos {
//    bool operator()(item const& i, int const xx, int const yy) const noexcept {
//        auto const p = i.position();
//        return x(p) == xx && y(p) == yy;
//    }
//};
//
//} //namespace detail

using item_map = bklib::spatial_map_2d<item_pile>;

} //namespace bkrl

