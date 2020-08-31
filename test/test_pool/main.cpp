#include "jaws/util/pool.hpp"
#include "gtest/gtest.h"
#include <array>


TEST(pool_test, empty)
{
    jaws::util::Pool<int64_t> p(0, 0);
    EXPECT_EQ(p.get_element_count(), 0);
    EXPECT_TRUE(p.empty());
    EXPECT_TRUE(p.lookup(1) == nullptr);
    EXPECT_TRUE(p.lookup(2) == nullptr);
    EXPECT_TRUE(p.lookup(3) == nullptr);
}


TEST(pool_test, basic)
{
    jaws::util::Pool<char> p(0, 10);
    auto a = p.emplace('A');
    auto b = p.insert('B');
    auto c = p.emplace('C');

    EXPECT_TRUE(*p.lookup(c) == 'C');
    EXPECT_TRUE(*p.lookup(b) == 'B');
    EXPECT_TRUE(*p.lookup(a) == 'A');

    p.remove(c);
    EXPECT_TRUE(p.lookup(a));
    EXPECT_TRUE(p.lookup(b));
    EXPECT_TRUE(p.lookup(c) == nullptr);

    p.remove(a);
    EXPECT_TRUE(p.lookup(a) == nullptr);
    EXPECT_TRUE(p.lookup(b));
    EXPECT_TRUE(p.lookup(c) == nullptr);

    p.remove(b);
    EXPECT_TRUE(p.lookup(a) == nullptr);
    EXPECT_TRUE(p.lookup(b) == nullptr);
    EXPECT_TRUE(p.lookup(c) == nullptr);
}


TEST(pool_test, no_moves)
{
    struct NoMoves
    {
        int value;

        NoMoves(int value) : value(value) {}
        NoMoves(const NoMoves &) = default;
        NoMoves &operator=(const NoMoves &) = default;
        NoMoves(NoMoves &&) = delete;
        NoMoves &operator=(NoMoves &&) = delete;
    };

    jaws::util::Pool<NoMoves> p(0, 10);

    auto i33 = p.insert(NoMoves(33));

    NoMoves m(77);
    // auto i77 = p.insert(std::move(m));
    auto i77 = p.insert(m);

    EXPECT_EQ(p.lookup(i33)->value, 33);
    EXPECT_EQ(p.lookup(i77)->value, 77);
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

    jaws::util::Pool<MoveOnly> p(0, 10);

    auto i33 = p.insert(MoveOnly(33));

    MoveOnly m(77);
    auto i77 = p.insert(std::move(m));

    EXPECT_EQ(p.lookup(i33)->value, 33);
    EXPECT_EQ(p.lookup(i77)->value, 77);
}


TEST(pool_test, dynamic_growth_disabled)
{
    struct Foo
    {
        std::array<uint8_t, 100> data;
    };

    jaws::util::Pool<Foo> p(0, 3, 0);

    auto a = p.emplace();
    EXPECT_EQ(p.get_element_count(), 1);

    auto b = p.emplace();
    EXPECT_EQ(p.get_element_count(), 2);

    auto c = p.emplace();
    EXPECT_EQ(p.get_element_count(), 3);

    auto d = p.emplace();
    EXPECT_EQ(d, -1);
}

TEST(pool_test, dynamic_growth)
{
    struct Foo
    {
        std::array<uint8_t, 100> data;
    };

    jaws::util::Pool<Foo> p(0, 4, 1.5);

    auto a = p.emplace();
    EXPECT_EQ(p.get_element_count(), 1);

    auto b = p.emplace();
    EXPECT_EQ(p.get_element_count(), 2);

    auto c = p.emplace();
    EXPECT_EQ(p.get_element_count(), 3);

    auto d = p.emplace();
    EXPECT_EQ(p.get_element_count(), 4);
    EXPECT_EQ(p.get_capacity(), 4);

    auto e = p.emplace();
    EXPECT_EQ(p.get_element_count(), 5);
    EXPECT_EQ(p.get_capacity(), 6);
}
