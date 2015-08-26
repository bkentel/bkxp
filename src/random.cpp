#include "random.hpp"
#include "bklib/exception.hpp"

namespace {
//--------------------------------------------------------------------------------------------------
auto get_integer(bklib::utf8_string_view const str, size_t const offset) {
    struct result_t {
        int    value;
        size_t offset;
    };

    BK_PRECONDITION(str.size() > offset);

    auto const first = str.data() + offset;
    char* last_digit = nullptr;
    auto const result = std::strtoll(first, &last_digit, 10);

    if (result < std::numeric_limits<int>::min()) {
        BOOST_THROW_EXCEPTION(std::underflow_error {"value below minimum for int"});
    } else if (result > std::numeric_limits<int>::max()) {
        BOOST_THROW_EXCEPTION(std::overflow_error {"value above maximum for int"});
    } else if (!result && ((first == last_digit && *first != '0') || (*(last_digit - 1) != '0'))) {
        BOOST_THROW_EXCEPTION(std::invalid_argument {"unable to convert the string to an integer"});
    }

    return result_t {
        static_cast<int>(result)
      , static_cast<size_t>(last_digit - str.data())
    };
};

//--------------------------------------------------------------------------------------------------
bkrl::random_integer from_string(bklib::utf8_string_view const str) {
    using bkrl::random_integer;

    auto const size = str.size();
    if (!size) {
        return {random_integer::uniform_range_t {0, 0}};
    }

    auto const p0 = get_integer(str.data(), 0);
    if (p0.offset == size) {
        return {random_integer::uniform_range_t {p0.value, p0.value}};
    }

    if (p0.offset + 2 > size) {
        BOOST_THROW_EXCEPTION(std::invalid_argument {"expected an additional parameter"});
    }

    auto const p1 = get_integer(str.data(), p0.offset + 1);

    switch (str[p0.offset]) {
    case 'd': {
        if (p1.offset == size) {
            return {random_integer::dice_t {p0.value, p1.value, 0}};
        }

        if (p1.offset + 2 > size) {
            BOOST_THROW_EXCEPTION(std::invalid_argument {"expected an additional parameter"});
        }

        auto const p2 = get_integer(str.data(), p1.offset);
        if (p2.offset != size) {
            BOOST_THROW_EXCEPTION(std::invalid_argument {"unexpected data following input"});
        }

        return {random_integer::dice_t {p0.value, p1.value, p2.value}};
    }
    case '(':
        if (p1.offset + 1 != size) {
            BOOST_THROW_EXCEPTION(std::invalid_argument {"expected an additional parameter"});
        } else if (str[p1.offset] != ')') {
            BOOST_THROW_EXCEPTION(std::invalid_argument {"expected ')'"});
        }

        return {random_integer::gaussian_t {p1.value, p0.value}};
    case '~':
        if (p1.offset != size) {
            BOOST_THROW_EXCEPTION(std::invalid_argument {"unexpected data following input"});
        }

        return {random_integer::uniform_range_t {p0.value, p1.value}};
    default:
        break;
    }

    BOOST_THROW_EXCEPTION(std::invalid_argument {"invalid format"});
}

} //namespace

//--------------------------------------------------------------------------------------------------
bkrl::random_integer bkrl::make_random_integer(bklib::utf8_string_view const str)
{
    auto result = from_string(str);

    switch (result.type) {
    case random_integer::distribution_type::dice :
        if (result.data.d.sides < 1) {
            BOOST_THROW_EXCEPTION(std::invalid_argument {"the side count must be >= 1"});
        } else if (result.data.d.number < 1) {
            BOOST_THROW_EXCEPTION(std::invalid_argument {"the dice count must be >= 1"});
        }
        break;
    case random_integer::distribution_type::uniform :
        if (result.data.u.lo > result.data.u.hi) {
            BOOST_THROW_EXCEPTION(std::invalid_argument {"the range min must be <= max"});
        }
        break;
    case random_integer::distribution_type::gaussian :
        if (result.data.g.variance <= 0) {
            BOOST_THROW_EXCEPTION(std::invalid_argument {"variance must be > 0"});
        }
        break;
    default:
        BK_UNREACHABLE;
    }

    return result;
}
