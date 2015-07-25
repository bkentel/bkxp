#pragma once

#include "identifier.hpp"
#include "random.hpp"
#include "definitions.hpp"

#include "bklib/assert.hpp"
#include "bklib/math.hpp"
#include "bklib/string.hpp"
#include "bklib/flag_set.hpp"

#include <forward_list>
#include <array>

namespace bklib { template <typename T> class spatial_map_2d; }
namespace bklib { template <typename T> class dictionary; }

namespace bkrl {

class  map;
struct context;
struct item_def;
class  item;
class  item_pile;
class  item_factory;
struct terrain_entry;
using  item_dictionary = bklib::dictionary<item_def>;
using  item_map = bklib::spatial_map_2d<item_pile>;

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
enum class item_flag : uint32_t {
    is_container
  , is_corpse

  , enum_size
};

using item_flags = bklib::flag_set<item_flag>;

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
struct item_def : definition_base {
    using id_type = def_id_t<tag_item>;

    explicit item_def(bklib::utf8_string identifier)
      : definition_base {std::move(identifier)}
      , id {definition_base::id_string}
    {
    }

    id_type id;
    int32_t weight = 1;
};

constexpr inline bool operator==(item_def const& lhs, item_def::id_type const& rhs) noexcept {
    return lhs.id == rhs;
}

constexpr inline bool operator==(item_def::id_type const& lhs, item_def const& rhs) noexcept {
    return rhs == lhs;
}

constexpr inline bool operator!=(item_def const& lhs, item_def::id_type const& rhs) noexcept {
    return !(lhs == rhs);
}

constexpr inline bool operator!=(item_def::id_type const& lhs, item_def const& rhs) noexcept {
    return !(lhs == rhs);
}

inline auto const& get_id(item_def const& def) noexcept {
    return def.id;
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

    instance_id_t<tag_item> id()  const noexcept { return id_; }
    def_id_t<tag_item>      def() const noexcept { return def_; }

    bool can_place_on(terrain_entry const& ter) const;

    void update();

    item_flags& flags()       noexcept { return flags_; }
    item_flags  flags() const noexcept { return flags_; }

    uint64_t& data()       noexcept { return data_; }
    uint64_t  data() const noexcept { return data_; }

    bklib::utf8_string friendly_name(definitions const& defs) const;
private:
    item(instance_id_t<tag_item> id, item_def const& def);

    uint64_t                data_;
    item_flags              flags_;
    instance_id_t<tag_item> id_;
    def_id_t<tag_item>      def_;
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
    decltype(auto) begin()       { return std::begin(items_); }
    decltype(auto) end()         { return std::end(items_); }

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
    item_factory() = default;

    item create(random_t& random, item_def const& def);
private:
    instance_id_t<tag_item>::type next_id_ = 0;
};

void advance(context& ctx, map& m, item& item);
void advance(context& ctx, map& m, item_pile& items);
void advance(context& ctx, map& m, item_map& imap);

bool has_tag(item_def const& def, def_id_t<tag_string_tag> tag);
bool has_tag(item const& c, item_dictionary const& defs, def_id_t<tag_string_tag> tag);
bool has_tag(context const& ctx, item const& c, def_id_t<tag_string_tag> tag);

} //namespace bkrl
