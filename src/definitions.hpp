#pragma once

#include "identifier.hpp"

#include "bklib/assert.hpp"
#include "bklib/string.hpp"
#include "bklib/utility.hpp"

#include <vector>

namespace bklib { template <typename T> class dictionary; }

namespace bkrl {

struct creature_def;
struct item_def;
struct color_def;

enum class random_stream : int;
class random_state;

class definitions {
public:
    definitions(bklib::dictionary<creature_def>* const creatures
              , bklib::dictionary<item_def>*     const items
              , bklib::dictionary<color_def>*    const colors
    ) : creature_defs_ {creatures}
      , item_defs_     {items}
      , color_defs_    {colors}
    {
    }

    creature_def const* find(def_id_t<tag_creature> id) const;
    item_def     const* find(def_id_t<tag_item>     id) const;
    color_def    const* find(def_id_t<tag_color>    id) const;

    creature_def const* random_creature(random_state& rnd, random_stream stream) const;
    item_def     const* random_item(random_state& rnd, random_stream stream) const;
    color_def    const* random_color(random_state& rnd, random_stream stream) const;

    bklib::dictionary<color_def> const& colors() const noexcept {
        BK_PRECONDITION(color_defs_);
        return *color_defs_;
    }

    bklib::dictionary<item_def> const& items() const noexcept {
        BK_PRECONDITION(item_defs_);
        return *item_defs_;
    }

    bklib::dictionary<creature_def> const& creatures() const noexcept {
        BK_PRECONDITION(creature_defs_);
        return *creature_defs_;
    }
private:
    bklib::dictionary<creature_def>* creature_defs_;
    bklib::dictionary<item_def>*     item_defs_;
    bklib::dictionary<color_def>*    color_defs_;
};

using tag_list = std::vector<def_id_t<tag_string_tag>>;

inline auto make_tag(bklib::utf8_string_view const str) noexcept {
    return def_id_t<tag_string_tag> {str};
}

inline auto make_tag(uint32_t const hash) noexcept {
    return def_id_t<tag_string_tag> {hash};
}

inline bool has_tag(tag_list const& tags, bkrl::def_id_t<tag_string_tag> const tag) noexcept {
    auto const last = std::end(tags);
    auto const it = std::lower_bound(begin(tags), last, tag);
    return it != last && *it == tag;
}

//--------------------------------------------------------------------------------------------------
struct definition_base {
    explicit definition_base(bklib::utf8_string&& def_id_string)
      : id_string {std::move(def_id_string)}
    {
    }

    bklib::utf8_string  id_string;
    bklib::utf8_string  name;
    bklib::utf8_string  description;
    bklib::utf8_string  symbol;
    def_id_t<tag_color> symbol_color;
    tag_list            tags;
};

} //namespace bkrl
