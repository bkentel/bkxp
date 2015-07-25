#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>
#include "creature.hpp"
#include "bklib/dictionary.hpp"

TEST_CASE("creature sanity tests", "[bkrl][creature]") {
    using namespace bklib::literals;

    bkrl::random_t random;

    bkrl::creature_factory    cfac;
    bkrl::creature_dictionary cdic;

    constexpr bklib::ipoint2 const p {2, 4};

    auto const def = [] {
        bkrl::creature_def result {"test creature"};
        result.tags.push_back(bkrl::make_tag("NO_CORPSE"));

        return result;
    }();

    cdic.insert_or_discard(def);

    auto const c = cfac.create(random, def, p);

    REQUIRE(static_cast<uint32_t>(c.id()) == 1);
    REQUIRE(c.position() == p);
    REQUIRE(c.def() == def);
    REQUIRE_FALSE(c.is_player());
    REQUIRE_FALSE(c.is_dead());
    REQUIRE(c.item_list().empty());

    REQUIRE(bkrl::has_tag(def, bkrl::make_tag("NO_CORPSE"_hash)));
    REQUIRE(bkrl::has_tag(c, cdic, bkrl::make_tag("NO_CORPSE"_hash)));

    REQUIRE_FALSE(bkrl::has_tag(def, bkrl::make_tag("BAD"_hash)));
    REQUIRE_FALSE(bkrl::has_tag(c, cdic, bkrl::make_tag("BAD"_hash)));
}

#endif // BK_NO_UNIT_TESTS
