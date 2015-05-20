#include "bklib/assert.hpp"
#include "bklib/exception.hpp"

#include "game.hpp"

int run_unit_tests();

void main() try {
    run_unit_tests();

    bkrl::game game;

    return;
} catch (bklib::exception_base const&) {
} catch (boost::exception const&) {
} catch (std::exception const&) {
} catch (...) {
}
