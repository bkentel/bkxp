#include "json_util.hpp"

#include "bklib/assert.hpp"
#include "bklib/json.hpp"
#include "bklib/utility.hpp"
#include "bklib/string.hpp"

namespace {

using namespace bklib::literals;

//--------------------------------------------------------------------------------------------------
//! JSON parser for tags
//!
//! ["tag0", "tag1", ...]
//--------------------------------------------------------------------------------------------------
struct tag_parser final : public bklib::json_parser_base {
    tag_parser(bklib::json_parser_base* const parent_parser
             , bkrl::json_make_tag_parser_traits::callback_t&& on_finish
    ) : json_parser_base {parent_parser}
      , on_finish_ {std::move(on_finish)}
    {
    }

    //----------------------------------------------------------------------------------------------
    bool on_string(const char* const str, size_type const len, bool) override final {
        tags_.emplace_back(bklib::utf8_string_view {str, len});
        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_start_array() override final {
        tags_.clear();
        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_array(size_type) override final {
        auto const beg = std::begin(tags_);
        auto const end = std::end(tags_);

        std::sort(beg, end);
        auto const uend = std::unique(beg, end);

        if (on_finish_) {
            auto const result = on_finish_(beg, uend, end); // TODO
        }

        if (parent) {
            return parent->on_finished();
        }

        return true;
    }

    bkrl::json_make_tag_parser_traits::callback_t on_finish_;
    bkrl::tag_list tags_;
};

//--------------------------------------------------------------------------------------------------
//! JSON parser for definition files
//!
//! { "file_type":   "my_file_type"
//! , "definitions": [ {...}, ... ]
//! }
//--------------------------------------------------------------------------------------------------
struct def_parser final : bklib::json_parser_base {
    enum class key : uint32_t {
        none
      , file_type   = bklib::static_djb2_hash("file_type")
      , definitions = bklib::static_djb2_hash("definitions")
    };

    enum class state {
        expect_object, expect_string, expect_array, expect_finish
    };

    using select_handler_t = std::function<json_parser_base* (bklib::utf8_string_view)>;
    using finished_def_t   = std::function<bool ()>;

    def_parser(finished_def_t on_finish_def, select_handler_t select_handler)
      : json_parser_base {}
      , finished_def_    {std::move(on_finish_def)}
      , select_handler_  {std::move(select_handler)}
    {
    }

    key               key_            {key::none};
    state             state_          {state::expect_object};
    finished_def_t    finished_def_   {};
    select_handler_t  select_handler_ {};
    json_parser_base* def_handler_    {};

    //----------------------------------------------------------------------------------------------
    bool on_string(const char* const str, size_type const len, bool const) override final {
        if (state_ != state::expect_string) {
            return def_result;
        }

        if (key_ == key::file_type) {
            def_handler_ = select_handler_(bklib::utf8_string_view {str, len});
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_start_array() override final {
        if (state_ != state::expect_array) {
            return def_result;
        }

        state_ = state::expect_object;
        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_array(size_type const) override final {
        handler = this;

        if (state_ != state::expect_object || key_ != key::definitions) {
            return def_result;
        }

        key_ = key::none;

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_start_object() override final {
        if (state_ != state::expect_object) {
            return def_result;
        }

        if (key_ == key::none) {
            return true;
        }

        if (key_ != key::definitions) {
            return def_result;
        }

        if (!def_handler_) {
            return def_result;
        }

        def_handler_->parent = this;
        handler = def_handler_;
        state_ = state::expect_finish;

        handler->StartObject();

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_key(const char* const str, size_type const len, bool const) override final {
        auto const key_from_hash = static_cast<key>(bklib::djb2_hash(str, str + len));

        switch (key_from_hash) {
        case key::file_type   : state_ = state::expect_string; break;
        case key::definitions : state_ = state::expect_array;  break;
        case key::none: BK_FALLTHROUGH
        default:
            return false;
        }

        key_ = key_from_hash;
        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_object(size_type const) override final {
        if (state_ == state::expect_finish) {
            return on_finished();
        }

        handler = this;

        if (key_ != key::none) {
            return def_result;
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_finished() override final {
        BK_PRECONDITION(!!finished_def_);

        if (state_ != state::expect_finish) {
            return def_result;
        }

        handler = this;
        state_ = state::expect_object;

        return finished_def_();
    }
};

//----------------------------------------------------------------------------------------------
//! JSON parser for base_definition
//!
//! { "id":           "string"
//!   "name":         "string"
//!   "description":  "string"
//!   "symbol":       "string"
//!   "symbol_color": "string"
//!   "tags":         ["string", ...]
//! }
//----------------------------------------------------------------------------------------------
struct base_def_parser final : bklib::json_parser_base {
    enum class field : uint32_t {
        none
      , id           = "id"_hash
      , name         = "name"_hash
      , description  = "description"_hash
      , symbol       = "symbol"_hash
      , symbol_color = "symbol_color"_hash
      , tags         = "tags"_hash
    };

    using tag_it = bkrl::json_make_tag_parser_traits::iterator_t const;

    std::unique_ptr<bklib::json_parser_base> make_tag_parser() {
        return bkrl::json_make_tag_parser(this, [&](tag_it beg, tag_it end, tag_it dup) {
            return on_finish_tags(beg, end, dup);
        });
    }

    base_def_parser(bklib::json_parser_base* const parent_parser
                  , bkrl::definition_base&         def_out
    ) : json_parser_base {parent_parser}
      , tag_parser {make_tag_parser()}
      , def_ {&def_out}
    {
        tag_parser->parent = this;
    }

    //----------------------------------------------------------------------------------------------
    bool on_finish_tags(tag_it beg, tag_it end, tag_it dup) {
        def_->tags.assign(beg, end);

        if (end != dup) {
            return true; //TODO ignore duplicate tags
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_key(const char* const str, size_type const len, bool const copy) override final {
        auto const get_string = [this](bklib::utf8_string& out) noexcept {
            handler = &string_parser;
            string_parser.out = &out;
        };

        switch (current_field_ = bkrl::hash_to_enum<field>(str, len)) {
        case field::none:         BK_ASSERT(false);              break;
        case field::id:           get_string(def_->id_string);   break;
        case field::name:         get_string(def_->name);        break;
        case field::description:  get_string(def_->description); break;
        case field::symbol:       get_string(def_->symbol);      break;
        case field::symbol_color: get_string(symbol_color_);     break;
        case field::tags:         handler = tag_parser.get();    break;
        default:
            if (parent) {
                parent->handler = parent;
                return parent->Key(str, len, copy);
            }
            return def_result;
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_start_object() override final {
        def_->description.clear();
        def_->id_string.clear();
        def_->name.clear();
        def_->symbol.clear();
        def_->symbol_color.reset("");
        def_->tags.clear();

        current_field_ = field::none;

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_object(size_type const size) override final {
        if (parent) {
            parent->handler = parent;
            return parent->EndObject(size);
        }

        return def_result;
    }

    //----------------------------------------------------------------------------------------------
    bool on_finished() override final {
        if (current_field_ == field::symbol_color) {
            def_->symbol_color.reset(symbol_color_);
        }

        handler = this;
        return true;
    }

    //----------------------------------------------------------------------------------------------
    bklib::json_string_parser string_parser {this};
    std::unique_ptr<bklib::json_parser_base> tag_parser;

    bklib::utf8_string symbol_color_;
    bkrl::definition_base* def_;
    field current_field_ {field::none};
};

} //namespace

//--------------------------------------------------------------------------------------------------
void bkrl::json_parse_definitions(
    bklib::utf8_string_view const json_data
  , json_select_handler_t         select_handler
  , json_on_finish_def_t          on_finish
) {
    rapidjson::Reader reader;
    rapidjson::StringStream ss {json_data.data()};

    def_parser parser {std::move(on_finish), std::move(select_handler)};
    if (reader.Parse(ss, parser)) {
        return;
    }

    auto const result = reader.GetParseErrorCode();
    if (!result) {
        return;
    }

    if (result == rapidjson::kParseErrorDocumentRootNotSingular) {
        if (reader.GetErrorOffset() != json_data.size()) {
            BK_ASSERT(false); //TODO
        }
    } else {
        BK_ASSERT(false); //TODO
    }
}

//--------------------------------------------------------------------------------------------------
bkrl::json_make_tag_parser_traits::result_t
bkrl::json_make_tag_parser(
    bklib::json_parser_base* const parent
  , json_make_tag_parser_traits::callback_t on_finish
) {
    return std::make_unique<tag_parser>(parent, std::move(on_finish));
}

//--------------------------------------------------------------------------------------------------
std::unique_ptr<bklib::json_parser_base>
bkrl::json_make_base_def_parser(
    bklib::json_parser_base* const parent
  , definition_base& out
) {
    return std::make_unique<base_def_parser>(parent, out);
}
