#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "bklib/string.hpp"
#include "bklib/algorithm.hpp"

TEST_CASE("ordinal_suffix", "[bklib][string]") {
    REQUIRE(bklib::oridinal_suffix(-1) == "th");
    REQUIRE(bklib::oridinal_suffix(0)  == "th");
    REQUIRE(bklib::oridinal_suffix(1)  == "st");
    REQUIRE(bklib::oridinal_suffix(2)  == "nd");
    REQUIRE(bklib::oridinal_suffix(3)  == "rd");
    REQUIRE(bklib::oridinal_suffix(4)  == "th");
    REQUIRE(bklib::oridinal_suffix(5)  == "th");
    REQUIRE(bklib::oridinal_suffix(6)  == "th");
    REQUIRE(bklib::oridinal_suffix(7)  == "th");
    REQUIRE(bklib::oridinal_suffix(8)  == "th");
    REQUIRE(bklib::oridinal_suffix(9)  == "th");
    REQUIRE(bklib::oridinal_suffix(10) == "th");
    REQUIRE(bklib::oridinal_suffix(11) == "th");
    REQUIRE(bklib::oridinal_suffix(12) == "th");
    REQUIRE(bklib::oridinal_suffix(13) == "th");
    REQUIRE(bklib::oridinal_suffix(14) == "th");
    REQUIRE(bklib::oridinal_suffix(15) == "th");
    REQUIRE(bklib::oridinal_suffix(16) == "th");
    REQUIRE(bklib::oridinal_suffix(17) == "th");
    REQUIRE(bklib::oridinal_suffix(18) == "th");
    REQUIRE(bklib::oridinal_suffix(19) == "th");
    REQUIRE(bklib::oridinal_suffix(20) == "th");
    REQUIRE(bklib::oridinal_suffix(21) == "st");
    REQUIRE(bklib::oridinal_suffix(22) == "nd");
    REQUIRE(bklib::oridinal_suffix(23) == "rd");

    REQUIRE(bklib::oridinal_suffix(101) == "st");
    REQUIRE(bklib::oridinal_suffix(102) == "nd");
    REQUIRE(bklib::oridinal_suffix(103) == "rd");

    REQUIRE(bklib::oridinal_suffix(111) == "th");
    REQUIRE(bklib::oridinal_suffix(112) == "th");
    REQUIRE(bklib::oridinal_suffix(113) == "th");
}

