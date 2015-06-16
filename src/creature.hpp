#pragma once

#include "identifier.hpp"
#include "random.hpp"
#include "item.hpp"

#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include "bklib/hash.hpp"
#include "bklib/spatial_map.hpp"

#include <vector>
#include <bitset>

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
//!
//--------------------------------------------------------------------------------------------------
enum class creature_flag : unsigned {
    is_player
  , is_aggressive

  , enum_size
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
class creature_flags {
public:
    creature_flags() = default;

    void set(creature_flag const flag) noexcept {
        flags_.set(value_of_(flag));
    }

    bool test(creature_flag const flag) const noexcept{
        return flags_.test(value_of_(flag));
    }

    void clear(creature_flag const flag) noexcept {
        flags_.reset(value_of_(flag));
    }
private:
    static size_t value_of_(creature_flag const flag) noexcept {
        auto const result = static_cast<size_t>(flag);
        BK_PRECONDITION(result < static_cast<size_t>(creature_flag::enum_size));
        return result;
    }

    std::bitset<static_cast<size_t>(creature_flag::enum_size)> flags_;
};

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
    bklib::utf8_string symbol;
    bklib::utf8_string symbol_color;
    creature_flags     flags;
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
    creature_def_id def() const noexcept;

    bool can_get_items(item_pile const& ip) const;
    bool can_get_item(item const& i) const;

    void get_item(item&& i);
    void get_items(item_pile&& ip);
private:
    creature(creature_instance_id id, creature_def const& def, bklib::ipoint2 p);

    creature_instance_id id_;
    creature_def_id      def_;
    bklib::ipoint2       pos_;
    creature_stats       stats_;
    item_pile            items_;
    creature_flags       flags_;
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
class creature_dictionary {
public:
    enum class load_from_file_t   {} static constexpr const load_from_file   {};
    enum class load_from_string_t {} static constexpr const load_from_string {};

    ~creature_dictionary();
    creature_dictionary();
    creature_dictionary(bklib::utf8_string_view filename, load_from_file_t);
    creature_dictionary(bklib::utf8_string_view string, load_from_string_t);

    int size() const noexcept;

    creature_def const* operator[](creature_def_id id) const;
    creature_def const* operator[](uint32_t hash) const;

    bool insert(creature_def def);
private:
    std::unique_ptr<detail::creature_dictionary_impl> impl_;
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

    explicit creature_factory(creature_dictionary& dic);
    ~creature_factory();

    creature create(random_state& random, creature_def_id def, bklib::ipoint2 p);
    creature create(random_state& random, creature_def const& def, bklib::ipoint2 p);

    creature_dictionary const& dictionary() const noexcept {
        return dic_.get();
    }
private:
    creature_instance_id::value_type next_id_;
    std::reference_wrapper<creature_dictionary const> dic_;
};

using creature_map = bklib::spatial_map_2d<creature>;

////////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace bkrl
////////////////////////////////////////////////////////////////////////////////////////////////////
