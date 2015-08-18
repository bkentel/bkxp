#pragma once

#include "identifier.hpp"
#include "random.hpp"
#include "definitions.hpp"

#include "bklib/algorithm.hpp"
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
  , is_equippable
  , is_equipped
  , enum_size
};

using item_flags = bklib::flag_set<item_flag>;

enum class equip_slot : uint32_t {
    none
  , hand_main
  , hand_off
  , hand_any
  , head
  , torso
};

using item_slots = bklib::flag_set<equip_slot>;

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
    item_flags flags;
    item_slots slots;
    int32_t weight = 1;
};

void process_tags(item_def& def);

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

template <typename T>
inline void no_throw_swap(T& a, T& b) noexcept {
    using std::swap;
    static_assert(noexcept(swap(a, b)), "throwing swap");
    swap(a, b);
}

enum class item_data_type : uint32_t {
    none
  , container
  , corpse
};

struct item_data_t {
    item_data_t()                              = default;
    item_data_t(item_data_t const&)            = delete;
    item_data_t& operator=(item_data_t const&) = delete;

    item_data_t(item_data_type const data_type, uint64_t const value) noexcept
      : type {data_type}
      , data {value}
    {
    }

    void swap(item_data_t& other) noexcept {
        no_throw_swap(type, other.type);
        no_throw_swap(data, other.data);
    }

    item_data_t(item_data_t&& other) noexcept;
    item_data_t& operator=(item_data_t&& other) noexcept;

    ~item_data_t();

    item_data_type type {item_data_type::none};
    uint64_t       data {};
};

inline void swap(item_data_t& a, item_data_t& b) noexcept { a.swap(b); }

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

    item_slots& slots()       noexcept { return slots_; }
    item_slots  slots() const noexcept { return slots_; }

    item_data_t&       data()       noexcept { return data_; }
    item_data_t const& data() const noexcept { return data_; }

    struct format_flags {
        constexpr format_flags() noexcept {}

        item_def const* use_definition = nullptr;
        int  count = 1;
        bklib::utf8_string_view override_color;
        bool use_color = true;
        bool capitalize = false;
        bool definite = false;
    };

    bklib::utf8_string friendly_name(context const& ctx, format_flags const& flags) const;

    bklib::utf8_string friendly_name(context const& ctx) const {
        return friendly_name(ctx, format_flags {});
    }

    int32_t weight() const { return 0; } // TODO
private:
    item(instance_id_t<tag_item> id, item_def const& def);

    def_id_t<tag_item>      def_;
    instance_id_t<tag_item> id_;
    item_flags              flags_;
    item_slots              slots_;
    item_data_t             data_;
};

inline bool has_flag(item const& itm, item_flag const flag) noexcept {
    return itm.flags().test(flag);
}

//--------------------------------------------------------------------------------------------------
//! @todo use a better data structure (segmented array).
//--------------------------------------------------------------------------------------------------
class item_pile {
public:
    using container_t = std::forward_list<item>;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

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

    //--------------------------------------------------------------------------------------------------
    //! @pre index must be a valid index
    //--------------------------------------------------------------------------------------------------
    item remove(int const index) {
        BK_PRECONDITION(index >= 0);

        auto const last = std::end(items_);
        auto const before = std::next(items_.before_begin(), index);
        BK_PRECONDITION(before != last);

        auto const it = std::next(before);
        BK_PRECONDITION(it != last);

        auto result = std::move(*it);
        items_.erase_after(before);

        return result;
    }

    bool empty() const noexcept { return items_.empty(); }

    decltype(auto) begin() const { return std::begin(items_); }
    decltype(auto) end()   const { return std::end(items_); }
    decltype(auto) begin()       { return std::begin(items_); }
    decltype(auto) end()         { return std::end(items_); }

    item const& advance(int const n) const noexcept {
        return const_cast<item_pile*>(this)->advance(n);
    }

    item& advance(int const n) noexcept {
        return *std::next(items_.begin(), n);
    }

    item const* checked_advance(int const n) const noexcept {
        auto it   = items_.begin();
        auto last = items_.end();
        for (int i = 0; i < n; ++i) {
            if (it == last) {
                break;
            }

            ++it;
        }

        return (it != last) ? &*it : nullptr;
    }

    void move_items_to(item_pile& dst) {
        dst.items_.splice_after(dst.items_.before_begin(), items_);
    }

