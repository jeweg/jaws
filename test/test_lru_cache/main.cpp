#include "jaws/util/lru_cache.hpp"
#include "gtest/gtest.h"
#include <string>

using namespace jaws::util;
using namespace std::string_literals;

TEST(lru_cache_test, basic)
{
    LruCache<int, std::string> c(4, 0);

    EXPECT_EQ(c.size(), 0);
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.lookup(100), nullptr);

    c.clear();
    EXPECT_EQ(c.size(), 0);
    c.purge();
    EXPECT_EQ(c.size(), 0);
    c.purge(2, 100);
    EXPECT_EQ(c.size(), 0);

    c.insert(100, "100");
    c.insert(101, "101");
    c.insert(102, "102");
    EXPECT_EQ(c.size(), 3);
    EXPECT_FALSE(c.empty());

    c.purge(2, 100);
    EXPECT_EQ(c.size(), 2);
    EXPECT_EQ(c.lookup(100), nullptr);
    EXPECT_STREQ(c.lookup(101)->c_str(), "101");
    EXPECT_STREQ(c.lookup(102)->c_str(), "102");

    c.purge(1, 100);
    EXPECT_EQ(c.size(), 1);
    EXPECT_EQ(c.lookup(101), nullptr);

    c.purge(0, 100);
    EXPECT_EQ(c.size(), 0);
    EXPECT_EQ(c.lookup(102), nullptr);
    EXPECT_TRUE(c.empty());

    c.insert(103, "103");
    EXPECT_EQ(c.size(), 1);

    auto *ptr = c.insert(101, "xx");
    EXPECT_EQ(*ptr, "xx");

    c.clear();
    EXPECT_TRUE(c.empty());
}

TEST(lru_cache_test, automatic_purge)
{
    LruCache<int, std::string> c(4, 0);

    c.insert(100, "100");
    c.insert(101, "101");
    c.insert(102, "102");
    EXPECT_EQ(c.size(), 3);

    c.insert(103, "103");
    EXPECT_EQ(c.size(), 4);

    c.insert(104, "104");
    EXPECT_EQ(c.size(), 4);
}

TEST(lru_cache_test, manual_purge)
{
    LruCache<int, std::string> c{/* default arguments disable purge */};

    c.insert(100, "100");
    c.insert(101, "101");
    c.insert(102, "102");
    c.insert(103, "103");
    c.insert(104, "104");
    EXPECT_EQ(c.size(), 5);

    c.insert(102, "xxxx");
    EXPECT_EQ(c.size(), 5);

    c.insert(109, "109");
    EXPECT_EQ(c.size(), 6);

    EXPECT_EQ(*c.lookup(101), "101");
    EXPECT_EQ(*c.lookup(102), "xxxx");
}

TEST(lru_cache_test, aging)
{
    LruCache<int, std::string> c;

    // Just using the elem count as limiter is not great if
    // we end up using the same n elements for a long while
    // and max elem count > n.
    // In that case we will keep stale elements in the cache
    // for that long time even though they clearly haven't been
    // used.

    c.insert(100, "100");
    c.insert(101, "101");
    c.insert(102, "102");
    EXPECT_EQ(c.size(), 3);

    c.advance_clock();
    c.insert(103, "103");
    EXPECT_EQ(c.size(), 4);

    c.advance_clock();
    c.insert(104, "104");
    EXPECT_EQ(c.size(), 5);

    // Max age 1 --> only the elems touched since
    // the second-to-last clock tick.
    c.purge(-1, 1);
    EXPECT_EQ(c.size(), 2);

    // Max age 0 --> only the elems touched since
    // the last clock tick.
    c.purge(-1, 0);
    EXPECT_EQ(c.size(), 1);

    c.advance_clock();

    // Again, but nothing was touched since the last
    // clock tick -> purges all.
    c.purge(-1, 0);
    EXPECT_TRUE(c.empty());
}

TEST(lru_cache_test, movable_only)
{
    struct A
    {
        A(int) {}
        A(A &&) = default;
        A &operator=(A &&) = default;
        A(const A &) = delete;
        A &operator=(const A &) = delete;
    };

    static_assert(std::is_move_constructible_v<A>);
    static_assert(!std::is_copy_constructible_v<A>);

    LruCache<int, A> c;
    c.insert(0, A{123});

    A a{456};
    // c.insert(0, a); // fails to compile, can't copy.
    c.insert(0, std::move(a));
}


TEST(lru_cache_test, copyable_only)
{
    struct A
    {
        A(int) {}
        A(A &&) = delete;
        A &operator=(A &&) = delete;
        A(const A &) = default;
        A &operator=(const A &) = default;
    };

    static_assert(!std::is_move_constructible_v<A>);
    static_assert(std::is_copy_constructible_v<A>);

    LruCache<int, A> c;
    c.insert(0, A{123});

    A a{456};
    c.insert(0, a);

    // This does compile, b/c std::move doesn't actually move
    // and when it comes time to call a constructor, we dispatch
    // to copy or move c'tor based on whether Value implements those.
    c.insert(0, std::move(a));
}


TEST(lru_cache_test, emplace)
{
    struct A
    {
        A(int i, std::string s, double d) : i(i), s(std::move(s)), d(d) {}
        int i;
        std::string s;
        double d;
    };

    LruCache<int, A> c;
    A *a1 = c.emplace(3, 1, "foo", 3.0);
    A *a2 = c.emplace(2, 7, "bar", 17.0);
    A *a3 = c.emplace(2, 3, "baz", 99.0);
    A *a4 = c.emplace(6, 99, "bla", -13.0);

    EXPECT_EQ(c.size(), 3);
    EXPECT_EQ(a1->i, 1);
    EXPECT_EQ(a2->i, 3);
    EXPECT_EQ(a3->i, 3);
    EXPECT_EQ(a4->i, 99);
}
