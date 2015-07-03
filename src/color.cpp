#include "color.hpp"

#include "json_util.hpp"
#include "bklib/json.hpp"
#include "bklib/assert.hpp"

namespace {
using namespace bklib::literals;

template <typename T>
struct json_datum {
    json_datum() = default;

    template <typename U>
    explicit json_datum(U&& value)
      : data {std::forward<U>(value)}
    {
    }

    template <typename U>
    json_datum& operator=(U&& value) {
        data = std::forward<U>(value);
        ++set_count;
        return *this;
    }

    explicit operator bool() const noexcept {
        return set_count > 0;
    }

    T data;
    int set_count = 0;
};

struct color_def_parser final : bklib::json_parser_base {
    using json_parser_base::json_parser_base;

    enum class field : uint32_t {
        id         = "id"_hash
      , short_name = "short_name"_hash
      , value      = "value"_hash
    };

    enum class state {
        none, color_array_beg, color_array0, color_array1, color_array2, color_array_end
    };

    bool on_start_array() override final {
        if (cur_state != state::color_array_beg) {
            return def_result;
        }

        cur_state = state::color_array0;

        return true;
    }

    bool on_uint(unsigned const n) override final {
        if (n > 0xFF) {
            return def_result;
        }

        auto const val = static_cast<uint8_t>(n & 0xFF);

        switch (cur_state) {
        case state::color_array0: value[0] = val; cur_state = state::color_array1;    break;
        case state::color_array1: value[1] = val; cur_state = state::color_array2;    break;
        case state::color_array2: value[2] = val; cur_state = state::color_array_end; break;
        default:
            return def_result;
        }

        return true;
    }

    bool on_end_array(size_type) override final {
        if (cur_state != state::color_array_end) {
            return def_result;
        }

        cur_state = state::none;

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_key(const char* const str, size_type const len, bool const) override final {
        auto const get_string = [this](bklib::utf8_string& out) {
            handler = &string_parser;
            string_parser.out = &out;
        };

        auto const key_hash = static_cast<field>(bklib::djb2_hash(str, str + len));
        switch (key_hash) {
        case field::id:         get_string(id);         break;
        case field::short_name: get_string(short_name); break;
        case field::value:      cur_state = state::color_array_beg; break;
        default:
            return def_result;
        }

        return true;
    }

    bool on_start_object() override final {
        if (cur_state != state::none) {
            return def_result;
        }

        id.clear();
        short_name.clear();
        value.fill(0);

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_object(size_type const) override final {
        cur_state = state::none;

        if (parent) {
            return parent->on_finished();
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_finished() override final {
        handler = this;
        return true;
    }

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------
    state cur_state = state::none;
    bklib::json_string_parser string_parser {this};

    bklib::utf8_string id;
    bklib::utf8_string short_name;
    bkrl::color4       value;
};

}

//--------------------------------------------------------------------------------------------------
void bkrl::load_definitions(
    color_dictionary& dic
  , bklib::utf8_string_view const data
  , detail::load_from_string_t
) {
    color_def_parser color_handler;

    auto const select_handler = [&](auto const& string) -> bklib::json_parser_base* {
        if (string == "colors") {
            return &color_handler;
        } else {
            BK_ASSERT(false);
        }

        return nullptr;
    };

    json_parse_definitions(data, select_handler, [&] {
        color_def def {std::move(color_handler.id)};

        def.short_name = std::move(color_handler.short_name);
        def.color      = std::move(color_handler.value);

        dic.insert_or_replace(std::move(def)); // TODO duplicates

        return true;
    });
}