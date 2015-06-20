#ifndef BK_NO_UNIT_TESTS
#include <catch/catch.hpp>

#include "bklib/string.hpp"
#include "bklib/utility.hpp"
#include "bklib/hash.hpp"
#include "bklib/dictionary.hpp"

namespace {

struct test_def {
    using id_type = bklib::tagged_value<int, test_def>;

    explicit test_def(bklib::utf8_string id_string)
      : id {bklib::hash_value(id_string)}
      , id_string {std::move(id_string)}
    {
    }

    id_type            id;
    bklib::utf8_string id_string;
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

    static test_def const def {"test0"};

    test_def def0 {"test0"};
    test_def def1 {"test0"};

    // insert into an empty dictionary
    auto const result0 = dic.insert_or_discard(std::move(def0));
    REQUIRE(result0.second == true);
    REQUIRE(!dic.empty());
    REQUIRE(dic.size() == 1);
    REQUIRE(def0.id_string.empty());
    REQUIRE(result0.first.id == def.id);
    REQUIRE(result0.first.id_string == def.id_string);

    // insert duplicate
    auto const result1 = dic.insert_or_discard(std::move(def1));
    REQUIRE(result1.second == false);
    REQUIRE(dic.size() == 1);
    REQUIRE(&result1.first == &def1);

    // find non-existing
    REQUIRE(!dic.find(test_def::id_type {0}));

    // find existing
    auto const ptr = dic.find(def.id);
    REQUIRE(ptr);
    REQUIRE(ptr->id == def.id);
    REQUIRE(ptr->id_string == def.id_string);
}

#endif // BK_NO_UNIT_TESTS
