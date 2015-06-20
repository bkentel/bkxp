#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "json.hpp"
#include "creature.hpp"

TEST_CASE("json definition parser", "[json][bkrl]") {
    bklib::utf8_string_view const json {R"(
        { "file_type": "test"
        , "definitions": []
        }
    )"};

    auto const select_handler = [&](bklib::utf8_string_view const file_type) {
        REQUIRE(file_type == "test");
        return static_cast<bklib::json_parser_base*>(nullptr);
    };

    int parsed_count = 0;
    bkrl::json_parse_definitions(json, select_handler, [&] {
        ++parsed_count;
        return true;
    });

    REQUIRE(parsed_count == 0);
}

// TODO move this elsewhere
TEST_CASE("json creature definition parser", "[creature][json][bkrl]") {
    bklib::utf8_string_view const json {R"(
        { "file_type": "creatures"
        , "definitions": [
            { "id": "id0"
            , "name": "name0"
            , "description": "desc0"
            , "symbol": "sym0"
            , "symbol_color": "color0"
            }
          , { "id": "id1"
            , "name": "name1"
            , "description": "desc1"
            , "symbol": "sym1"
            , "symbol_color": "color1"
            }
          ]
        }
    )"};

    bkrl::creature_dictionary creatures;
    bkrl::load_definitions(creatures, json, bkrl::load_from_string);

    REQUIRE(creatures.size() == 2);

    auto const check_values = [&](auto const id, auto const name, auto const desc, auto const sym, auto const sym_color) {
        auto const hash = bklib::djb2_hash(id);
        auto const ptr = creatures.find(bkrl::creature_def_id {hash});

        REQUIRE(ptr);
        REQUIRE(ptr->id.value == hash);
        REQUIRE(ptr->id_string == id);
        REQUIRE(ptr->name == name);
        REQUIRE(ptr->description == desc);
        REQUIRE(ptr->symbol == sym);
        REQUIRE(ptr->symbol_color == sym_color);
    };

    check_values("id0", "name0", "desc0", "sym0", "color0");
    check_values("id1", "name1", "desc1", "sym1", "color1");
}

#endif // BK_NO_UNIT_TESTS
