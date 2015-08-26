#pragma once

#include "identifier.hpp"
#include "random.hpp"
#include "item.hpp"
#include "definitions.hpp"
#include "equip.hpp"

#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include "bklib/flag_set.hpp"
#include "bklib/algorithm.hpp"

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
    random_integer stat_hp;
};

void process_tags(creature_def& def);

constexpr inline bool operator==(creature_def const& lhs, creature_def::id_type const& rhs) noexcept {
    return lhs.id == rhs;
}

constexpr inline bool operator==(creature_def::id_type const& lhs, creature_def const& rhs) noexcept {
    return rhs == lhs;
}

constexpr inline bool operator!=(creature_def const& lhs, creature_def::id_type const& rhs) noexcept {
    return !(lhs == rhs);
}

constexpr inline bool operator!=(creature_def::id_type const& lhs, creature_def const& rhs) noexcept {
    return !(lhs == rhs);
}

inline auto const& get_id(creature_def const& def) noexcept {
    return def.id;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
enum class stat_type {
    health
  , stamina
  , mana
  , strength
  , constitution
  , dexterity
  , intelligence
  , wisdom
  , charisma
  , luck
};

template <typename R = int, typename T, typename U>
inline auto clamped_add(T const lhs, U const rhs) noexcept {
    static_assert(std::is_integral<R>::value && std::is_signed<R>::value, "");
    static_assert(std::is_integral<T>::value && std::is_signed<T>::value, "");
    static_assert(std::is_integral<U>::value && std::is_signed<U>::value, "");

    using type = std::common_type_t<R, T, U>;

    constexpr auto const min = std::numeric_limits<type>::min();
    constexpr auto const max = std::numeric_limits<type>::max();

    type const a {lhs};
    type const b {rhs};

    if (a > 0 && b > max - a) {
        return max; //overflow
    } else if (a < 0 && b < min - a) {
        return min; //underflow
    }

    return a + b;
};

template <typename T, stat_type Stat>
struct creature_stat {
    static_assert(std::is_integral<T>::value, "");
    static_assert(std::is_signed<T>::value, "");

    using value_type = T;
    static constexpr auto const type = Stat;

    auto value() const noexcept {
        return clamped_add(base, modifier);
    }

    template <typename U>
    void modify(U const n) noexcept {
        auto const result = clamped_add(modifier, n);
        modifier = static_cast<T>(
            bklib::clamp<decltype(result)>(
                result
              , std::numeric_limits<T>::min()
              , std::numeric_limits<T>::max()
            )
        );
    }

    T base     = 1;
    T modifier = 0;
};

struct creature_stats {
    creature_stat<int16_t, stat_type::health>  hp_val;
    creature_stat<int16_t, stat_type::stamina> sp_val;
    creature_stat<int16_t, stat_type::mana>    mp_val;

    creature_stat<int8_t, stat_type::strength>     str_val;
    creature_stat<int8_t, stat_type::constitution> con_val;
    creature_stat<int8_t, stat_type::dexterity>    dex_val;
    creature_stat<int8_t, stat_type::intelligence> int_val;
    creature_stat<int8_t, stat_type::wisdom>       wis_val;
    creature_stat<int8_t, stat_type::charisma>     cha_val;
    creature_stat<int8_t, stat_type::luck>         luc_val;

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
    bool is_dead() const noexcept;

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
    void drop_item(item_pile& dst, item_pile::iterator it);
    void drop_items(item_pile& dst);

    //! @pre the item @p i must be already held by this creature
    equipment::result_t equip_item(item& i);

    //! @pre index must be a valid index; i.e. index > 0 && index < size(items_)
    equipment::result_t equip_item(int index);

    bool has_item(item const& i) const;

    item_pile const& item_list() const {
        return items_;
    }

    item_pile& item_list() {
        return items_;
    }

    //TODO change this interface?
    equipment&       equip_list()       { return equip_; }
    equipment const& equip_list() const { return equip_; }

    int modify(stat_type stat, int mod);
    int current(stat_type stat) const noexcept;

    bklib::utf8_string friendly_name(context const& ctx) const;
private:
    creature(instance_id_t<tag_creature> id, creature_def const& def, bklib::ipoint2 p);

    instance_id_t<tag_creature> id_;
    def_id_t<tag_creature>      def_;
    bklib::ipoint2       pos_;
    creature_stats       stats_;
    item_pile            items_;
    equipment            equip_;
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

bool move_by(context& ctx, creature& c, map& m, bklib::ivec2 v);

void attack(context& ctx, map& m, creature& att, creature& def);

void kill(context& ctx, map& m, creature& c);
item_pile* make_corpse(context& ctx, map& m, creature const& c);
item_pile* drop_all(context& ctx, map& m, creature& c);

void advance(context& ctx, map& m, creature& c);
void advance(context& ctx, map& m, creature_map& cmap);

bool has_tag(creature_def const& def, def_id_t<tag_string_tag> tag);
bool has_tag(creature const& c, creature_dictionary const& defs, def_id_t<tag_string_tag> tag);
bool has_tag(context const& ctx, creature const& c, def_id_t<tag_string_tag> tag);

////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
