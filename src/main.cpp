#include "bklib/assert.hpp"
#include "bklib/exception.hpp"
#include "bklib/string.hpp"

#include "game.hpp"

#include <map>

int run_unit_tests();

namespace {
#if !defined(BK_TESTS_ONLY)
void run_game() {
    bkrl::game game;
}
#else
void run_game() {
}
#endif
} //namespace

int main(int const argc, char const* argv[]) try {
    using namespace bklib::literals;

    bool flag_no_unit_tests = false;
    bool flag_no_run_game   = false;

    std::map<uint32_t, std::reference_wrapper<bool>> const flags {
        {"--no-unit-tests"_hash, std::ref(flag_no_unit_tests)}
      , {"--no-run-game"_hash,   std::ref(flag_no_run_game)}
    };

    for (int i = 1; i < argc; ++i) {
        auto const it = flags.find(bklib::djb2_hash(argv[i]));
        if (it != std::end(flags)) {
            it->second.get() = true;
        }
    }

    if (!flag_no_unit_tests) {
        run_unit_tests();
    }

    if (!flag_no_run_game) {
        run_game();
    }

    return 0;
} catch (bklib::exception_base const&) {
} catch (boost::exception const&) {
} catch (std::exception const&) {
} catch (...) {
}
