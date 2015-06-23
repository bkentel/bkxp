#include "creature.hpp"
#include "map.hpp"

#include "json.hpp"
#include "bklib/json.hpp"

#include <functional>

namespace {

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
struct creature_def_parser final : bklib::json_parser_base {
    using json_parser_base::json_parser_base;

    enum class field : uint32_t {
        id           = bklib::static_djb2_hash("id")
      , name         = bklib::static_djb2_hash("name")
      , description  = bklib::static_djb2_hash("description")
      , symbol       = bklib::static_djb2_hash("symbol")
      , symbol_color = bklib::static_djb2_hash("symbol_color")
    };

    //----------------------------------------------------------------------------------------------
    bool on_key(const char* const str, size_type const len, bool const) override final {
        auto const get_string = [this](bklib::utf8_string& out) {
            handler = &string_parser;
            string_parser.out = &out;
        };

        auto const key_hash = static_cast<field>(bklib::djb2_hash(str, str + len));
        switch (key_hash) {
        default:
            return false;
        case field::id:           get_string(id);           break;
        case field::name:         get_string(name);         break;
        case field::description:  get_string(description);  break;
        case field::symbol:       get_string(symbol);       break;
        case field::symbol_color: get_string(symbol_color); break;
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_object(size_type const) override final {
        if (parent) {
            return parent->on_finished();
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_finished() override final {
        handler = this;
        return true;
    }

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------
    bklib::json_string_parser string_parser {this};

    bklib::utf8_string id;
    bklib::utf8_string name;
    bklib::utf8_string description;
    bklib::utf8_string symbol;
    bklib::utf8_string symbol_color;
};
} //namespace

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::creature
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::is_player() const noexcept
{
    return flags_.test(creature_flag::is_player);
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::move_by(bklib::ivec2 const v) noexcept
{
    pos_ += v;
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::move_to(bklib::ipoint2 const p) noexcept
{
    pos_ = p;
}

//--------------------------------------------------------------------------------------------------
bklib::ipoint2 bkrl::creature::position() const noexcept
{
    return pos_;
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_instance_id bkrl::creature::id() const noexcept
{
    return id_;
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_def_id bkrl::creature::def() const noexcept
{
    return def_;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::can_get_items(item_pile const& ip) const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::can_get_item(item const& i) const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::get_item(item&& i)
{
    items_.insert(std::move(i));
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::get_items(item_pile&& ip)
{
    items_.insert(std::move(ip));
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::drop_item(item_pile& dst, int const i)
{
    move_item(items_, dst, i);
}

//--------------------------------------------------------------------------------------------------
bkrl::creature::creature(
    creature_instance_id const  id
  , creature_def         const& def
  , bklib::ipoint2       const  p
) : id_  {id}
  , def_ {def.id}
  , pos_ {p}
  , stats_ {}
  , items_ {}
  , flags_ {def.flags}
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::creature_factory
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bkrl::creature_factory::creature_factory(creature_dictionary& dic)
  : next_id_ {0}
  , dic_     {dic}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_factory::~creature_factory() = default;

//--------------------------------------------------------------------------------------------------
bkrl::creature bkrl::creature_factory::create(
    random_state&         random
  , creature_def_id const def
  , bklib::ipoint2  const p
) {
    auto const ptr = dic_.get().find(def);
    BK_PRECONDITION(ptr);

    return create(random, *ptr, p);
}

//--------------------------------------------------------------------------------------------------
bkrl::creature bkrl::creature_factory::create(
    random_state& random
  , creature_def const& def
  , bklib::ipoint2 const p
) {
    return creature {creature_instance_id {++next_id_}, def, p};
}

//--------------------------------------------------------------------------------------------------
void bkrl::load_definitions(creature_dictionary& dic, bklib::utf8_string_view const data, detail::load_from_string_t)
{
    creature_def_parser creature_handler;

    auto const select_handler = [&](auto const& string) -> bklib::json_parser_base* {
        if (string == "creatures") {
            return &creature_handler;
        } else {
            BK_ASSERT(false);
        }

        return nullptr;
    };

    json_parse_definitions(data, select_handler, [&] {
        creature_def def {std::move(creature_handler.id)};

        def.name         = std::move(creature_handler.name);
        def.description  = std::move(creature_handler.description);
        def.symbol       = std::move(creature_handler.symbol);
        def.symbol_color = std::move(creature_handler.symbol_color);

        dic.insert_or_replace(std::move(def)); // TODO duplicates

        return true;
    });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
void bkrl::advance(random_state& random, map& m, creature& c)
{
    if (c.is_player()) {
        return;
    }

    auto& rnd = random[random_stream::creature];

    if (!x_in_y_chance(rnd, 1, 3)) {
        return;
    }

    bklib::ivec2 const v {
        random_range(rnd, -1, 1)
      , random_range(rnd, -1, 1)
    };

    move_by(c, m, v);
}

//--------------------------------------------------------------------------------------------------
void bkrl::advance(random_state& random, map& m, creature_map& cmap)
{
    cmap.for_each_data([&](creature& c) {
        advance(random, m, c);
    });
}

//--------------------------------------------------------------------------------------------------
bool bkrl::move_by(creature& c, map& m, bklib::ivec2 const v)
{
    BK_ASSERT(std::abs(x(v)) <= 1);
    BK_ASSERT(std::abs(y(v)) <= 1);

    auto const from = c.position();
    auto const to   = c.position() + v;

    if (!intersects(m.bounds(), to)) {
        return false;
    }

    auto const& ter = m.at(to);

    switch (ter.type) {
    case terrain_type::empty:
    case terrain_type::floor:
        break;
    case terrain_type::door:
        if (!door {ter}.is_open()) {
            return false;
        }
        break;
    default:
        return false;
    }

    if (m.creature_at(to)) {
        return false;
    }

    m.move_creature_to(c, to);

    return true;
}
