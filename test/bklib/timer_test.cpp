#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "bklib/timer.hpp"
#include <vector>

namespace {
inline auto get_time_now() noexcept {
    return std::chrono::high_resolution_clock::now();
}
}

TEST_CASE("timer basic", "[timer][bklib]") {
    using namespace std::chrono_literals;
    using namespace std::chrono;
    using bklib::timer;

    std::vector<int> counts = {0, 0, 0};
    timer t;

    auto const id0 = t.add(0.1s, [&](auto&) { ++counts[0]; });
    auto const id1 = t.add(0.2s, [&](auto&) { ++counts[1]; });
    auto const id2 = t.add(0.5s, [&](auto&) { ++counts[2]; });

    auto const start_time = get_time_now();

    SECTION("basic sanity checks") {
        REQUIRE(!t.empty());
        REQUIRE(t.size() == 3);

        REQUIRE(t.reset(id0));
        REQUIRE(t.remove(id0));
        REQUIRE(!t.remove(id0));

        REQUIRE(t.reset(id1));
        REQUIRE(t.remove(id1));
        REQUIRE(!t.remove(id1));

        REQUIRE(t.reset(id2));
        REQUIRE(t.remove(id2));
        REQUIRE(!t.remove(id2));

        REQUIRE(t.empty());
        REQUIRE(t.size() == 0);
    }

    SECTION("relative counts") {
        bool do_reset = false;
        t.add(0.6s, [&](auto&) { do_reset = true; });

        while (counts[0] < 10 || counts[1] < 5) {
            t.update();
            if (do_reset) {
                do_reset = false;
                REQUIRE(t.reset(id2));
            }
        }

        REQUIRE(counts[0] == 10);
        REQUIRE(counts[1] == 5);
        REQUIRE(counts[2] == 1);

        auto const delta = duration_cast<seconds>(get_time_now() - start_time);
        REQUIRE(delta == 1s);
    }
}

#endif // BK_NO_UNIT_TESTS
