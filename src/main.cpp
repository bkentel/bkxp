#include "bklib/assert.hpp"
#include "bklib/exception.hpp"

#include "game.hpp"

int run_unit_tests();

#if !defined(BK_TESTS_ONLY)
void run_game() {
    bkrl::game game;
}
#else
void run_game() {
}
#endif

int main() try {
    run_unit_tests();
    run_game();

    return 0;
} catch (bklib::exception_base const&) {
} catch (boost::exception const&) {
} catch (std::exception const&) {
} catch (...) {
}
