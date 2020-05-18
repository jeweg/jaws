#include "jaws/util/ref_ptr.hpp"
#include "jaws/util/instance_counter.hpp"
#include "gtest/gtest.h"
#include <string>

using namespace jaws::util;

struct A : public RefCounted<A>, public jaws::util::InstanceCounter<A>
{};

TEST(ref_ptr_test, basic)
{
    using pA = ref_ptr<A>;

    EXPECT_EQ(A::get_alive_instance_count(), 0);
    pA a1(new A);
    EXPECT_EQ(A::get_alive_instance_count(), 1);
    EXPECT_EQ(a1->get_ref_count(), 1);

    pA a2(a1);
    EXPECT_EQ(a1->get_ref_count(), 2);

    {
        pA a3(a1);
        EXPECT_EQ(a1->get_ref_count(), 3);
    }
    EXPECT_EQ(a1->get_ref_count(), 2);

    a2.reset();
    EXPECT_EQ(a2, false);
    EXPECT_EQ(a1->get_ref_count(), 1);

    pA dummy;
    a1.swap(dummy);
    EXPECT_EQ(a1, false);
    EXPECT_EQ(dummy, true);
    dummy = nullptr;
    EXPECT_EQ(A::get_alive_instance_count(), 0);
}
