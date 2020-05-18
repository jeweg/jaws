#include "jaws/jaws.hpp"
#include "jaws/util/enumerate_range.hpp"
#include "jaws/util/instance_counter.hpp"
#include "jaws/util/type_traits.hpp"
#include "gtest/gtest.h"
#include <iostream>

class Mock : public jaws::util::InstanceCounter<Mock, false, false>
{
    std::vector<int> _data = {888, 666, 444, 222};

public:
    auto begin() { return _data.begin(); }
    auto end() { return _data.end(); }
    [[nodiscard]] auto begin() const { return _data.begin(); }
    [[nodiscard]] auto end() const { return _data.end(); }
};

TEST(enumerate_range_test, const_range)
{
    const Mock const_mock;
    for (auto [i, elem] : jaws::util::enumerate_range(const_mock)) {
        EXPECT_STREQ("const T&", jaws::util::get_type_string_repr(elem));
        EXPECT_TRUE(i != 0 || elem == 888);
        EXPECT_TRUE(i != 1 || elem == 666);
        EXPECT_TRUE(i != 2 || elem == 444);
        EXPECT_TRUE(i != 3 || elem == 222);
    }
}

TEST(enumerate_range_test, nonconst_range)
{
    Mock mutable_mock;
    for (auto [i, elem] : jaws::util::enumerate_range(mutable_mock)) {
        EXPECT_STREQ("T&", jaws::util::get_type_string_repr(elem));
        elem += 1;
    }
    for (auto [i, elem] : jaws::util::enumerate_range(mutable_mock)) {
        EXPECT_TRUE(i != 0 || elem == 889);
        EXPECT_TRUE(i != 1 || elem == 667);
        EXPECT_TRUE(i != 2 || elem == 445);
        EXPECT_TRUE(i != 3 || elem == 223);
    }
}

TEST(enumerate_range_test, temporary_range)
{
    auto get_temp = []() { return std::move(Mock()); };
    for (auto [i, elem] : jaws::util::enumerate_range(get_temp())) {
        EXPECT_STREQ("T&", jaws::util::get_type_string_repr(elem));
        elem -= 1;
        EXPECT_TRUE(i != 0 || elem == 887);
        EXPECT_TRUE(i != 1 || elem == 665);
        EXPECT_TRUE(i != 2 || elem == 443);
        EXPECT_TRUE(i != 3 || elem == 221);
    }
}

TEST(enumerate_range_test, initializer_list)
{
    for (auto [i, elem] : jaws::util::enumerate_range({"foo", "bar", "baz"})) {
        EXPECT_STREQ("const T&", jaws::util::get_type_string_repr(elem));
        EXPECT_TRUE(i != 0 || std::string(elem) == "foo");
        EXPECT_TRUE(i != 1 || std::string(elem) == "bar");
        EXPECT_TRUE(i != 2 || std::string(elem) == "baz");
    }
}

TEST(enumerate_range_test, initializer_list_of_refs)
{
    float a = 111;
    float b = 222;
    float c = 333;
    int i = 0;
    for (auto elem : {std::ref(a), std::ref(b), std::ref(c)}) {
        elem += 1;
    }
    EXPECT_EQ(112, a);
    EXPECT_EQ(223, b);
    EXPECT_EQ(334, c);
    for (auto [i, elem] : jaws::util::enumerate_range({std::ref(a), std::ref(b), std::ref(c)})) {
        EXPECT_STREQ("const T&", jaws::util::get_type_string_repr(elem));
        EXPECT_TRUE(i != 0 || elem == 112);
        EXPECT_TRUE(i != 1 || elem == 223);
        EXPECT_TRUE(i != 2 || elem == 334);
    }
}

TEST(enumerate_range_test, no_structured_binding)
{
    // This is rather pointless without structured bindings.
    for (const auto& tup : jaws::util::enumerate_range({3, 1, 4, 1, 5, 9, 2, 6})) {
        int i, elem;
        std::tie(i, elem) = tup;

        EXPECT_TRUE(i != 0 || elem == 3);
        EXPECT_TRUE(i != 1 || elem == 1);
        EXPECT_TRUE(i != 2 || elem == 4);
        EXPECT_TRUE(i != 3 || elem == 1);
        EXPECT_TRUE(i != 4 || elem == 5);
        EXPECT_TRUE(i != 5 || elem == 9);
        EXPECT_TRUE(i != 6 || elem == 2);
        EXPECT_TRUE(i != 7 || elem == 6);
    }
}

