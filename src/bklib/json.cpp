#include "json.hpp"

// to prevent warnings about weak vtables
bklib::json_parser_base::~json_parser_base() noexcept = default;
bklib::json_string_parser::~json_string_parser() noexcept = default;
