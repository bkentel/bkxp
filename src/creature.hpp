#pragma once

#include "identifier.hpp"
#include "random.hpp"
#include "item.hpp"
#include "definitions.hpp"

#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include "bklib/hash.hpp"
#include "bklib/spatial_map.hpp"
#include "bklib/flag_set.hpp"
#include "bklib/dictionary.hpp"

#include <vector>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

class map;

struct creature_def;
class creature;
class creature_factory;

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
enum class creature_flag : unsigned {
    is_player
  , is_aggressive

  , enum_size
};

using creature_flags = bklib::flag_set<creature_flag>;

//--------------------------------------------------------------------------------------------------
//! The "template" to create an instance of a creature.
//--------------------------------------------------------------------------------------------------
struct creature_def : definition_base {
    using id_type = creature_def_id;

    explicit creature_def(bklib::utf8_string id_string)
      : definition_base {std::move(id_string)}
      , id {bklib::djb2_hash(this->id_string)}
    {
    }

    creature_def_id id;
    creature_flags  flags;
};

inline creature_def_id get_id(creature_def const& def) noexcept {
    return def.id;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
template <typename T>
struct creature_stat {
    T base     = 1;
    T modifier = 0;
};

struct creature_stats {
    creature_stat<int16_t> hp_val;
    creature_stat<int16_t> sp_val;
    creature_stat<int16_t> mp_val;

    creature_stat<int8_t> str_val;
    creature_stat<int8_t> con_val;
    creature_stat<int8_t> dex_val;
    creature_stat<int8_t> int_val;
    creature_stat<int8_t> wis_val;
    creature_stat<int8_t> cha_val;
    creature_stat<int8_t> luc_val;

    int8_t reserved[6];
};

//--------------------------------------------------------------------------------------------------
//! An instance of a creature_def.
//! Move only.
//--------------------------------------------------------------------------------------------------
class creature {
    friend creature_factory;
public:
    creature(creature&&) = default;
    creature& operator=(creature&&) = default;
    creature(creature const&) = delete;
    creature& operator=(creature const&) = delete;

    void advance(random_state& random, map& m);

    bool is_player() const noexcept;

    void move_by(bklib::ivec2 v) noexcept;
    void move_to(bklib::ipoint2 p) noexcept;

    bklib::ipoint2 position() const noexcept;

    creature_instance_id id() const noexcept;
    creature_def_id def() const noexcept;

    bool can_get_items(item_pile const& ip) const;
    bool can_get_item(item const& i) const;

    void get_item(item&& i);
    void get_items(item_pile&& ip);

    void drop_item(item_pile& dst, int i = 0);
    void drop_items(item_pile& dst);

    item_pile const& item_list() const {
        return items_;
    }
private:
    creature(creature_instance_id id, creature_def const& def, bklib::ipoint2 p);

    creature_instance_id id_;
    creature_def_id      def_;
    bklib::ipoint2       pos_;
    creature_stats       stats_;
    item_pile            items_;
    creature_flags       flags_;
};

using creature_map = bklib::spatial_map_2d<creature>;
using creature_dictionary = bklib::dictionary<creature_def>;

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
class creature_factory {
public:
    creature_factory(creature_factory const&) = delete;
    creature_factory& operator=(creature_factory const&) = delete;
    creature_factory(creature_factory&&) = default;
    creature_factory& operator=(creature_factory&&) = default;

    explicit creature_factory(creature_dictionary const& dic);
    ~creature_factory() noexcept;

    creature create(random_state& random, creature_def_id def, bklib::ipoint2 p);
    creature create(random_state& random, creature_def const& def, bklib::ipoint2 p);

    creature_dictionary const& dictionary() const noexcept {
        return *dic_;
    }
private:
    creature_dictionary const* dic_;
    creature_instance_id::value_type next_id_;
};

void load_definitions(creature_dictionary& dic, bklib::utf8_string_view data, detail::load_from_string_t);

void advance(random_state& random, map& m, creature& c);
void advance(random_state& random, map& m, creature_map& cmap);

bool move_by(creature& c, map& m, bklib::ivec2 v);

////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
