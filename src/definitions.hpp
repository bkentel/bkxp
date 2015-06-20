#pragma once

#include "random.hpp"

#include "bklib/string.hpp"
#include "bklib/dictionary.hpp"
#include "bklib/assert.hpp"
#include "bklib/utility.hpp"

namespace bkrl {

namespace detail {
    enum class load_from_string_t {};
    enum class load_from_file_t   {};
} //namespace detail

static constexpr detail::load_from_string_t const load_from_string {};
static constexpr detail::load_from_file_t   const load_from_file   {};

struct definition_base {
    explicit definition_base(bklib::utf8_string id_string)
      : id_string {std::move(id_string)}
    {
    }

    bklib::utf8_string id_string;
    bklib::utf8_string name;
    bklib::utf8_string description;
    bklib::utf8_string symbol;
    bklib::utf8_string symbol_color;
};

//--------------------------------------------------------------------------------------------------
template <typename T>
inline void load_definitions(
    bklib::dictionary<T>& dic
  , bklib::utf8_string_view const filename
  , detail::load_from_file_t
) {
    auto const buf = bklib::read_file_to_buffer(filename);
    load_definitions(dic, bklib::utf8_string_view {buf.data(), buf.size()}, load_from_string);
}

//--------------------------------------------------------------------------------------------------
template <typename T>
inline decltype(auto) random_definition(random_t& random, bklib::dictionary<T> const& dic) {
    BK_PRECONDITION(!dic.empty());
    return random_element(random, dic);
}

} //namespace bkrl
