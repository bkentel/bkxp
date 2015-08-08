#include "item.hpp"
#include "terrain.hpp"
#include "context.hpp"
#include "map.hpp"
#include "inventory.hpp"
#include "bklib/dictionary.hpp"
#include "external/format.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::item
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bool bkrl::item::can_place_on(terrain_entry const&) const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
bkrl::item::item(
    instance_id_t<tag_item> const  id
  , item_def                const& def
)
  : data_  {}
  , flags_ {def.flags}
  , slots_ {def.slots}
  , id_    {id}
  , def_   {get_id(def)}
{
}

//--------------------------------------------------------------------------------------------------
void bkrl::process_tags(item_def& def)
{
    using namespace bklib::literals;

    for (auto const& tag : def.tags) {
        switch (static_cast<uint32_t>(tag)) {
        case "CORPSE"_hash        : def.flags.set(item_flag::is_corpse);     break;
        case "CAT_WEAPON"_hash    : def.flags.set(item_flag::is_equippable); break;
        case "CAT_ARMOR"_hash     : def.flags.set(item_flag::is_equippable); break;
        case "EQS_HAND_MAIN"_hash : def.slots.set(equip_slot::hand_main);    break;
        case "EQS_HAND_OFF"_hash  : def.slots.set(equip_slot::hand_off);     break;
        case "EQS_HAND_ANY"_hash  : def.slots.set(equip_slot::hand_any);     break;
        case "EQS_HEAD"_hash      : def.slots.set(equip_slot::head);         break;
        case "EQS_TORSO"_hash     : def.slots.set(equip_slot::torso);        break;
        default:
            break;
        }
    }
}

namespace {

inline size_t size(bklib::utf8_string const& s) noexcept {
    return s.size();
}

inline size_t size(bklib::utf8_string_view const s) noexcept {
    return s.size();
}

template <size_t N>
inline size_t size(char const (&)[N]) noexcept {
    return N - 1;
}

inline constexpr size_t size() noexcept { return 0; }

template <typename Head, typename... Tail>
inline size_t size(Head const& head, Tail const&... tail) noexcept {
    return size(head) + size(tail...);
}

inline bklib::utf8_string& make_string_helper(bklib::utf8_string& str) noexcept {
    return str;
}

template <size_t N>
inline bklib::utf8_string& make_string_helper(bklib::utf8_string& str, char const (&s)[N]) noexcept {
    return str.append(s, N - 1);
}

inline bklib::utf8_string& make_string_helper(bklib::utf8_string& str, bklib::utf8_string_view const next) noexcept {
    return str.append(next.data(), next.size());
}

template <typename T>
inline bklib::utf8_string& make_string_helper(bklib::utf8_string& str, T const& next) noexcept {
    return str.append(next);
}

template <typename First, typename Second, typename... Tail>
inline bklib::utf8_string& make_string_helper(
    bklib::utf8_string& str
  , First const& first
  , Second const& second
  , Tail const&... tail
) noexcept {
    return make_string_helper(make_string_helper(str, first), second, tail...);
}

template <typename Head, typename... Tail>
bklib::utf8_string make_string(Head const& head, Tail const&... tail) {
    bklib::utf8_string result;
    result.reserve(size(head, tail...) + 1);
    make_string_helper(result, head, tail...);
    return result;
}

bklib::utf8_string decorate_name(
    bklib::utf8_string s
  , int  const n
  , bool const definite
) {
    auto const plural = (n != 1);

    if (definite && !plural) {
        return make_string("the ", s);
    }

    if (definite && plural) {
        return make_string("the ", std::to_string(n), s, "s");
    }

    if (!definite && !plural) {
        return make_string("a ", s);
    }

    if (!definite && plural) {
        return make_string(std::to_string(n), s, "s");
    }

    return s;
}

bklib::utf8_string capitalize_name(
    bklib::utf8_string s
  , bool const capitalize
) {
    if (capitalize && !s.empty()) {
        s[0] = std::toupper(s[0], std::locale::classic());
    }

    return s;
}

bklib::utf8_string
get_corpse_name(bkrl::context const& ctx, bkrl::item const& i)
{
    using namespace bkrl;

    auto const cid = def_id_t<tag_creature>(static_cast<uint32_t>(i.data()));
    if (!cid) {
        return "{unknown}";
    }

    auto const cdef = ctx.data.find(cid);
    if (!cdef || cdef->name.empty()) {
        return to_string(cid);
    }

    return cdef->name;
}

}

//--------------------------------------------------------------------------------------------------
bklib::utf8_string bkrl::item::friendly_name(context const& ctx, item::format_flags const& f) const
{
    constexpr auto const c_error = bklib::make_string_view("r");
    constexpr auto const c_item  = bklib::make_string_view("gy");

    auto const apply_color = [f](bklib::utf8_string s, bklib::utf8_string_view color) {
        if (!f.use_color) {
            return s;
        }

        if (!f.override_color.empty()) {
            color = f.override_color;
        }

        return make_string("<color=", color, ">", s, "</color>");
    };

    auto const decorate = [f](bklib::utf8_string s) {
        return capitalize_name(decorate_name(s, f.count, f.definite), f.capitalize);
    };

    auto const idef = f.use_definition
      ?  f.use_definition
      :  ctx.data.find(def());

    if (!idef) {
        return apply_color(to_string(def()), c_error);
    }

    if (idef->name.empty()) {
        return apply_color(make_string("{", idef->id_string, "}"), c_error);
    }

    return decorate(
        flags().test(item_flag::is_corpse)
          ? fmt::format("the remains of {}", apply_color(get_corpse_name(ctx, *this), c_item))
          : apply_color(idef->name, c_item));
}

//--------------------------------------------------------------------------------------------------
bkrl::item bkrl::item_factory::create(random_t& random, item_def const& def)
{
    return item {instance_id_t<tag_item> {++next_id_}, def};
}

//--------------------------------------------------------------------------------------------------
void bkrl::advance(context& ctx, map& m, item& item)
{
}

//--------------------------------------------------------------------------------------------------
void bkrl::advance(context& ctx, map& m, item_pile& items)
{
    for (auto& itm : items) {
        advance(ctx, m, itm);
    }
}

//--------------------------------------------------------------------------------------------------
void bkrl::advance(context& ctx, map& m, item_map& imap)
{
    imap.for_each_data([&](item_pile& items) {
        advance(ctx, m, items);
    });
}

//--------------------------------------------------------------------------------------------------
bool bkrl::has_tag(item_def const& def, def_id_t<tag_string_tag> const tag)
{
    return has_tag(def.tags, tag);
}

//--------------------------------------------------------------------------------------------------
bool bkrl::has_tag(item const& c, item_dictionary const& defs, def_id_t<tag_string_tag> const tag)
{
    if (auto const def = defs.find(c.def())) {
        return has_tag(*def, tag);
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::has_tag(context const& ctx, item const& c, def_id_t<tag_string_tag> const tag)
{
    return has_tag(c, ctx.data.items(), tag);
}
