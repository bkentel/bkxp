#ifndef BK_NO_UNIT_TESTS
#   define CATCH_CONFIG_RUNNER
#   include <catch/catch.hpp>
#endif

int run_unit_tests() {
    return Catch::Session().run();
}
