#include "identifier.hpp"
#include "external/format.h"

bklib::utf8_string
bkrl::detail::to_string(uint32_t const hash, bklib::utf8_string_view const str)
{
    if (str.empty() || !str.front()) {
        return fmt::sprintf("[%#08x]", hash);
    }

    return fmt::sprintf("[%#08x {%.*s}]", hash, str.size(), str.data());
}