    void move_item_to(item_pile& dst, int const index) {
        BK_PRECONDITION(index >= 0);
        dst.items_.splice_after(dst.items_.before_begin(), items_, std::next(items_.before_begin(), index));
    }

    void move_item_to(item_pile& dst, iterator const where) {
        auto const last = end();
        for (auto it = items_.begin(), prev = items_.before_begin(); it != last; ++it, ++prev) {
            if (it == where) {
                dst.items_.splice_after(dst.items_.before_begin(), items_, prev);
                return;
            }
        }

        BK_PRECONDITION_SAFE(false && "unreachable");
    }

    template <typename Predicate>
    iterator find_if(Predicate&& pred) {
        return find_if_(items_.before_begin(), items_.end(), std::forward<Predicate>(pred));
    }

    template <typename Predicate>
    const_iterator find_if(Predicate&& pred) const {
        return find_if_(items_.before_begin(), items_.end(), std::forward<Predicate>(pred));
    }

    template <typename Predicate>
    iterator find_if(iterator const before, iterator const last, Predicate&& pred) {
        return find_if_(before, last, std::forward<Predicate>(pred));
    }

    template <typename Predicate>
    const_iterator find_if(const_iterator const before, const_iterator const last, Predicate&& pred) const {
        return find_if_(before, last, std::forward<Predicate>(pred));
    }
private:
    template <typename Predicate>
    iterator find_if_(iterator before, iterator const last, Predicate&& pred) {
        for (auto it = ++before; it != last; before = it++) {
            if (pred(*it)) {
                return it;
            }
        }

        return last;
    }

    template <typename Predicate>
    const_iterator find_if_(const_iterator const before, const_iterator const last, Predicate&& pred) const {
        return const_cast<item_pile*>(this)->find_if(before, last, std::forward<Predicate>(pred));
    }

    std::forward_list<item> items_;
};

inline void move_items(item_pile& src, item_pile& dst) {
    src.move_items_to(dst);
}

inline void move_item(item_pile& src, item_pile& dst, int const index) {
    src.move_item_to(dst, index);
}

inline void move_item(item_pile& src, item_pile& dst, item_pile::iterator const it) {
    src.move_item_to(dst, it);
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
class item_factory {
public:
    item_factory() = default;

    item create(random_t& random, item_dictionary const& idic, item_def const& def);
private:
    instance_id_t<tag_item>::type next_id_ = 0;
};

void advance(context& ctx, map& m, item& item);
void advance(context& ctx, map& m, item_pile& items);
void advance(context& ctx, map& m, item_map& imap);

bool has_tag(item_def const& def, def_id_t<tag_string_tag> tag);
bool has_tag(item const& c, item_dictionary const& defs, def_id_t<tag_string_tag> tag);
bool has_tag(context const& ctx, item const& c, def_id_t<tag_string_tag> tag);

template <typename Visitor>
void visit_tags(item_def const& def, Visitor&& visitor) {
    for (auto const& tag : def.tags) {
        visitor(tag);
    }
}

//--------------------------------------------------------------------------------------------------
inline decltype(auto) find_by_id(instance_id_t<tag_item> const id) noexcept {
    return [id](item const& i) { return id == i.id(); };
}

//--------------------------------------------------------------------------------------------------
inline decltype(auto) find_by_flag(item_flag const flag) noexcept {
    return [flag](item const& i) { return i.flags().test(flag); };
}

//--------------------------------------------------------------------------------------------------
inline decltype(auto) find_container() noexcept {
    return [](item_pile const& pile) noexcept {
        return !!bklib::find_maybe(pile, find_by_flag(item_flag::is_container));
    };
}

template <item_data_type Type>
inline auto get_item_data(item& itm) noexcept
  -> std::enable_if_t<Type == bkrl::item_data_type::container, item_pile*>
{
    BK_PRECONDITION(itm.data().type == item_data_type::container);
    BK_PRECONDITION(has_flag(itm, item_flag::is_container));

    auto& data = itm.data();
    return reinterpret_cast<item_pile*>(data.data);
}

template <item_data_type Type>
inline auto get_item_data(item const& itm) noexcept
  -> std::enable_if_t<Type == bkrl::item_data_type::container, item_pile const*>
{
    return get_item_data<item_data_type::container>(const_cast<item&>(itm));
}


} //namespace bkrl
