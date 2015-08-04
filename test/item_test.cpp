#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>
#include "item.hpp"
#include "bklib/dictionary.hpp"

TEST_CASE("item sanity tests", "[bkrl][item]") {
    using namespace bklib::literals;

    bkrl::random_t random;

    bkrl::item_factory    ifac;
    bkrl::item_dictionary idic;

    auto const def = [] {
        bkrl::item_def result {"test item"};
        result.tags.push_back(bkrl::make_tag("CORPSE"));

        return result;
    }();

    idic.insert_or_discard(def);

    auto const i = ifac.create(random, def);

    REQUIRE(static_cast<uint32_t>(i.id()) == 1);
    REQUIRE(i.def() == def);
    REQUIRE(i.data() == 0);

    REQUIRE(bkrl::has_tag(def, bkrl::make_tag("CORPSE"_hash)));
    REQUIRE(bkrl::has_tag(i, idic, bkrl::make_tag("CORPSE"_hash)));

    REQUIRE_FALSE(bkrl::has_tag(def, bkrl::make_tag("BAD"_hash)));
    REQUIRE_FALSE(bkrl::has_tag(i, idic, bkrl::make_tag("BAD"_hash)));
}

TEST_CASE("item list (pile)", "[bkrl][item]") {
    using namespace bklib::literals;

    bkrl::random_t random;

    bkrl::item_factory    ifac;
    bkrl::item_dictionary idic;
    bkrl::item_def const  item0 {"test item0"};
    bkrl::item_def const  item1 {"test item1"};
    bkrl::item_def const  item2 {"test item2"};

    idic.insert_or_replace(item0);
    idic.insert_or_replace(item1);
    idic.insert_or_replace(item2);

    bkrl::item_pile pile;

    SECTION("insert remove") {
        pile.insert(ifac.create(random, item0));
        REQUIRE_FALSE(pile.empty());
        REQUIRE(pile.begin()->def() == item0);

        pile.insert(ifac.create(random, item1));
        REQUIRE(pile.begin()->def() == item1);

        pile.insert(ifac.create(random, item2));
        REQUIRE(pile.begin()->def() == item2);

        REQUIRE(std::distance(std::begin(pile), std::end(pile)) == 3);

        REQUIRE(pile.remove(2).def() == item0);
        REQUIRE(pile.remove(0).def() == item2);
    }
}

#endif // BK_NO_UNIT_TESTS
