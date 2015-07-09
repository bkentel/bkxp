#pragma once

#include "definitions.hpp"

#include "bklib/string.hpp"
#include "bklib/json.hpp"

#include <functional>
#include <memory>
#include <vector>
#include <type_traits>

namespace bklib { struct json_parser_base; }
namespace bklib { template <typename> struct string_id; }

namespace bkrl {

using json_select_handler_t = std::function<bklib::json_parser_base* (bklib::utf8_string_view)>;
using json_on_finish_def_t  = std::function<bool ()>;

//--------------------------------------------------------------------------------------------------
//! Create a parser for .def json files.
//! @param json_data The actual json string data.
//! @param select_handler The function used to select a json_parser_base.
//! @param on_finish The function called after each definition is parsed.
//--------------------------------------------------------------------------------------------------
void json_parse_definitions(
    bklib::utf8_string_view json_data
  , json_select_handler_t   select_handler
  , json_on_finish_def_t    on_finish
);

//--------------------------------------------------------------------------------------------------
//! Convenience function for creating a simple json_select_handler_t which matches a single string.
//! @param file_type The file type expected.
//! @param handler The parser which will be used for definitions.
//--------------------------------------------------------------------------------------------------
inline decltype(auto) json_make_select_handler(
    bklib::utf8_string_view const file_type
  , bklib::json_parser_base&      handler
) noexcept {
    return [file_type, h = &handler]
           (bklib::utf8_string_view const string) noexcept -> bklib::json_parser_base*
    {
        if (file_type == string) {
            return h;
        } else {
            BK_ASSERT(false); // TODO
        }

        return nullptr;
    };
}

//--------------------------------------------------------------------------------------------------
// Helper traits for defining json_make_tag_parser
//--------------------------------------------------------------------------------------------------
struct json_make_tag_parser_traits {
    using result_t    = std::unique_ptr<bklib::json_parser_base>;
    using iterator_t  = tag_list::const_iterator;
    using callback_t  = std::function<bool (iterator_t, iterator_t, iterator_t)>;
};

//--------------------------------------------------------------------------------------------------
//! Create a parser for bkrl::tag_list.
//! @param parent An optional parent parser to forward to.
//! @param on_finish
//!   A function to be called when all tags have been parsed. The function is of the form
//!   bool f(begin, unique_end, end) where each of the arguments is a const_iterator.
//!   begin - iterator to the first tag
//!   unique_end - iterator to the end of the unique (as hashed) set of tags
//!   end - iterator to the last tag.
//!   if unique_end != end then duplicate tags (as hashed) were present.
//--------------------------------------------------------------------------------------------------
json_make_tag_parser_traits::result_t
json_make_tag_parser(
    bklib::json_parser_base*                parent
  , json_make_tag_parser_traits::callback_t on_finish
);

//--------------------------------------------------------------------------------------------------
//! Create a parser for bkrl::definition_base.
//! @param parent An optional parent parser to forward to.
//! @param out The definition to write to.
//--------------------------------------------------------------------------------------------------
std::unique_ptr<bklib::json_parser_base>
json_make_base_def_parser(
    bklib::json_parser_base* parent
  , definition_base& out
);

//--------------------------------------------------------------------------------------------------
//! Convert a string -> uint32_t hash -> enum
//! @tparam T The enumerator type to convert to
//--------------------------------------------------------------------------------------------------
template <typename T>
inline T hash_to_enum(char const* const str, size_t const len) noexcept
{
    static_assert(std::is_enum<T>::value, "");

    auto const hash = bklib::djb2_hash(str, str + len);

    using result_t = std::remove_const_t<decltype(hash)>;
    using enum_t = std::underlying_type_t<T>;

    static_assert(std::is_same<result_t, enum_t>::value, "");

    return static_cast<T>(hash);
}

} //namespace bkrl
