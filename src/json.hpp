#pragma once

#include "bklib/string.hpp"
#include <functional>

namespace bklib { struct json_parser_base; }

namespace bkrl {

using json_select_handler_t = std::function<bklib::json_parser_base* (bklib::utf8_string_view)>;
using json_on_finish_def_t  = std::function<bool ()>;

//--------------------------------------------------------------------------------------------------
//! 
//--------------------------------------------------------------------------------------------------
void json_parse_definitions(
    bklib::utf8_string_view json_data
  , json_select_handler_t select_handler
  , json_on_finish_def_t on_finish
);

} //namespace bkrl
