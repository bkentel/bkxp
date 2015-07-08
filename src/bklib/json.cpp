#include "json.hpp"
#include "assert.hpp"

// to prevent warnings about weak vtables
bklib::json_parser_base::~json_parser_base() noexcept = default;
bklib::json_string_parser::~json_string_parser() noexcept = default;

//--------------------------------------------------------------------------------------------------
bool bklib::json_parse_string(json_parser_base& parser, utf8_string_view const json)
{
    rapidjson::Reader reader;
    rapidjson::StringStream ss {json.data()};

    if (reader.Parse(ss, parser)) {
        return true;
    }

    auto const result = reader.GetParseErrorCode();
    if (!result) {
        return false;
    }

    if (result == rapidjson::kParseErrorDocumentRootNotSingular) {
        if (reader.GetErrorOffset() != json.size()) {
            BK_ASSERT(false); //TODO
        }
    } else {
        BK_ASSERT(false); //TODO
    }

    return false;
}
