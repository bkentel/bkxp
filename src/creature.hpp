#pragma once

#include "identifier.hpp"
#include "random.hpp"

#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include "bklib/hash.hpp"
#include "bklib/spatial_map.hpp"

#include <vector>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bkrl {
////////////////////////////////////////////////////////////////////////////////////////////////////

class renderer;
class map;

struct creature_def;
class creature;
class creature_factory;
class creature_dictionary;

namespace detail { class creature_dictionary_impl; }

//--------------------------------------------------------------------------------------------------
//! The "template" to create an instance of a creature.
//--------------------------------------------------------------------------------------------------
struct creature_def {
    using id_t = creature_def_id;

    explicit creature_def(bklib::utf8_string id_string)
      : id {bklib::djb2_hash(id_string)}
      , id_string {std::move(id_string)}
    {
    }

    creature_def_id    id;
    bklib::utf8_string id_string;
    bklib::utf8_string name;
    bklib::utf8_string description;
};

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

    void draw(renderer& render) const;
    void advance(random_state& random, map& m);

    bool is_player() const noexcept;

    bool move_by(bklib::ivec2 const v);
    bool move_by(int dx, int dy);

    bklib::ipoint2 position() const noexcept;

    creature_instance_id id() const noexcept;
private:
    creature(creature_instance_id id, creature_def const& def, bklib::ipoint2 p);

    creature_instance_id id_;
    creature_def_id      def_;
    bklib::ipoint2       pos_;
    creature_stats       stats_;
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
class creature_factory {
public:
    creature create(random_state& random, creature_def const& def, bklib::ipoint2 p);
private:
    creature_instance_id::value_type next_id_;
};

using creature_map = bklib::spatial_map_2d<creature>;

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
class creature_dictionary {
public:
    ~creature_dictionary();
    explicit creature_dictionary(bklib::utf8_string_view filename);

    creature_def const* operator[](creature_def_id id) const;
private:
    std::unique_ptr<detail::creature_dictionary_impl> impl_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
