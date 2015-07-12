#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "json.hpp"
#include "definitions.hpp"
#include "item.hpp"
#include "creature.hpp"
#include "color.hpp"

#include "bklib/algorithm.hpp"

//
// TODO need more tests for edge cases
//

namespace {

bkrl::tag_list make_tag_list(std::initializer_list<char const*> const ilist) {
    bkrl::tag_list result;
    result.reserve(ilist.size());

    std::transform(begin(ilist), end(ilist), std::back_inserter(result), [](char const* const s) {
        return bkrl::tag_list::value_type {s};
    });

    std::sort(begin(result), end(result));

    return result;
}

} //namespace

TEST_CASE("valid item_def parser", "[json][bkrl][item]") {
    bklib::utf8_string_view const json {R"({
      "file_type": "items"
    , "definitions": [
        { "id": "test_id"
        , "name": "test_name"
        , "description": "test_desc"
        , "symbol": "test_symbol"
        , "symbol_color": "test_color"
        , "weight": 123
        , "tags": ["tag0", "tag1"]
        }
      ]
    })"};

    using bkrl::item_def;

    auto const n = bkrl::load_definitions<item_def>(json, [&](item_def const& def) {
        REQUIRE(def.id_string    == "test_id");
        REQUIRE(def.id           == "test_id");
        REQUIRE(def.name         == "test_name");
        REQUIRE(def.description  == "test_desc");
        REQUIRE(def.symbol       == "test_symbol");
        REQUIRE(def.symbol_color == "test_color");
        REQUIRE(def.weight       == 123);

        REQUIRE(equal(def.tags, make_tag_list({"tag0", "tag1"})));

        return true;
    });

    REQUIRE(n == 1);
}

TEST_CASE("valid creature_def parser", "[json][bkrl][creature]") {
    bklib::utf8_string_view const json {R"({
      "file_type": "creatures"
    , "definitions": [
        { "id": "test_id"
        , "name": "test_name"
        , "description": "test_desc"
        , "symbol": "test_symbol"
        , "symbol_color": "test_color"
        , "tags": ["tag0", "tag1"]
        }
      ]
    })"};

    using bkrl::creature_def;

    auto const n = bkrl::load_definitions<creature_def>(json, [&](creature_def const& def) {
        REQUIRE(def.id_string    == "test_id");
        REQUIRE(def.id           == "test_id");
        REQUIRE(def.name         == "test_name");
        REQUIRE(def.description  == "test_desc");
        REQUIRE(def.symbol       == "test_symbol");
        REQUIRE(def.symbol_color == "test_color");

        REQUIRE(equal(def.tags, make_tag_list({"tag0", "tag1"})));

        return true;
    });

    REQUIRE(n == 1);
}

TEST_CASE("valid color_def parser", "[json][bkrl][color]") {
    bklib::utf8_string_view const json {R"({
      "file_type": "colors"
    , "definitions": [
        { "id": "test_id"
        , "short_name": "test_short_name"
        , "value": [0, 1, 2, 3]
        }
      ]
    })"};

    using bkrl::color_def;

    auto const n = bkrl::load_definitions<color_def>(json, [&](color_def const& def) {
        REQUIRE(def.id           == "test_id");
        REQUIRE(def.short_name   == "test_short_name");
        REQUIRE(bklib::equal(def.color, bkrl::color4 {0, 1, 2, 3}));

        return true;
    });

    REQUIRE(n == 1);
}

#endif // BK_NO_UNIT_TESTS
