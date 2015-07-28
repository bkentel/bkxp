#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "commands.hpp"

TEST_CASE("command values", "[bkrl][command]") {
    // use compiler warnings to check that all enum values have been checked

    using namespace bklib::literals;
    using ct = bkrl::command_type;

    auto cmd = ct::none;

    #define BK_CHECK_VAL(v) static_assert(static_cast<uint32_t>(ct::v) == #v ## _hash, "")

    switch (cmd) {
        case ct::none           : static_assert(static_cast<uint32_t>(ct::none) == 0, ""); break;
        case ct::text           : static_assert(static_cast<uint32_t>(ct::text) == 1, ""); break;
        case ct::invalid        : BK_CHECK_VAL(invalid); break;
        case ct::scroll         : BK_CHECK_VAL(scroll); break;
        case ct::zoom           : BK_CHECK_VAL(zoom); break;
        case ct::cancel         : BK_CHECK_VAL(cancel); break;
        case ct::yes            : BK_CHECK_VAL(yes); break;
        case ct::no             : BK_CHECK_VAL(no); break;
        case ct::dir_here       : BK_CHECK_VAL(dir_here); break;
        case ct::dir_north      : BK_CHECK_VAL(dir_north); break;
        case ct::dir_south      : BK_CHECK_VAL(dir_south); break;
        case ct::dir_east       : BK_CHECK_VAL(dir_east); break;
        case ct::dir_west       : BK_CHECK_VAL(dir_west); break;
        case ct::dir_n_west     : BK_CHECK_VAL(dir_n_west); break;
        case ct::dir_n_east     : BK_CHECK_VAL(dir_n_east); break;
        case ct::dir_s_west     : BK_CHECK_VAL(dir_s_west); break;
        case ct::dir_s_east     : BK_CHECK_VAL(dir_s_east); break;
        case ct::dir_up         : BK_CHECK_VAL(dir_up); break;
        case ct::dir_down       : BK_CHECK_VAL(dir_down); break;
        case ct::quit           : BK_CHECK_VAL(quit); break;
        case ct::use            : BK_CHECK_VAL(use); break;
        case ct::open           : BK_CHECK_VAL(open); break;
        case ct::close          : BK_CHECK_VAL(close); break;
        case ct::drop           : BK_CHECK_VAL(drop); break;
        case ct::show_inventory : BK_CHECK_VAL(show_inventory); break;
        case ct::get            : BK_CHECK_VAL(get); break;
        default:
            break;
    }

    #undef BK_CHECK_VAL
}

#endif // BK_NO_UNIT_TESTS
