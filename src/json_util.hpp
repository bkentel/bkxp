#pragma once

#include "definitions.hpp"
#include "bklib/string.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace bklib { struct json_parser_base; }
namespace bklib { template <typename> struct string_id; }

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

struct json_make_tag_parser_traits {
    using result_t    = std::unique_ptr<bklib::json_parser_base>;
    using iterator_t  = tag_list::const_iterator;
    using callback_t  = std::function<bool(iterator_t, iterator_t, iterator_t)>;
};

json_make_tag_parser_traits::result_t
json_make_tag_parser(json_make_tag_parser_traits::callback_t on_finish);

} //namespace bkrl
