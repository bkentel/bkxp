#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>
#include "bklib/utility.hpp"
#include "bklib/exception.hpp"
#include "bklib/scope_guard.hpp"
#include "bklib/assert.hpp"
#include <fstream>
#include <algorithm>

TEST_CASE("hash_id", "[bklib][utility]") {
    struct tag;
    using id_t = bklib::hash_id<tag>;

    static_assert(bklib::hash_id_base<0>::extra == 0, "");
    static_assert(bklib::hash_id_base<1>::extra == 4, "");
    static_assert(bklib::hash_id_base<2>::extra == 4, "");
    static_assert(bklib::hash_id_base<3>::extra == 4, "");
    static_assert(bklib::hash_id_base<4>::extra == 4, "");
    static_assert(bklib::hash_id_base<5>::extra == 4 + sizeof(size_t), "");

    SECTION("default construct") {
        id_t id;

        for (int i = 0; i < 1; ++i) {
            REQUIRE(id.c_str());
            REQUIRE(!id.c_str()[0]);
            REQUIRE(static_cast<uint32_t>(id) == 0);
            REQUIRE(!id);
            id.reset();
        }
    }

    SECTION("construct from value") {
        constexpr auto const value = 100u;

        id_t id {value};

        for (int i = 0; i < 1; ++i) {
            REQUIRE(id.c_str());
            REQUIRE(!id.c_str()[0]);
            REQUIRE(static_cast<uint32_t>(id) == value);
            REQUIRE(!!id);
            id.reset(value);
        }
    }

    SECTION("construct from string") {
        constexpr auto const str  = bklib::make_string_view("test string");
        constexpr auto const hash = bklib::static_djb2_hash(str);

        id_t id {str};

        for (int i = 0; i < 1; ++i) {
            REQUIRE(str == id.c_str());
            REQUIRE(static_cast<uint32_t>(id) == hash);
            REQUIRE(!!id);
            id.reset(str);
        }
    }

    SECTION("construct from string + len") {
        constexpr auto const str  = bklib::make_string_view("test string");
        constexpr auto const hash = bklib::static_djb2_hash(str);

        id_t id {str.data(), str.size()};

        for (int i = 0; i < 1; ++i) {
            REQUIRE(str == id.c_str());
            REQUIRE(static_cast<uint32_t>(id) == hash);
            REQUIRE(!!id);
            id.reset(str.data(), str.size());
        }
    }

    SECTION("construct from oversize id") {
        constexpr auto const str  = bklib::make_string_view("long test string");
        constexpr auto const hash = bklib::static_djb2_hash(str);

        bklib::hash_id<tag, uint32_t, 24> const id_other {str};

        id_t const id = id_other;
        REQUIRE(str.substr(0, 11) == id.c_str());
        REQUIRE(static_cast<uint32_t>(id) == hash);
        REQUIRE(!!id);
    }

    SECTION("construct from undersize id") {
        constexpr auto const str  = bklib::make_string_view("test");
        constexpr auto const hash = bklib::static_djb2_hash(str);

        bklib::hash_id<tag, uint32_t, 5> const id_other {str};

        id_t const id = id_other;
        REQUIRE(str.substr(0, 5) == id.c_str());
        REQUIRE(static_cast<uint32_t>(id) == hash);
        REQUIRE(!!id);
    }

    SECTION("comparisons") {
        constexpr auto const str = bklib::make_string_view("test string");

        REQUIRE(id_t {str} == id_t {str});
        REQUIRE(id_t {str} == str);
        REQUIRE(str == id_t {str});

        REQUIRE_FALSE(id_t {str} != id_t {str});
        REQUIRE_FALSE(id_t {str} != str);
        REQUIRE_FALSE(str != id_t {str});

        using id2_t = bklib::hash_id<tag, uint32_t, 0>;

        REQUIRE(id_t {str} == id2_t {str});
        REQUIRE(id2_t {str} == id_t {str});

        REQUIRE_FALSE(id_t {str} != id2_t {str});
        REQUIRE_FALSE(id2_t {str} != id_t {str});
    }
}

