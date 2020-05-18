#include "jaws/util/instance_counter.hpp"
#include "gtest/gtest.h"
#include <iostream>

struct A : public jaws::util::InstanceCounter<A>
{
};


TEST(instance_counter_test, basic)
{
    EXPECT_EQ(0, A::get_alive_instance_count());
    {
        A a1;
        EXPECT_EQ(1, A::get_alive_instance_count());

        {
            A a2;
            EXPECT_EQ(2, A::get_alive_instance_count());

            A a3 = std::move(a2);
            EXPECT_EQ(3, A::get_alive_instance_count());
        }
        EXPECT_EQ(1, A::get_alive_instance_count());
    }
    {
        A a4;
        EXPECT_EQ(1, A::get_alive_instance_count());

        A a5{a4};
        EXPECT_EQ(2, A::get_alive_instance_count());
    }
    EXPECT_EQ(0, A::get_alive_instance_count());
}
