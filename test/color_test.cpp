#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "color.hpp"

TEST_CASE("color") {
    using namespace bklib::literals;

    bklib::utf8_string_view const json {R"(
        { "file_type": "colors"
        , "definitions": [
            { "id": "white"
            , "short_name": "w"
            , "value": [255, 255, 255]
            }
          , { "id": "red"
            , "short_name": "r"
            , "value": [255, 0, 0]
            }
          , { "id": "grey"
            , "short_name": "gy"
            , "value": [127, 127, 127]
            }
          ]
        }
    )"};

    bkrl::color_dictionary dic;
    bkrl::load_definitions(dic, json, bkrl::load_from_string);

    REQUIRE(dic.size() == 3);

    auto const white = dic.find(bkrl::color_def_id {"white"_hash});
    REQUIRE(white);
    REQUIRE(white->id_string  == "white");
    REQUIRE(white->short_name == "w");
    REQUIRE(white->color[0] == 255);
    REQUIRE(white->color[1] == 255);
    REQUIRE(white->color[2] == 255);
    REQUIRE(white->color[3] == 0);
}

#endif // BK_NO_UNIT_TESTS
