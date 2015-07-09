#include "item.hpp"

#include "json_util.hpp"
#include "terrain.hpp"
#include "bklib/json.hpp"

namespace {
using namespace bklib::literals;

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
struct item_def_parser final : bklib::json_parser_base {
    using json_parser_base::json_parser_base;

    enum class field : uint32_t {
        weight = "weight"_hash
    };

    enum class state {
        base, weight
    };

    //----------------------------------------------------------------------------------------------
    bool on_key(const char* const str, size_type const len, bool const) override final {
        switch (bkrl::hash_to_enum<field>(str, len)) {
        case field::weight: current_state_ = state::weight; break;
        default:
            return false;
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_uint(unsigned const n) override final {
        if (current_state_ == state::weight) {
            using weight_t = decltype(bkrl::item_def::weight);

            if (n > static_cast<unsigned>(std::numeric_limits<weight_t>::max())) {
                return def_result; //TODO
            }

            def_.weight = static_cast<weight_t>(n);
            handler = base_parser_.get();
            current_state_ = state::base;
        } else {
            return def_result;
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_start_object() override final {
        def_.id.reset("");
        def_.weight = 1;

        current_state_ = state::base;
        handler = base_parser_.get();
        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_object(size_type const) override final {
        def_.id.reset(def_.id_string);

        if (parent) {
            return parent->on_finished();
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bkrl::item_def get_result() {
        return def_;
    }

    //----------------------------------------------------------------------------------------------
    bkrl::item_def def_ {""};
    std::unique_ptr<bklib::json_parser_base> base_parser_ {bkrl::json_make_base_def_parser(this, def_)};
    state current_state_ {state::base};
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

    json_parse_definitions(data, json_make_select_handler("items", item_handler), [&] {
        dic.insert_or_replace(item_handler.get_result()); // TODO duplicates
        return true;
    });
}
