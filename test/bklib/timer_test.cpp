#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "bklib/timer.hpp"
#include <vector>

TEST_CASE("timer", "[timer][bklib]") {
    using namespace std::chrono_literals;
    using bklib::timer;

    std::vector<int> counts = {0, 0, 0};
    timer t;

    t.add(0.1s, [&](timer::record_t& rec) { ++counts[0]; });
    t.add(0.2s, [&](timer::record_t& rec) { ++counts[1]; });
    t.add(0.5s, [&](timer::record_t& rec) { ++counts[2]; });

    auto const get_time_now = [] {
        return std::chrono::high_resolution_clock::now();
    };

    SECTION("relative counts") {
        auto const start_time = get_time_now();
        auto const test_time = 1s;

        while (get_time_now() - start_time < test_time) {
            t.update();
        }

        REQUIRE(counts[0] == 10);
        REQUIRE(counts[1] == 5);
        REQUIRE(counts[2] == 2);
    }
}

#endif // BK_NO_UNIT_TESTS
