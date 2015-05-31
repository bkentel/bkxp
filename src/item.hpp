#pragma once

#include "identifier.hpp"
#include "random.hpp"

#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include "bklib/hash.hpp"
#include "bklib/spatial_map.hpp"

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

    bklib::utf8_string id;
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
    bklib::ipoint2 position() const noexcept { return pos_; }

    void draw(renderer& render);
    void update();
private:
    item(item_instance_id id, item_def const& def, bklib::ipoint2 p);

    item_instance_id     id_;
    item_def_id          def_;
    creature_instance_id owner_;
    bklib::ipoint2       pos_;
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
class item_factory {
public:
    item create(random_state& random, item_def const& def, bklib::ipoint2 p);
private:
    item_instance_id::value_type next_id_;
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
namespace detail {

struct compare_item_key {
    bool operator()(item_instance_id const key, item const& i) const noexcept {
        return key == i.id();
    }
};

struct compare_item_pos {
    bool operator()(item const& i, int const xx, int const yy) const noexcept {
        auto const p = i.position();
        return x(p) == xx && y(p) == yy;
    }
};

} //namespace detail

using item_map = bklib::spatial_map<item, item_instance_id, detail::compare_item_key, detail::compare_item_pos>;

} //namespace bkrl

namespace std {
template <> struct hash<bkrl::item_def> {
    inline size_t operator()(bkrl::item_def const& k) const noexcept {
        return bklib::hash_value(k.id);
    }
};
} //namespace std