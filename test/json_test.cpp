#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "json_util.hpp"
#include "definitions.hpp"

TEST_CASE("json definition parser", "[json][bkrl]") {
    bklib::utf8_string_view const json {R"(
        { "file_type": "test"
        , "definitions": []
        }
    )"};

    auto select_handler_count = 0;
    auto const select_handler = [&](bklib::utf8_string_view const file_type) {
        REQUIRE(file_type == "test");
        ++select_handler_count;
        return static_cast<bklib::json_parser_base*>(nullptr);
    };

    auto parsed_count = 0;
    bkrl::json_parse_definitions(json, select_handler, [&] {
        ++parsed_count;
        return true;
    });

    REQUIRE(select_handler_count == 1);
    REQUIRE(parsed_count == 0);
}

TEST_CASE("json tag parser", "[json][bkrl]") {
    auto const do_test = [&](
        bklib::utf8_string_view const json
      , int const expected_unique
      , int const expected_duplicate
      , bkrl::tag_list&& expected_tags
    ) {
        using it_t = bkrl::json_make_tag_parser_traits::iterator_t const;

        REQUIRE(static_cast<int>(expected_tags.size()) == expected_unique);
        std::sort(std::begin(expected_tags), std::end(expected_tags));

        auto const parser = bkrl::json_make_tag_parser(nullptr, [&](it_t beg, it_t end, it_t dup) {
            REQUIRE(std::distance(beg, end) == expected_unique);
            REQUIRE(std::distance(end, dup) == expected_duplicate);

            REQUIRE(std::equal(beg, end, std::begin(expected_tags)));

            return true;
        });

        REQUIRE(bklib::json_parse_string(*parser, json));
    };

    using tag_t = bkrl::tag_list::value_type;

    SECTION("no tags") {
        bklib::utf8_string_view const json {R"(
            []
        )"};

        do_test(json, 0, 0, bkrl::tag_list {});
    }

    SECTION("one tag") {
        bklib::utf8_string_view const json {R"(
            ["tag0"]
        )"};

        do_test(json, 1, 0, bkrl::tag_list {tag_t {"tag0"}});
    }

    SECTION("multiple tag") {
        bklib::utf8_string_view const json {R"(
            ["tag0", "tag1", "tag2"]
        )"};

        do_test(json, 3, 0, bkrl::tag_list {tag_t {"tag0"}, tag_t {"tag1"}, tag_t {"tag2"}});
    }

    SECTION("multiple duplicates") {
        bklib::utf8_string_view const json {R"(
            ["tag0", "tag0", "tag0", "tag1", "tag1", "tag1"]
        )"};

        do_test(json, 2, 4, bkrl::tag_list {tag_t {"tag0"}, tag_t {"tag1"}});
    }
}

TEST_CASE("json base definition parser", "[json][bkrl]") {
    bklib::utf8_string_view const json {R"(
        { "file_type": "test"
        , "definitions": [
            { "id": "id0"
            , "name": "name0"
            , "description": "desc0"
            , "symbol": "sym0"
            , "symbol_color": "color0"
            , "tags": ["tag00", "tag01"]
            }
          , { "id": "id1"
            , "name": "name1"
            , "description": "desc1"
            , "symbol": "sym1"
            , "symbol_color": "color1"
            , "tags": ["tag10", "tag11"]
            }
          ]
        }
    )"};

    bkrl::definition_base def {""};
    auto const parser = bkrl::json_make_base_def_parser(nullptr, def);

    auto const select_handler = [&](bklib::utf8_string_view const file_type) {
        REQUIRE(file_type == "test");
        return parser.get();
    };

    auto const check_values = [&](auto const id, auto const name, auto const desc, auto const sym, auto const sym_color) {
        REQUIRE(def.id_string    == id);
        REQUIRE(def.name         == name);
        REQUIRE(def.description  == desc);
        REQUIRE(def.symbol       == sym);
        REQUIRE(def.symbol_color == sym_color);
        REQUIRE(def.tags.size()  == 2);
    };

    int count = 0;

    bkrl::json_parse_definitions(json, select_handler, [&] {
        if (count == 0) {
            check_values("id0", "name0", "desc0", "sym0", "color0");
        } else if (count == 1) {
            check_values("id1", "name1", "desc1", "sym1", "color1");
        } else {
            FAIL();
        }

        ++count;

        return true;
    });

    REQUIRE(count == 2);
}

#endif // BK_NO_UNIT_TESTS
