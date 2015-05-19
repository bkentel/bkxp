#pragma once

#include "identifier.hpp"
#include "bklib/string.hpp"
#include "bklib/hash.hpp"

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

    void draw(renderer& render);
    void update();
private:
    item(item_instance_id id, item_def const& def);

    item_instance_id id_;
    item_def_id      def_;
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
class item_factory {
public:
    item create(item_def const& def);
private:
    item_instance_id::value_type next_id_;
};

} //namespace bkrl

namespace std {
template <> struct hash<bkrl::item_def> {
    inline size_t operator()(bkrl::item_def const& k) const noexcept {
        return bklib::hash_value(k.id);
    }
};
} //namespace std