TEST_CASE("alphanum_id", "[bklib][string]") {
    REQUIRE(bklib::alphanum_id::to_char(-1)  == -1);
    REQUIRE(bklib::alphanum_id::to_char(62)  == -1);

    REQUIRE(bklib::alphanum_id::to_char(0)  == 'a');
    REQUIRE(bklib::alphanum_id::to_char(1)  == 'b');
    REQUIRE(bklib::alphanum_id::to_char(2)  == 'c');
    REQUIRE(bklib::alphanum_id::to_char(3)  == 'd');
    REQUIRE(bklib::alphanum_id::to_char(4)  == 'e');
    REQUIRE(bklib::alphanum_id::to_char(5)  == 'f');
    REQUIRE(bklib::alphanum_id::to_char(6)  == 'g');
    REQUIRE(bklib::alphanum_id::to_char(7)  == 'h');
    REQUIRE(bklib::alphanum_id::to_char(8)  == 'i');
    REQUIRE(bklib::alphanum_id::to_char(9)  == 'j');
    REQUIRE(bklib::alphanum_id::to_char(10) == 'k');
    REQUIRE(bklib::alphanum_id::to_char(11) == 'l');
    REQUIRE(bklib::alphanum_id::to_char(12) == 'm');
    REQUIRE(bklib::alphanum_id::to_char(13) == 'n');
    REQUIRE(bklib::alphanum_id::to_char(14) == 'o');
    REQUIRE(bklib::alphanum_id::to_char(15) == 'p');
    REQUIRE(bklib::alphanum_id::to_char(16) == 'q');
    REQUIRE(bklib::alphanum_id::to_char(17) == 'r');
    REQUIRE(bklib::alphanum_id::to_char(18) == 's');
    REQUIRE(bklib::alphanum_id::to_char(19) == 't');
    REQUIRE(bklib::alphanum_id::to_char(20) == 'u');
    REQUIRE(bklib::alphanum_id::to_char(21) == 'v');
    REQUIRE(bklib::alphanum_id::to_char(22) == 'w');
    REQUIRE(bklib::alphanum_id::to_char(23) == 'x');
    REQUIRE(bklib::alphanum_id::to_char(24) == 'y');
    REQUIRE(bklib::alphanum_id::to_char(25) == 'z');
    REQUIRE(bklib::alphanum_id::to_char(26) == 'A');
    REQUIRE(bklib::alphanum_id::to_char(27) == 'B');
    REQUIRE(bklib::alphanum_id::to_char(28) == 'C');
    REQUIRE(bklib::alphanum_id::to_char(29) == 'D');
    REQUIRE(bklib::alphanum_id::to_char(30) == 'E');
    REQUIRE(bklib::alphanum_id::to_char(31) == 'F');
    REQUIRE(bklib::alphanum_id::to_char(32) == 'G');
    REQUIRE(bklib::alphanum_id::to_char(33) == 'H');
    REQUIRE(bklib::alphanum_id::to_char(34) == 'I');
    REQUIRE(bklib::alphanum_id::to_char(35) == 'J');
    REQUIRE(bklib::alphanum_id::to_char(36) == 'K');
    REQUIRE(bklib::alphanum_id::to_char(37) == 'L');
    REQUIRE(bklib::alphanum_id::to_char(38) == 'M');
    REQUIRE(bklib::alphanum_id::to_char(39) == 'N');
    REQUIRE(bklib::alphanum_id::to_char(40) == 'O');
    REQUIRE(bklib::alphanum_id::to_char(41) == 'P');
    REQUIRE(bklib::alphanum_id::to_char(42) == 'Q');
    REQUIRE(bklib::alphanum_id::to_char(43) == 'R');
    REQUIRE(bklib::alphanum_id::to_char(44) == 'S');
    REQUIRE(bklib::alphanum_id::to_char(45) == 'T');
    REQUIRE(bklib::alphanum_id::to_char(46) == 'U');
    REQUIRE(bklib::alphanum_id::to_char(47) == 'V');
    REQUIRE(bklib::alphanum_id::to_char(48) == 'W');
    REQUIRE(bklib::alphanum_id::to_char(49) == 'X');
    REQUIRE(bklib::alphanum_id::to_char(50) == 'Y');
    REQUIRE(bklib::alphanum_id::to_char(51) == 'Z');
    REQUIRE(bklib::alphanum_id::to_char(52) == '0');
    REQUIRE(bklib::alphanum_id::to_char(53) == '1');
    REQUIRE(bklib::alphanum_id::to_char(54) == '2');
    REQUIRE(bklib::alphanum_id::to_char(55) == '3');
    REQUIRE(bklib::alphanum_id::to_char(56) == '4');
    REQUIRE(bklib::alphanum_id::to_char(57) == '5');
    REQUIRE(bklib::alphanum_id::to_char(58) == '6');
    REQUIRE(bklib::alphanum_id::to_char(59) == '7');
    REQUIRE(bklib::alphanum_id::to_char(60) == '8');
    REQUIRE(bklib::alphanum_id::to_char(61) == '9');

    REQUIRE(bklib::alphanum_id::to_index(-1)   == -1);
    REQUIRE(bklib::alphanum_id::to_index('.')  == -1);
    REQUIRE(bklib::alphanum_id::to_index(1000) == -1);

    REQUIRE(0  == bklib::alphanum_id::to_index('a'));
    REQUIRE(1  == bklib::alphanum_id::to_index('b'));
    REQUIRE(2  == bklib::alphanum_id::to_index('c'));
    REQUIRE(3  == bklib::alphanum_id::to_index('d'));
    REQUIRE(4  == bklib::alphanum_id::to_index('e'));
    REQUIRE(5  == bklib::alphanum_id::to_index('f'));
    REQUIRE(6  == bklib::alphanum_id::to_index('g'));
    REQUIRE(7  == bklib::alphanum_id::to_index('h'));
    REQUIRE(8  == bklib::alphanum_id::to_index('i'));
    REQUIRE(9  == bklib::alphanum_id::to_index('j'));
    REQUIRE(10 == bklib::alphanum_id::to_index('k'));
    REQUIRE(11 == bklib::alphanum_id::to_index('l'));
    REQUIRE(12 == bklib::alphanum_id::to_index('m'));
    REQUIRE(13 == bklib::alphanum_id::to_index('n'));
    REQUIRE(14 == bklib::alphanum_id::to_index('o'));
    REQUIRE(15 == bklib::alphanum_id::to_index('p'));
    REQUIRE(16 == bklib::alphanum_id::to_index('q'));
    REQUIRE(17 == bklib::alphanum_id::to_index('r'));
    REQUIRE(18 == bklib::alphanum_id::to_index('s'));
    REQUIRE(19 == bklib::alphanum_id::to_index('t'));
    REQUIRE(20 == bklib::alphanum_id::to_index('u'));
    REQUIRE(21 == bklib::alphanum_id::to_index('v'));
    REQUIRE(22 == bklib::alphanum_id::to_index('w'));
    REQUIRE(23 == bklib::alphanum_id::to_index('x'));
    REQUIRE(24 == bklib::alphanum_id::to_index('y'));
    REQUIRE(25 == bklib::alphanum_id::to_index('z'));
    REQUIRE(26 == bklib::alphanum_id::to_index('A'));
    REQUIRE(27 == bklib::alphanum_id::to_index('B'));
    REQUIRE(28 == bklib::alphanum_id::to_index('C'));
    REQUIRE(29 == bklib::alphanum_id::to_index('D'));
    REQUIRE(30 == bklib::alphanum_id::to_index('E'));
    REQUIRE(31 == bklib::alphanum_id::to_index('F'));
    REQUIRE(32 == bklib::alphanum_id::to_index('G'));
    REQUIRE(33 == bklib::alphanum_id::to_index('H'));
    REQUIRE(34 == bklib::alphanum_id::to_index('I'));
    REQUIRE(35 == bklib::alphanum_id::to_index('J'));
    REQUIRE(36 == bklib::alphanum_id::to_index('K'));
    REQUIRE(37 == bklib::alphanum_id::to_index('L'));
    REQUIRE(38 == bklib::alphanum_id::to_index('M'));
    REQUIRE(39 == bklib::alphanum_id::to_index('N'));
    REQUIRE(40 == bklib::alphanum_id::to_index('O'));
    REQUIRE(41 == bklib::alphanum_id::to_index('P'));
    REQUIRE(42 == bklib::alphanum_id::to_index('Q'));
    REQUIRE(43 == bklib::alphanum_id::to_index('R'));
    REQUIRE(44 == bklib::alphanum_id::to_index('S'));
    REQUIRE(45 == bklib::alphanum_id::to_index('T'));
    REQUIRE(46 == bklib::alphanum_id::to_index('U'));
    REQUIRE(47 == bklib::alphanum_id::to_index('V'));
    REQUIRE(48 == bklib::alphanum_id::to_index('W'));
    REQUIRE(49 == bklib::alphanum_id::to_index('X'));
    REQUIRE(50 == bklib::alphanum_id::to_index('Y'));
    REQUIRE(51 == bklib::alphanum_id::to_index('Z'));
    REQUIRE(52 == bklib::alphanum_id::to_index('0'));
    REQUIRE(53 == bklib::alphanum_id::to_index('1'));
    REQUIRE(54 == bklib::alphanum_id::to_index('2'));
    REQUIRE(55 == bklib::alphanum_id::to_index('3'));
    REQUIRE(56 == bklib::alphanum_id::to_index('4'));
    REQUIRE(57 == bklib::alphanum_id::to_index('5'));
    REQUIRE(58 == bklib::alphanum_id::to_index('6'));
    REQUIRE(59 == bklib::alphanum_id::to_index('7'));
    REQUIRE(60 == bklib::alphanum_id::to_index('8'));
    REQUIRE(61 == bklib::alphanum_id::to_index('9'));
}

