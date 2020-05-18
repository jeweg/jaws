#include "jaws/jaws.hpp"
#include "jaws/util/string_lines_builder.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include <string_view>

TEST(string_builder_test, xxx) {

    // abseil uses absl::string_view to pass the pass std::string
    // to its internals. When C++17 support is detected, that type
    // is a typedef to std::string_view, and whether it is is decided
    // in an abseil header file which is compiled in both abseil itself
    // and by user code. This is a problem if the language versions
    // don't match between those builds.
    // Depending on the build proces, this might not be an easy thing to achieve,
    // and if the mismatch is present, weird crashes will happen
    // (abseil and GCC7's string_view have flipped members, for instance).
    // This test forces the issue.

    std::string accum;

    absl::StrAppend(&accum, std::string("hello, "));

    std::string w("world!");
    absl::StrAppend(&accum, w);

    EXPECT_STREQ(accum.c_str(), "hello, world!");
}
