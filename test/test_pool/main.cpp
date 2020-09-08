#include "jaws/util/pool.hpp"
#include "gtest/gtest.h"
#include <array>


#include <iostream>

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

    EXPECT_TRUE(p.lookup(0) == nullptr);

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


TEST(pool_test, clear)
{
    jaws::util::Pool<char> p(0, 10);
    auto a = p.emplace('A');
    auto b = p.insert('B');
    auto c = p.emplace('C');

    EXPECT_EQ(p.size(), 3);
    EXPECT_FALSE(p.empty());

    p.clear();
    EXPECT_EQ(p.size(), 0);
    EXPECT_TRUE(p.empty());

    auto d = p.emplace('D');
    EXPECT_EQ(p.size(), 1);
    EXPECT_FALSE(p.empty());

    p.clear();
    EXPECT_EQ(p.size(), 0);
    EXPECT_TRUE(p.empty());
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

    jaws::util::Pool<Foo> p(3, 0);

    auto a = p.emplace();
    EXPECT_EQ(p.get_element_count(), 1);

    auto b = p.emplace();
    EXPECT_EQ(p.get_element_count(), 2);

    auto c = p.emplace();
    EXPECT_EQ(p.get_element_count(), 3);

    auto d = p.emplace();
    EXPECT_EQ(d.is_valid(), false);
}

TEST(pool_test, dynamic_growth)
{
    struct Foo
    {
        std::array<uint8_t, 100> data;
    };

    jaws::util::Pool<Foo> p(4, 1.5);

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

TEST(pool_test, less_bits)
{
    jaws::util::Pool<int, uint8_t, 2, 3> p;

    EXPECT_EQ(p.MAX_ELEM_COUNT, 7);
    for (int i = 0; i < 7; ++i) {
        auto id = p.insert(i);
        EXPECT_TRUE(id.is_valid());
    }
    auto id = p.insert(88);
    EXPECT_TRUE(!id.is_valid());
}

TEST(pool_test, less_bits_for_generations)
{
    jaws::util::Pool<int, uint8_t, 2, 3> p;

    for (int i = 0; i < 7; ++i) {
        auto id = p.insert(i);
        EXPECT_TRUE(id.is_valid());

        p.remove(id);
        EXPECT_TRUE(p.lookup(id) == nullptr);
    }
}

TEST(pool_test, generation_check)
{
    // Make a pool with 1 element max.
    jaws::util::Pool<int, int, 4, 1> p;
    EXPECT_EQ(p.MAX_ELEM_COUNT, 1);

    // Insert an element and keep its id around in held_id.
    auto id = p.insert(123);
    auto held_id = id;

    // Make sure the pool is now indeed full.
    auto full_id = p.insert(123);
    EXPECT_TRUE(!full_id.is_valid());

    // Remove and insert new element. The pool can only ever
    // hold 1 element max, so this new element gets necessarily stored in the
    // position of the previously removed one.
    p.remove(id);
    id = p.insert(345);
    EXPECT_TRUE(*p.lookup(id) == 345);

    // If we used indices only, held_id would now address 345. Alas...
    EXPECT_TRUE(p.lookup(held_id) == nullptr);
}
