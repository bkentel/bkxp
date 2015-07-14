#pragma once

#include "identifier.hpp"
#include "random.hpp"
#include "item.hpp"
#include "definitions.hpp"

#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include "bklib/flag_set.hpp"

#include <vector>

namespace bklib { template <typename T> class spatial_map_2d; }
namespace bklib { template <typename T> class dictionary; }

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

struct context;
class  map;
struct creature_def;
class  creature;
class  creature_factory;
struct terrain_entry;
using  creature_map = bklib::spatial_map_2d<creature>;
using  creature_dictionary = bklib::dictionary<creature_def>;

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
enum class creature_flag : uint32_t {
    is_player
  , is_aggressive

  , enum_size
};

using creature_flags = bklib::flag_set<creature_flag>;

//--------------------------------------------------------------------------------------------------
//! The "template" to create an instance of a creature.
//--------------------------------------------------------------------------------------------------
struct creature_def : definition_base {
    using id_type = def_id_t<tag_creature>;

    explicit creature_def(bklib::utf8_string def_id_string)
      : definition_base {std::move(def_id_string)}
      , id {id_string}
    {
    }

    id_type        id;
    creature_flags flags;
};

inline auto const& get_id(creature_def const& def) noexcept {
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

    instance_id_t<tag_creature> id() const noexcept;
    def_id_t<tag_creature> def() const noexcept;

    bool can_get_items(item_pile const& ip) const;
    bool can_get_item(item const& i) const;

    bool can_enter_terrain(terrain_entry const& ter) const;

    void get_item(item&& i);
    void get_items(item_pile&& ip);

    void drop_item(item_pile& dst, int i = 0);
    void drop_items(item_pile& dst);

    item_pile const& item_list() const {
        return items_;
    }
private:
    creature(instance_id_t<tag_creature> id, creature_def const& def, bklib::ipoint2 p);

    instance_id_t<tag_creature> id_;
    def_id_t<tag_creature>      def_;
    bklib::ipoint2       pos_;
    creature_stats       stats_;
    item_pile            items_;
    creature_flags       flags_;
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
class creature_factory {
public:
    creature_factory(creature_factory const&) = delete;
    creature_factory& operator=(creature_factory const&) = delete;
    creature_factory(creature_factory&&) = default;
    creature_factory& operator=(creature_factory&&) = default;

    creature_factory();
    ~creature_factory() noexcept;

    creature create(random_t& random, creature_def const& def, bklib::ipoint2 p);
private:
    instance_id_t<tag_creature>::type next_id_;
};

bool move_by(creature& c, map& m, bklib::ivec2 v);

void advance(context& ctx, map& m, creature& c);
void advance(context& ctx, map& m, creature_map& cmap);

////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
