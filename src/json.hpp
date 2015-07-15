#pragma once

#include "bklib/string.hpp"
#include "bklib/exception.hpp"
#include <functional>

namespace bklib { template <typename T> class dictionary; }

namespace bkrl { struct item_def; }
namespace bkrl { struct creature_def; }
namespace bkrl { struct color_def; }

namespace bkrl {

struct json_error : virtual bklib::exception_base {
    virtual ~json_error() noexcept;

    json_error() = default;
    json_error(json_error const&) = default;
    json_error(json_error&&) = default;
    json_error& operator=(json_error const&) = default;
    json_error& operator=(json_error&&) = default;

    struct tag_json_error_code;
    struct tag_json_error_offset;
};

// From RapidJson
enum class json_error_code_type : int {
    none = 0                           //!< no error.
  , document_empty                     //!< the document is empty.
  , document_root_not_singular         //!< the document root must not follow by other values.
  , value_invalid                      //!< invalid value.
  , object_miss_name                   //!< missing a name for object member.
  , object_miss_colon                  //!< missing a colon after a name of object member.
  , object_miss_comma_or_curly_bracket //!< missing a comma or '}' after an object member.
  , array_miss_comma_or_square_bracket //!< missing a comma or ']' after an array element.
  , string_unicode_escape_invalid_hex  //!< incorrect hex digit after \\u escape in string.
  , string_unicode_surrogate_invalid   //!< the surrogate pair in string is invalid.
  , string_escape_invalid              //!< invalid escape character in string.
  , string_miss_quotation_mark         //!< missing a closing quotation mark in string.
  , string_invalid_encoding            //!< invalid encoding in string.
  , number_too_big                     //!< number too big to be stored in double.
  , number_miss_fraction               //!< miss fraction part in number.
  , number_miss_exponent               //!< miss exponent in number.
  , termination                        //!< parsing was terminated.
  , unspecific_syntax_error            //!< unspecific syntax error.
};

using json_error_code   = boost::error_info<json_error::tag_json_error_code, json_error_code_type>;
using json_error_offset = boost::error_info<json_error::tag_json_error_offset, size_t>;
using json_error_success_count = boost::error_info<json_error::tag_json_error_offset, int>;

enum class load_from_string_t {};
enum class load_from_file_t   {};

static constexpr load_from_string_t const load_from_string {};
static constexpr load_from_file_t   const load_from_file   {};

int load_definitions(bklib::utf8_string_view data, std::function<bool (item_def     const&)> callback);
int load_definitions(bklib::utf8_string_view data, std::function<bool (creature_def const&)> callback);
int load_definitions(bklib::utf8_string_view data, std::function<bool (color_def    const&)> callback);

template <typename T, typename Callback>
int load_definitions(bklib::utf8_string_view const data, Callback&& callback) {
    return load_definitions(
        data
      , std::function<bool (T const&)> {std::forward<Callback>(callback)}
    );
}

int load_definitions(bklib::dictionary<creature_def>& dic, bklib::utf8_string_view data, load_from_string_t);
int load_definitions(bklib::dictionary<item_def>&     dic, bklib::utf8_string_view data, load_from_string_t);
int load_definitions(bklib::dictionary<color_def>&    dic, bklib::utf8_string_view data, load_from_string_t);

int load_definitions(bklib::dictionary<creature_def>& dic, bklib::utf8_string_view filename, load_from_file_t);
int load_definitions(bklib::dictionary<item_def>&     dic, bklib::utf8_string_view filename, load_from_file_t);
int load_definitions(bklib::dictionary<color_def>&    dic, bklib::utf8_string_view filename, load_from_file_t);

} //namespace bkrl
