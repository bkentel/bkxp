#include "json.hpp"

#include "bklib/json.hpp"
#include "bklib/utility.hpp"

namespace {

struct def_parser final : bklib::json_parser_base {
    enum class field : uint32_t {
        none
      , file_type   = bklib::static_djb2_hash("file_type")
      , definitions = bklib::static_djb2_hash("definitions")
    };

    enum class state {
        expect_object, expect_file_type, expect_definitions, expect_end
    };

    def_parser(bklib::utf8_string_view const file_type, json_parser_base& definition_handler)
      : json_parser_base()
      , expected_file_type(file_type)
      , definition_handler(&definition_handler)
    {
        definition_handler.parent = this;
    }

    field current_field = field::none;
    state current_state = state::expect_object;
    bklib::utf8_string_view expected_file_type;
    json_parser_base* definition_handler = nullptr;
    std::function<void ()> const* on_finished_definition = nullptr;

    //----------------------------------------------------------------------------------------------
    bool on_string(const char* const str, size_type const len, bool const) override final {
        bklib::utf8_string_view const value {str, len};

        if (current_field == field::file_type) {
            if (current_state == state::expect_file_type) {
                current_state = state::expect_definitions;
                return value == expected_file_type;
            }
        }

        return false;
    }

    //----------------------------------------------------------------------------------------------
    bool on_start_array() override final {
        if (current_state == state::expect_definitions) {
            current_state = state::expect_end;
            handler = definition_handler;
            return true;
        }

        return false;
    }
    
    //----------------------------------------------------------------------------------------------
    bool on_end_array(size_type const) override final {
        if (current_state == state::expect_end) {
            return true;
        }

        return false;
    }

    //----------------------------------------------------------------------------------------------
    bool on_start_object() override final {
        if (current_state == state::expect_object) {
            current_state = state::expect_file_type;
            return true;
        }

        return false;
    }
    
    //----------------------------------------------------------------------------------------------
    bool on_key(const char* const str, size_type const len, bool const) override final {               
        auto const key_hash = static_cast<field>(bklib::djb2_hash(str, str + len));
        switch (key_hash) {
        default:
            return false;
        case field::file_type :
        case field::definitions :
            current_field = key_hash;
            break;
        }

        return true;
    }
    
    //----------------------------------------------------------------------------------------------
    bool on_end_object(size_type const) override final {
        if (current_state == state::expect_end) {
            return true;
        }

        return false;
    }

    //----------------------------------------------------------------------------------------------
    bool on_finished() override final {
        if (on_finished_definition) {
            (*on_finished_definition)();
        }

        return true;
    }
};

} //namespace

void bkrl::json_parse_definitions(
    bklib::utf8_string_view file_name
  , bklib::utf8_string_view const file_type
  , bklib::json_parser_base& handler
  , std::function<void()> const& on_finish
) {
    auto const buffer = bklib::read_file_to_buffer(file_name);
    rapidjson::Reader reader;
    rapidjson::StringStream ss {buffer.data()};

    def_parser parser {file_type, handler};
    parser.on_finished_definition = &on_finish;

    reader.Parse(ss, parser);
}