//TEST_CASE("string_id", "[bklib][string]") {
//    using test_id_t = bklib::string_id<struct tag_test>;
//
//    SECTION("default constructed string hashes") {
//        test_id_t id;
//
//        REQUIRE(id.hash == 0);
//        auto const result = bklib::all_of(id.hash_string, [](auto const& e) { return e == 0; });
//        REQUIRE(result);
//    }
//
//    SECTION("maximum size") {
//        constexpr char const string[] = "0123456789A";
//        constexpr auto const hash = bklib::static_djb2_hash(string);
//
//        test_id_t const id {string};
//
//        REQUIRE(id.hash == hash);
//        REQUIRE(id.hash_string.back() == 0);
//
//        auto const result = std::equal(begin(id.hash_string), end(id.hash_string), std::begin(string));
//        REQUIRE(result);
//    }
//
//    SECTION("overlong string hashes") {
//        constexpr char const string[] = "this is too long";
//        constexpr auto const hash = bklib::static_djb2_hash(string);
//
//        test_id_t const id {string};
//
//        REQUIRE(id.hash == hash);
//        REQUIRE(id.hash_string.back() == 0);
//
//        auto const result = std::mismatch(
//            begin(id.hash_string), end(id.hash_string)
//          , std::begin(string), std::end(string));
//
//        auto const a = std::distance(begin(id.hash_string), result.first);
//        auto const b = std::distance(std::begin(string), result.second);
//
//        REQUIRE(a == b);
//        REQUIRE(a == sizeof(id.hash_string) - 1);
//    }
//}

#endif // BK_NO_UNIT_TESTS
