#include "json_util.hpp"

#include "bklib/assert.hpp"
#include "bklib/json.hpp"
#include "bklib/utility.hpp"

namespace {

//--------------------------------------------------------------------------------------------------
//! JSON parser for definition files
//!
//! { "file_type": "my_file_type"
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