TEST(enumerate_range_total_test, const_range)
{
    const Mock const_mock;
    for (auto [i, total, elem] : jaws::util::enumerate_range_total(const_mock)) {
        EXPECT_STREQ("const T&", jaws::util::get_type_string_repr(elem));
        EXPECT_EQ(total, 4);
        EXPECT_TRUE(i != 0 || elem == 888);
        EXPECT_TRUE(i != 1 || elem == 666);
        EXPECT_TRUE(i != 2 || elem == 444);
        EXPECT_TRUE(i != 3 || elem == 222);
    }
}

TEST(enumerate_range_total_test, nonconst_range)
{
    Mock mutable_mock;
    for (auto [i, total, elem] : jaws::util::enumerate_range_total(mutable_mock)) {
        EXPECT_EQ(total, 4);
        EXPECT_STREQ("T&", jaws::util::get_type_string_repr(elem));
        elem += 1;
    }
    for (auto [i, total, elem] : jaws::util::enumerate_range_total(mutable_mock)) {
        EXPECT_EQ(total, 4);
        EXPECT_TRUE(i != 0 || elem == 889);
        EXPECT_TRUE(i != 1 || elem == 667);
        EXPECT_TRUE(i != 2 || elem == 445);
        EXPECT_TRUE(i != 3 || elem == 223);
    }
}

TEST(enumerate_range_total_test, temporary_range)
{
    auto get_temp = []() { return std::move(Mock()); };
    for (auto [i, total, elem] : jaws::util::enumerate_range_total(get_temp())) {
        EXPECT_EQ(total, 4);
        EXPECT_STREQ("T&", jaws::util::get_type_string_repr(elem));
        elem -= 1;
        EXPECT_TRUE(i != 0 || elem == 887);
        EXPECT_TRUE(i != 1 || elem == 665);
        EXPECT_TRUE(i != 2 || elem == 443);
        EXPECT_TRUE(i != 3 || elem == 221);
    }
}

TEST(enumerate_range_total_test, initializer_list)
{
    for (auto [i, total, elem] : jaws::util::enumerate_range_total({"foo", "bar", "baz"})) {
        EXPECT_EQ(total, 3);
        EXPECT_STREQ("const T&", jaws::util::get_type_string_repr(elem));
        EXPECT_TRUE(i != 0 || std::string(elem) == "foo");
        EXPECT_TRUE(i != 1 || std::string(elem) == "bar");
        EXPECT_TRUE(i != 2 || std::string(elem) == "baz");
    }
}

/*
TEST(enumerate_range_total_test, initializer_list_of_refs)
{
#if 1
    float a = 111;
    float b = 222;
    float c = 333;
    for (auto [i, total, elem] : jaws::util::enumerate_range_total({std::ref(a), std::ref(b), std::ref(c)})) {
        EXPECT_EQ(total, 3);
        EXPECT_STREQ("const T&", jaws::util::get_type_string_repr(elem));
        EXPECT_TRUE(i != 0 || elem == 111);
        EXPECT_TRUE(i != 1 || elem == 222);
        EXPECT_TRUE(i != 2 || elem == 333);
        // std::reference_wrapper allows modification.
        // Test whether this sticks beyond the loop.
        elem += (i + 1.f) * 100.f;
    }
    EXPECT_EQ(111 + 1.f * 100.f, a);
    EXPECT_EQ(222 + 2.f * 100.f, b);
    EXPECT_EQ(333 + 3.f * 100.f, c);
#endif
}
 */

TEST(enumerate_range_total_test, no_structured_binding)
{
    // This is rather pointless without structured bindings.
    for (const auto& tup : jaws::util::enumerate_range_total({3, 1, 4, 1, 5, 9, 2, 6})) {
        int i, total, elem;
        std::tie(i, total, elem) = tup;

        EXPECT_EQ(total, 8);
        EXPECT_TRUE(i != 0 || elem == 3);
        EXPECT_TRUE(i != 1 || elem == 1);
        EXPECT_TRUE(i != 2 || elem == 4);
        EXPECT_TRUE(i != 3 || elem == 1);
        EXPECT_TRUE(i != 4 || elem == 5);
        EXPECT_TRUE(i != 5 || elem == 9);
        EXPECT_TRUE(i != 6 || elem == 2);
        EXPECT_TRUE(i != 7 || elem == 6);
    }
}
