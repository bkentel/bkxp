#include "creature.hpp"
#include "renderer.hpp"
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
void bkrl::creature::draw(renderer& render) const
{
    render.draw_cell(x(pos_), y(pos_), 1);
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::advance(random_state& random, map& m)
{
    auto& rnd = random[random_stream::creature];
    
    if (!x_in_y_chance(rnd, 1, 3)) {
        return;
    }

    int const dx = random_range(rnd, -1, 1);
    int const dy = random_range(rnd, -1, 1);

    m.move_creature_by(*this, bklib::ivec2 {dx, dy});
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::is_player() const noexcept
{
    return flags_.test(creature_flag::is_player);
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::move_by(bklib::ivec2 const v)
{
    pos_ += v;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::move_by(int const dx, int const dy)
{
    return move_by(bklib::ivec2 {dx, dy});
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
bkrl::creature::creature(
    creature_instance_id const  id
  , creature_def         const& def
  , bklib::ipoint2       const  p
) : id_  {id}
  , def_ {def.id}
  , pos_ {p}
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::creature_factory
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bkrl::creature bkrl::creature_factory::create(
    random_state&        random
  , creature_def const&  def
  , bklib::ipoint2 const p
) {
    return creature {creature_instance_id {++next_id_}, def, p};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::creature_dictionary
////////////////////////////////////////////////////////////////////////////////////////////////////
class bkrl::detail::creature_dictionary_impl {
public:
    explicit creature_dictionary_impl(bklib::utf8_string_view const filename);
    
    creature_def const* operator[](creature_def_id id) const;
    creature_def const* operator[](uint32_t hash) const;
private:
    std::vector<creature_def> defs_;
};

//--------------------------------------------------------------------------------------------------
bkrl::detail::creature_dictionary_impl::creature_dictionary_impl(
    bklib::utf8_string_view const filename
) {
    creature_def_parser creature_handler;

    auto json_data = bklib::read_file_to_buffer(filename);
    auto json_data_string = bklib::utf8_string_view {json_data.data(), json_data.size()};

    auto const select_handler = [&](auto const& string) -> bklib::json_parser_base* {
        if (string == "creatures") {
            return &creature_handler;
        }

        return nullptr;
    };

    json_parse_definitions(json_data_string, select_handler, [&] {
        creature_def def {std::move(creature_handler.id)};

        def.name         = std::move(creature_handler.name);
        def.description  = std::move(creature_handler.description);
        def.symbol       = std::move(creature_handler.symbol);
        def.symbol_color = std::move(creature_handler.symbol_color);

        defs_.push_back(std::move(def));

        return true;
    });
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_def const*
bkrl::detail::creature_dictionary_impl::operator[](creature_def_id const id) const
{
    return bklib::find_maybe(defs_, [&](creature_def const& def) {
        return def.id == id;
    });
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_def const*
bkrl::detail::creature_dictionary_impl::operator[](uint32_t const id) const
{
    return (*this)[creature_def_id {id}];
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_dictionary::~creature_dictionary() = default;

//--------------------------------------------------------------------------------------------------
bkrl::creature_dictionary::creature_dictionary(bklib::utf8_string_view const filename)
  : impl_ {std::make_unique<detail::creature_dictionary_impl>(filename)}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_def const*
bkrl::creature_dictionary::operator[](creature_def_id const id) const
{
    return (*impl_)[id];
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_def const*
bkrl::creature_dictionary::operator[](uint32_t const hash) const
{
    return (*impl_)[hash];
}
