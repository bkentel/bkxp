#ifndef BK_NO_UNIT_TESTS
#include <boost/predef.h>
#if BOOST_COMP_CLANG
#   pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#include <catch/catch.hpp>

#include "bklib/string.hpp"
#include "bklib/utility.hpp"
#include "bklib/hash.hpp"
#include "bklib/dictionary.hpp"

namespace {

struct test_def {
    using id_type = bklib::tagged_value<test_def, size_t>;

    test_def(bklib::utf8_string def_id_string, int const def_data)
      : id {bklib::hash_value(def_id_string)}
      , id_string {std::move(def_id_string)}
      , data {def_data}
    {
    }

    id_type            id;
    bklib::utf8_string id_string;
    int                data;
};

template <typename T, typename Tag>
inline decltype(auto) get_id(bklib::tagged_value<T, Tag> const& id) noexcept {
    return id;
}

inline decltype(auto) get_id(test_def const& def) noexcept {
    return def.id;
}

} //namespace

TEST_CASE("new test case") {
    bklib::dictionary<test_def> dic;
    REQUIRE(dic.empty());
    REQUIRE(dic.size() == 0);

    auto const expect_result = [](bool const ok, bklib::utf8_string_view const id, int const data, auto const& result) {
        REQUIRE(result.second == ok);
        REQUIRE(result.first->id_string == id);
        REQUIRE(static_cast<size_t>(result.first->id) == bklib::djb2_hash(id));
        REQUIRE(result.first->data == data);
    };

    using insert_type0 = decltype(dic.insert_or_discard(std::declval<test_def>()));
    using insert_type1 = decltype(dic.insert_or_replace(std::declval<test_def>()));

    static_assert(std::is_same<insert_type0, insert_type1>::value, "");
    static_assert(std::is_same<insert_type0::first_type, test_def const*>::value, "");
    static_assert(std::is_same<insert_type0::second_type, bool>::value, "");

    // insert into an empty dictionary
    expect_result(true, "test0", 0, dic.insert_or_discard(test_def {"test0", 0}));
    REQUIRE(!dic.empty());
    REQUIRE(dic.size() == 1);

    // insert duplicate - discard
    expect_result(false, "test0", 0, dic.insert_or_discard(test_def {"test0", 1}));
    REQUIRE(dic.size() == 1);

    expect_result(false, "test0", 1, dic.insert_or_replace(test_def {"test0", 1}));
    REQUIRE(dic.size() == 1);

    // find non-existing by id
    REQUIRE(!dic.find(test_def::id_type {0}));
    // find non-existing by def
    REQUIRE(!dic.find(test_def {"test1", 0}));

    // find existing - by def
    auto const ptr0 = dic.find(test_def {"test0", 0});
    REQUIRE(ptr0);

    // find existing - by id
    auto const ptr1 = dic.find(test_def {"test0", 0}.id);
    REQUIRE(ptr1);

    REQUIRE(ptr0 == ptr1);
}

#endif // BK_NO_UNIT_TESTS
