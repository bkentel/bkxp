#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "color.hpp"

//TEST_CASE("color") {
//    using namespace bklib::literals;
//
//    bklib::utf8_string_view const json {R"(
//        { "file_type": "colors"
//        , "definitions": [
//            { "id": "white"
//            , "short_name": "w"
//            , "value": [255, 255, 255]
//            }
//          , { "id": "red"
//            , "short_name": "r"
//            , "value": [255, 0, 0]
//            }
//          , { "id": "grey"
//            , "short_name": "gy"
//            , "value": [127, 127, 127]
//            }
//          ]
//        }
//    )"};
//
//    bkrl::color_dictionary dic;
//    bkrl::load_definitions(dic, json, bkrl::load_from_string);
//
//    REQUIRE(dic.size() == 3);
//
//    using bklib::utf8_string_view;
//
//    auto const check = [&](
//        utf8_string_view const id
//      , utf8_string_view const short_name
//      , bkrl::color4 const c
//    ) {
//        auto const ptr = dic.find(bkrl::color_def_id {bklib::djb2_hash(id)});
//        REQUIRE(ptr);
//        REQUIRE(ptr->id == id);
//        REQUIRE(ptr->short_name == short_name);
//        REQUIRE(ptr->color[0] == c[0]);
//        REQUIRE(ptr->color[1] == c[1]);
//        REQUIRE(ptr->color[2] == c[2]);
//        REQUIRE(ptr->color[3] == c[3]);
//    };
//
//    check("white", "w",  bkrl::color4 {255, 255, 255, 0});
//    check("red",   "r",  bkrl::color4 {255, 0, 0, 0});
//    check("grey",  "gy", bkrl::color4 {127, 127, 127, 0});
//}

#endif // BK_NO_UNIT_TESTS
