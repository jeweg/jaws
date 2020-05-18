#include "jaws/jaws.hpp"
#include "gtest/gtest.h"
#include <sstream>

TEST(IndentingOStreamTest, Usage)
{
    using jaws::util::IndentingOStream;
    using jaws::util::Indenting;

    std::ostringstream oss;
    IndentingOStream ios(oss);

    ios << "hello\n";
    ios.ChangeLevelBy(1);
    ios << "hello" << std::endl << "world!";
    ios.ChangeLevelBy(2);
    ios << "hello" << std::endl << "world!";
    ios.ChangeLevelTo(0);
    ios << "\n";

    // Using RAII:
    ios << "Using RAII\n";
    ios << "a\n";
    ios << "b\n";
    {
        Indenting indenting(ios);
        ios << "c\n";
        ios << "d\n";
        {
            Indenting indenting(ios);
            ios << "e\n";
            ios << "f\n";
        }
        ios << "g\n";
        ios << "h\n";
    }
    ios << "i\n";
    ios << "j\n";

    const auto& result = oss.str();

    EXPECT_STREQ(
        R"end(hello
    hello
    world!hello
            world!
Using RAII
a
b
    c
    d
        e
        f
    g
    h
i
j
)end",
        result.c_str());
}
