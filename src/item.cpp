#include "item.hpp"
#include "renderer.hpp"

#include "json.hpp"
#include "bklib/json.hpp"

#include <unordered_map>

namespace {

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
struct item_def_parser final : bklib::json_parser_base {
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
void bkrl::item::draw(renderer& render, bklib::ipoint2 const p) const
{
    render.draw_cell(x(p), y(p), 4);
}

//--------------------------------------------------------------------------------------------------
bkrl::item bkrl::item_factory::create(random_state& random, item_def const& def)
{
    return item {item_instance_id {++next_id_}, def};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::item_dictionary
////////////////////////////////////////////////////////////////////////////////////////////////////
class bkrl::detail::item_dictionary_impl {
public:
    item_dictionary_impl() = default;
    item_dictionary_impl(bklib::utf8_string_view filename, item_dictionary::load_from_file_t);
    item_dictionary_impl(bklib::utf8_string_view string, item_dictionary::load_from_string_t);

    int size() const noexcept;

    item_def const* operator[](item_def_id id) const;
    item_def const* operator[](uint32_t hash) const;

    item_def const& random(random_state& random) const;

    bool insert(item_def&& def);
private:
    void load_(bklib::utf8_string_view json);

    std::vector<item_def> defs_;
};

//--------------------------------------------------------------------------------------------------
bkrl::detail::item_dictionary_impl::item_dictionary_impl(
    bklib::utf8_string_view const filename, item_dictionary::load_from_file_t
) {
    auto const json_data = bklib::read_file_to_buffer(filename);
    load_(bklib::utf8_string_view {json_data.data(), json_data.size()});
}

//--------------------------------------------------------------------------------------------------
bkrl::detail::item_dictionary_impl::item_dictionary_impl(
    bklib::utf8_string_view const string, item_dictionary::load_from_string_t
) {
    load_(string);
}

//--------------------------------------------------------------------------------------------------
int bkrl::detail::item_dictionary_impl::size() const noexcept
{
    return defs_.size();
}

//--------------------------------------------------------------------------------------------------
void bkrl::detail::item_dictionary_impl::load_(bklib::utf8_string_view const json)
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

    json_parse_definitions(json, select_handler, [&] {
        item_def def {std::move(item_handler.id)};

        def.name         = std::move(item_handler.name);
        def.description  = std::move(item_handler.description);
        def.symbol       = std::move(item_handler.symbol);
        def.symbol_color = std::move(item_handler.symbol_color);

        insert(std::move(def)); // TODO duplicates

        return true;
    });
}

//--------------------------------------------------------------------------------------------------
bkrl::item_def const*
bkrl::detail::item_dictionary_impl::operator[](item_def_id const id) const
{
    return bklib::find_maybe(defs_, [&](item_def const& def) {
        return def.id == id;
    });
}

//--------------------------------------------------------------------------------------------------
bkrl::item_def const*
bkrl::detail::item_dictionary_impl::operator[](uint32_t const id) const
{
    return (*this)[item_def_id {id}];
}

//--------------------------------------------------------------------------------------------------
bkrl::item_def const& bkrl::detail::item_dictionary_impl::random(random_state& random) const
{
    BK_PRECONDITION(!defs_.empty());

    auto& rnd = random[random_stream::item];
    auto const i = bkrl::random_range(rnd, 0, static_cast<int>(defs_.size()) - 1);

    return defs_[i];
}

//--------------------------------------------------------------------------------------------------
bool bkrl::detail::item_dictionary_impl::insert(item_def&& def) {
    defs_.push_back(std::move(def)); //TODO duplicates
    return true;
}

//--------------------------------------------------------------------------------------------------
bkrl::item_dictionary::~item_dictionary() = default;

//--------------------------------------------------------------------------------------------------
bkrl::item_dictionary::item_dictionary()
  : impl_ {std::make_unique<detail::item_dictionary_impl>()}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::item_dictionary::item_dictionary(
    bklib::utf8_string_view const filename
  , item_dictionary::load_from_file_t
)
  : impl_ {std::make_unique<detail::item_dictionary_impl>(filename, load_from_file)}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::item_dictionary::item_dictionary(
    bklib::utf8_string_view const string
  , item_dictionary::load_from_string_t
)
  : impl_ {std::make_unique<detail::item_dictionary_impl>(string, load_from_string)}
{
}

//--------------------------------------------------------------------------------------------------
int bkrl::item_dictionary::size() const noexcept
{
    return impl_->size();
}

//--------------------------------------------------------------------------------------------------
bkrl::item_def const*
bkrl::item_dictionary::operator[](item_def_id const id) const
{
    return (*impl_)[id];
}

//--------------------------------------------------------------------------------------------------
bkrl::item_def const*
bkrl::item_dictionary::operator[](uint32_t const hash) const
{
    return (*impl_)[hash];
}

//--------------------------------------------------------------------------------------------------
bool bkrl::item_dictionary::insert(item_def def)
{
    return impl_->insert(std::move(def));
}

//--------------------------------------------------------------------------------------------------
bkrl::item_def const& bkrl::item_dictionary::random(random_state& random) const
{
    return impl_->random(random);
}
