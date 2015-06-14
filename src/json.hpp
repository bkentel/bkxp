#pragma once

#include "bklib/string.hpp"
#include <functional>

namespace bklib { struct json_parser_base; }

namespace bkrl {

void json_parse_definitions(
    bklib::utf8_string_view file_name
  , bklib::utf8_string_view file_type
  , bklib::json_parser_base& handler
  , std::function<void ()> const& on_finish
);

} //namespace bkrl
