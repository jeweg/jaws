#include "jaws/util/pool.hpp"
#include "gtest/gtest.h"
#include <array>


#include <iostream>

TEST(pool_test, empty)
{
    jaws::util::Pool<int64_t> p;
    EXPECT_EQ(p.size(), 0);
    EXPECT_TRUE(p.empty());
}


TEST(pool_test, basic)
{
    jaws::util::Pool<char> p;
    auto *a = p.emplace('A');
    auto *b = p.emplace('B');
    auto *c = p.emplace('C');

    EXPECT_EQ(*c, 'C');
    EXPECT_EQ(*b, 'B');
    EXPECT_EQ(*a, 'A');

    p.free(c);
    EXPECT_EQ(*b, 'B');
    EXPECT_EQ(*a, 'A');
}


TEST(pool_test, clear)
{
    jaws::util::Pool<char> p;
    auto a = p.emplace('A');
    auto b = p.emplace('B');
    auto c = p.emplace('C');

    EXPECT_EQ(p.size(), 3);
    EXPECT_FALSE(p.empty());

    auto d = p.emplace('D');
    EXPECT_EQ(p.size(), 4);
    EXPECT_FALSE(p.empty());

    // Pool elements must be cleaned up!
    p.free(d);
    p.free(b);
    p.free(a);
    p.free(c);

    p.clear();
    EXPECT_EQ(p.size(), 0);
    EXPECT_TRUE(p.empty());
}


TEST(pool_test, no_moves)
{
    struct NoMoves
    {
        int value;

        NoMoves(int value) noexcept : value(value) {}
        NoMoves(const NoMoves &) = default;
        NoMoves &operator=(const NoMoves &) = default;
        NoMoves(NoMoves &&) = delete;
        NoMoves &operator=(NoMoves &&) = delete;
    };

    jaws::util::Pool<NoMoves> p;

    auto *i33 = p.insert(NoMoves(33));

    NoMoves m(77);
    // auto i77 = p.insert(std::move(m));
    auto *i77 = p.emplace(m);

    EXPECT_EQ(i33->value, 33);
    EXPECT_EQ(i77->value, 77);
}


TEST(pool_test, move_only)
{
    struct MoveOnly
    {
        int value;

        MoveOnly(int value) : value(value) {}
        MoveOnly(const MoveOnly &) = delete;
        MoveOnly &operator=(const MoveOnly &) = delete;
        MoveOnly(MoveOnly &&) = default;
        MoveOnly &operator=(MoveOnly &&) = default;
    };
    static_assert(std::is_move_constructible_v<MoveOnly>, "");

    jaws::util::Pool<MoveOnly> p;

    auto *i33 = p.emplace(MoveOnly(33));

    MoveOnly m(77);
    auto *i77 = p.emplace(std::move(m));

    EXPECT_EQ(i33->value, 33);
    EXPECT_EQ(i77->value, 77);
}


TEST(pool_test, dynamic_growth)
{
    struct Foo
    {
        std::array<uint8_t, 100> data;
    };

    jaws::util::Pool<Foo> p;

    auto a = p.emplace();
    EXPECT_EQ(p.size(), 1);

    auto b = p.emplace();
    EXPECT_EQ(p.size(), 2);

    auto c = p.emplace();
    EXPECT_EQ(p.size(), 3);

    auto d = p.emplace();
    EXPECT_EQ(p.size(), 4);

    auto e = p.emplace();
    EXPECT_EQ(p.size(), 5);
}
