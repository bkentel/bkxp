#include "color.hpp"

#include "json_util.hpp"
#include "bklib/json.hpp"
#include "bklib/assert.hpp"

namespace {
using namespace bklib::literals;

template <typename T>
struct json_datum {
    json_datum() = default;
    json_datum(json_datum const&) = delete;
    json_datum(json_datum&&) = default;
    json_datum& operator=(json_datum const&) = delete;
    json_datum& operator=(json_datum&&) = default;

    template <typename U>
    json_datum(U&& value)
      : data {std::forward<U>(value)}
    {
    }

    template <typename U>
    json_datum& operator=(U&& value) {
        data = std::forward<U>(value);
        ++count;
        return *this;
    }

    explicit operator bool() const noexcept {
        return count > 0;
    }

    operator T const&() const noexcept {
        return data;
    }

    T& operator->() noexcept { return data; }
    T const & operator->() const noexcept { return data; }

    void reset() {
        count = 0;
    }

    void reset(T const& value) {
        data = value;
        reset();
    }

    T data;
    int count = 0;
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
        case state::color_array0: color_r = val; cur_state = state::color_array1;    break;
        case state::color_array1: color_g = val; cur_state = state::color_array2;    break;
        case state::color_array2: color_b = val; cur_state = state::color_array_end; break;
        case state::none:            BK_FALLTHROUGH
        case state::color_array_end: BK_FALLTHROUGH
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
        case field::id:         get_string(id);         return true;
        case field::short_name: get_string(short_name); return true;
        case field::value:      cur_state = state::color_array_beg; return true;
        }

        return def_result;
    }

    bool on_start_object() override final {
        if (cur_state != state::none) {
            return def_result;
        }

        id.clear();
        short_name.clear();

        color_r.reset(0);
        color_g.reset(0);
        color_b.reset(0);
        color_a.reset(0);

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

    bkrl::color_def get_result() {
        bkrl::color_def result {id};
        result.short_name.assign(short_name);
        result.color[0] = color_r;
        result.color[1] = color_g;
        result.color[2] = color_b;
        result.color[3] = color_a;

        return result;
    }

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------
    state cur_state = state::none;
    bklib::json_string_parser string_parser {this};

    bklib::utf8_string id;
    bklib::utf8_string short_name;

    json_datum<uint8_t> color_r = uint8_t {0};
    json_datum<uint8_t> color_g = uint8_t {0};
    json_datum<uint8_t> color_b = uint8_t {0};
    json_datum<uint8_t> color_a = uint8_t {255};
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
        dic.insert_or_replace(color_handler.get_result()); // TODO duplicates
        return true;
    });
}
