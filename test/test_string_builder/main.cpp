#include "jaws/jaws.hpp"
#include "jaws/util/string_lines_builder.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include <string_view>

TEST(string_builder_test, simple) {
    jaws::util::StringLinesBuilder b;
    b.append(std::string("foo"));
    b.append(std::string("bar\n"));
    b.append(std::string("baz"));

    EXPECT_STREQ(b.str().c_str(), "foo\nbar\nbaz\n");
}
