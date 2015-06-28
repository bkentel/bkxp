#include "item.hpp"

#include "json_util.hpp"
#include "bklib/json.hpp"

#include <unordered_map>

using namespace bklib::literals;

namespace {

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
struct item_def_parser final : bklib::json_parser_base {
    using json_parser_base::json_parser_base;

    enum class field : uint32_t {
        id           = "id"_hash
      , name         = "name"_hash
      , description  = "description"_hash
      , symbol       = "symbol"_hash
      , symbol_color = "symbol_color"_hash
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
// bkrl::item
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bkrl::item::item(
    item_instance_id const  id
  , item_def         const& def
)
  : id_  {id}
  , def_ {def.id}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::item bkrl::item_factory::create(random_state& random, item_def const& def)
{
    return item {item_instance_id {++next_id_}, def};
}

//--------------------------------------------------------------------------------------------------
void bkrl::load_definitions(item_dictionary& dic, bklib::utf8_string_view const data, detail::load_from_string_t)
{
    item_def_parser item_handler;

    auto const select_handler = [&](auto const& string) -> bklib::json_parser_base* {
        if (string == "items") {
            return &item_handler;
        } else {
            BK_ASSERT(false);
        }

        return nullptr;
    };

    json_parse_definitions(data, select_handler, [&] {
        item_def def {std::move(item_handler.id)};

        def.name         = std::move(item_handler.name);
        def.description  = std::move(item_handler.description);
        def.symbol       = std::move(item_handler.symbol);
        def.symbol_color = std::move(item_handler.symbol_color);

        dic.insert_or_replace(std::move(def)); // TODO duplicates

        return true;
    });
}
