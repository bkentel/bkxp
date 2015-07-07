#include "item.hpp"

#include "json_util.hpp"
#include "terrain.hpp"
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
      , tags         = "tags"_hash
    };

    item_def_parser()
      : tag_parser {bkrl::json_make_tag_parser(
          [&](auto&& beg, auto&& end, auto&& dup) { return on_finish_tags(beg, end, dup); })
      }
      , def_ {""}
    {
        tag_parser->parent = this;
    }

    //----------------------------------------------------------------------------------------------
    bool on_finish_tags(
        bkrl::json_make_tag_parser_traits::iterator_t const beg
      , bkrl::json_make_tag_parser_traits::iterator_t const end
      , bkrl::json_make_tag_parser_traits::iterator_t const dup
    ) {
        def_.tags.assign(beg, end);

        if (end != dup) {
            return true; //TODO ignore duplicate tags
        }

        return true;
    }

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
        case field::id:           get_string(def_.id_string);   break;
        case field::name:         get_string(def_.name);        break;
        case field::description:  get_string(def_.description); break;
        case field::symbol:       get_string(def_.symbol);      break;
        case field::symbol_color: get_string(symbol_color_);    break;
        case field::tags:         handler = tag_parser.get();   break;
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
    bkrl::item_def get_result() {
        def_.id.reset(def_.id_string);
        def_.symbol_color.reset(symbol_color_);
        return def_;
    }

    //----------------------------------------------------------------------------------------------
    bklib::json_string_parser string_parser {this};
    std::unique_ptr<bklib::json_parser_base> tag_parser;

    bklib::utf8_string symbol_color_;
    bkrl::item_def def_;
};
} //namespace


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
    item_instance_id const  id
  , item_def         const& def
)
  : id_  {id}
  , def_ {get_id(def)}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::item bkrl::item_factory::create(random_t& random, item_def const& def)
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
        dic.insert_or_replace(item_handler.get_result()); // TODO duplicates
        return true;
    });
}