TEST_CASE("tagged_value", "[bklib][utility]") {
    using type = int;
    using tagged_t = bklib::tagged_value<struct tag, type>;

    REQUIRE(static_cast<type>(tagged_t {}) == type {});
    REQUIRE(static_cast<type>(tagged_t {type {1}}) == type {1});

    REQUIRE(!tagged_t {});
    REQUIRE(!!tagged_t {1});

    constexpr tagged_t const a {1};
    constexpr tagged_t const b {2};

    REQUIRE(a < b);
    REQUIRE(!(b < a));

    REQUIRE(a == a);
    REQUIRE(b == b);

    REQUIRE(a != b);
    REQUIRE(!(a == b));
}

TEST_CASE("pseudo_cast", "[bklib][utility]") {
    struct test_t {
        uint32_t a;
        uint16_t b;
        uint8_t  c;
        uint8_t  d;
    };

    static_assert(sizeof(test_t) == sizeof(uint64_t), "");

    test_t const from {
        uint32_t {0xABCDEF01}
      , uint16_t {0x1234}
      , uint8_t  {0x56}
      , uint8_t  {0x78}
    };

    auto const to = bklib::pseudo_cast(from, uint64_t {});

    switch (bklib::get_endian_type()) {
    case bklib::endian_type::little:  REQUIRE(to == 0x78561234abcdef01); break;
    case bklib::endian_type::big:     REQUIRE(to == 0x01efcdab34125678); break;
    case bklib::endian_type::unknown: BK_FALLTHROUGH
    default:
        REQUIRE(false); break;
    }

    auto const reverse_to = bklib::pseudo_cast(to, test_t {});
    REQUIRE(std::memcmp(&from, &reverse_to, sizeof(test_t)) == 0);
}

TEST_CASE("to_array", "[bklib][utility]") {
    constexpr auto const len = ptrdiff_t {5};

    auto const mismatch_distance = [](auto const& str, auto const& arr) {
        using std::begin;
        using std::end;

        auto const beg0 = begin(str);
        auto const beg1 = begin(arr);

        auto const p = std::mismatch(beg0, end(str), beg1, end(arr));

        return std::make_pair(
            std::distance(beg0, p.first)
          , std::distance(beg1, p.second));
    };

    SECTION("equal length case") {
        constexpr auto const str = bklib::make_string_view("test");
        static_assert(str.size() == len - 1, "");

        auto const arr = bklib::to_array<len>(str);
        REQUIRE(arr.size() == len);

        REQUIRE(mismatch_distance(str, arr) == std::make_pair(len - 1, len - 1));
        REQUIRE(arr[len - 1] == 0);
    }

    SECTION("truncate case") {
        constexpr auto const str = bklib::make_string_view("long test");
        static_assert(str.size() > len, "");

        auto const arr = bklib::to_array<len>(str);
        REQUIRE(arr.size() == len);

        REQUIRE(mismatch_distance(str, arr) == std::make_pair(len - 1, len - 1));
        REQUIRE(arr[len - 1] == 0);
    }

    SECTION("short case") {
        constexpr auto const str = bklib::make_string_view("abc");
        constexpr auto const short_len = static_cast<ptrdiff_t>(str.size());
        static_assert(str.size() < len, "");

        auto const arr = bklib::to_array<len>(str);
        REQUIRE(arr.size() == len);

        REQUIRE(mismatch_distance(str, arr) == std::make_pair(short_len, short_len));
        REQUIRE(arr[short_len] == 0);
    }

    SECTION("empty string case") {
        REQUIRE(bklib::to_array<len>("")[0] == 0);
    }

    SECTION("size 0") {
        REQUIRE(bklib::to_array<0>("test").size() == 0);
    }

    SECTION("size 1") {
        REQUIRE(bklib::to_array<1>("test")[0] == 0);
    }
}

TEST_CASE("read file", "[bklib][utility]") {
    REQUIRE_THROWS_AS(bklib::read_file_to_buffer("foo"), bklib::io_error const&);

    constexpr auto const file_name   = "test.txt";
    constexpr auto const test_string = bklib::make_string_view("test string");

    BK_NAMED_SCOPE_EXIT(guard) {
        std::remove(file_name);
    };

    std::ofstream {file_name, std::ios_base::out | std::ios_base::trunc} << test_string << '\0';
    auto const buffer = bklib::read_file_to_buffer(file_name);
    REQUIRE(!std::remove(file_name));
    REQUIRE(buffer.data() == test_string);

    guard.dismiss();
}

#endif // BK_NO_UNIT_TESTS
