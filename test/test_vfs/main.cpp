#include "jaws/vfs/vfs_backend.hpp"
#include "gtest/gtest.h"
#include <string>

using namespace jaws::vfs;
using namespace std::string_literals;

TEST(vfs_test, path_basic)
{
    Path x("x:/foo/bar/baz.ext");
    EXPECT_EQ(x.get_parents(), "/foo/bar/");

    Path a("foo/bar.x");
    EXPECT_EQ(a.get_extension(), ".x");

    Path b("blah");
    EXPECT_EQ(b.get_extension(), "");

    b = a;
    EXPECT_EQ(b.get_extension(), ".x");

    Path c("x:/foo/bar/baz.ext");
    EXPECT_EQ(c.get_parents(), "/foo/bar/");

    b = std::move(c);
    EXPECT_EQ(b.get_parents(), "/foo/bar/");

    b = " foo :123";
    EXPECT_EQ(b.get_parents(), "");

    x = b.get_parent_path();
    EXPECT_STREQ(x.c_str(), " foo :");
};

TEST(vfs_test, path_parts)
{
    auto part = [](const Path& path, Path::Part part) {
        auto sv = path.get_part(part);
        EXPECT_LT(sv.size(), 1000);
        return std::string(sv);
    };

    {
        Path p("");
        EXPECT_EQ(part(p, Path::Part::Domain), "");
        EXPECT_EQ(part(p, Path::Part::Path), "");
        EXPECT_EQ(part(p, Path::Part::Parents), "");
        EXPECT_EQ(part(p, Path::Part::Filename), "");
        EXPECT_EQ(part(p, Path::Part::Stem), "");
        EXPECT_EQ(part(p, Path::Part::Extension), "");
        EXPECT_EQ(p.is_absolute(), false);
        EXPECT_EQ(p.is_relative(), true);
    }
    {
        Path p("x");
        EXPECT_EQ(part(p, Path::Part::Domain), "");
        EXPECT_EQ(part(p, Path::Part::Path), "x");
        EXPECT_EQ(part(p, Path::Part::Parents), "");
        EXPECT_EQ(part(p, Path::Part::Filename), "x");
        EXPECT_EQ(part(p, Path::Part::Stem), "x");
        EXPECT_EQ(part(p, Path::Part::Extension), "");
        EXPECT_EQ(p.is_absolute(), false);
        EXPECT_EQ(p.is_relative(), true);
    }
    {
        Path p(":x");
        EXPECT_EQ(part(p, Path::Part::Domain), ":");
        EXPECT_EQ(part(p, Path::Part::Path), "x");
        EXPECT_EQ(part(p, Path::Part::Parents), "");
        EXPECT_EQ(part(p, Path::Part::Filename), "x");
        EXPECT_EQ(part(p, Path::Part::Stem), "x");
        EXPECT_EQ(part(p, Path::Part::Extension), "");
        EXPECT_EQ(p.is_absolute(), false);
        EXPECT_EQ(p.is_relative(), true);
    }
    {
        Path p("z:x");
        EXPECT_EQ(part(p, Path::Part::Domain), "z:");
        EXPECT_EQ(part(p, Path::Part::Path), "x");
        EXPECT_EQ(part(p, Path::Part::Parents), "");
        EXPECT_EQ(part(p, Path::Part::Filename), "x");
        EXPECT_EQ(part(p, Path::Part::Stem), "x");
        EXPECT_EQ(part(p, Path::Part::Extension), "");
        EXPECT_EQ(p.is_absolute(), false);
        EXPECT_EQ(p.is_relative(), true);
    }
    {
        Path p("z:/x");
        EXPECT_EQ(part(p, Path::Part::Domain), "z:");
        EXPECT_EQ(part(p, Path::Part::Path), "/x");
        EXPECT_EQ(part(p, Path::Part::Parents), "/");
        EXPECT_EQ(part(p, Path::Part::Filename), "x");
        EXPECT_EQ(part(p, Path::Part::Stem), "x");
        EXPECT_EQ(part(p, Path::Part::Extension), "");
        EXPECT_EQ(p.is_absolute(), true);
        EXPECT_EQ(p.is_relative(), false);
    }
    {
        Path p("foo bar:123/4567/8/9/123.foobar");
        EXPECT_EQ(part(p, Path::Part::Domain), "foo bar:");
        EXPECT_EQ(part(p, Path::Part::Path), "123/4567/8/9/123.foobar");
        EXPECT_EQ(part(p, Path::Part::Parents), "123/4567/8/9/");
        EXPECT_EQ(part(p, Path::Part::Filename), "123.foobar");
        EXPECT_EQ(part(p, Path::Part::Stem), "123");
        EXPECT_EQ(part(p, Path::Part::Extension), ".foobar");
        EXPECT_EQ(p.is_absolute(), false);
        EXPECT_EQ(p.is_relative(), true);
    }
    {
        Path p("123/4567/8/9/123.foobar");
        EXPECT_EQ(part(p, Path::Part::Domain), "");
        EXPECT_EQ(part(p, Path::Part::Path), "123/4567/8/9/123.foobar");
        EXPECT_EQ(part(p, Path::Part::Parents), "123/4567/8/9/");
        EXPECT_EQ(part(p, Path::Part::Filename), "123.foobar");
        EXPECT_EQ(part(p, Path::Part::Stem), "123");
        EXPECT_EQ(part(p, Path::Part::Extension), ".foobar");
        EXPECT_EQ(p.is_absolute(), false);
        EXPECT_EQ(p.is_relative(), true);
    }
    {
        Path p("/123/4567/8/9/123.foobar");
        EXPECT_EQ(part(p, Path::Part::Domain), "");
        EXPECT_EQ(part(p, Path::Part::Path), "/123/4567/8/9/123.foobar");
        EXPECT_EQ(part(p, Path::Part::Parents), "/123/4567/8/9/");
        EXPECT_EQ(part(p, Path::Part::Filename), "123.foobar");
        EXPECT_EQ(part(p, Path::Part::Stem), "123");
        EXPECT_EQ(part(p, Path::Part::Extension), ".foobar");
        EXPECT_EQ(p.is_absolute(), true);
        EXPECT_EQ(p.is_relative(), false);
    }
    {
        Path p("foo bar:");
        EXPECT_EQ(part(p, Path::Part::Domain), "foo bar:");
        EXPECT_EQ(part(p, Path::Part::Path), "");
        EXPECT_EQ(part(p, Path::Part::Parents), "");
        EXPECT_EQ(part(p, Path::Part::Filename), "");
        EXPECT_EQ(part(p, Path::Part::Stem), "");
        EXPECT_EQ(part(p, Path::Part::Extension), "");
        EXPECT_EQ(p.is_absolute(), false);
        EXPECT_EQ(p.is_relative(), true);
    }
    {
        Path p("fo::o :bar:xxx");
        EXPECT_EQ(part(p, Path::Part::Domain), "fo::o :bar:");
        EXPECT_EQ(part(p, Path::Part::Path), "xxx");
        EXPECT_EQ(part(p, Path::Part::Parents), "");
        EXPECT_EQ(part(p, Path::Part::Filename), "xxx");
        EXPECT_EQ(part(p, Path::Part::Stem), "xxx");
        EXPECT_EQ(part(p, Path::Part::Extension), "");
        EXPECT_EQ(p.is_absolute(), false);
        EXPECT_EQ(p.is_relative(), true);
    }
    {
        Path p(":123/4567/8/9/123.foo/bar");
        EXPECT_EQ(part(p, Path::Part::Domain), ":");
        EXPECT_EQ(part(p, Path::Part::Path), "123/4567/8/9/123.foo/bar");
        EXPECT_EQ(part(p, Path::Part::Parents), "123/4567/8/9/123.foo/");
        EXPECT_EQ(part(p, Path::Part::Filename), "bar");
        EXPECT_EQ(part(p, Path::Part::Stem), "bar");
        EXPECT_EQ(part(p, Path::Part::Extension), "");
        EXPECT_EQ(p.is_absolute(), false);
        EXPECT_EQ(p.is_relative(), true);
    }
    {
        Path p(" foo :123/4567/8/9/123.foo/bar.");
        EXPECT_EQ(part(p, Path::Part::Domain), " foo :");
        EXPECT_EQ(part(p, Path::Part::Path), "123/4567/8/9/123.foo/bar.");
        EXPECT_EQ(part(p, Path::Part::Parents), "123/4567/8/9/123.foo/");
        EXPECT_EQ(part(p, Path::Part::Filename), "bar.");
        EXPECT_EQ(part(p, Path::Part::Stem), "bar");
        EXPECT_EQ(part(p, Path::Part::Extension), ".");
        EXPECT_EQ(p.is_absolute(), false);
        EXPECT_EQ(p.is_relative(), true);
    }
    {
        Path p(" foo :123/4567/8/9/123.foo/bar.baz.blah");
        EXPECT_EQ(part(p, Path::Part::Domain), " foo :");
        EXPECT_EQ(part(p, Path::Part::Path), "123/4567/8/9/123.foo/bar.baz.blah");
        EXPECT_EQ(part(p, Path::Part::Parents), "123/4567/8/9/123.foo/");
        EXPECT_EQ(part(p, Path::Part::Filename), "bar.baz.blah");
        EXPECT_EQ(part(p, Path::Part::Stem), "bar.baz");
        EXPECT_EQ(part(p, Path::Part::Extension), ".blah");
        EXPECT_EQ(p.is_absolute(), false);
        EXPECT_EQ(p.is_relative(), true);
    }
}

TEST(vfs_test, path_join)
{
    {
        Path p = Path("foo:/x/y") / Path("z/foo.bar");
        EXPECT_EQ(p.string(), "foo:/x/y/z/foo.bar");
    }
    {
        Path p = Path("foo:/x/y/") / Path("z/foo.bar");
        EXPECT_EQ(p.string(), "foo:/x/y/z/foo.bar");
    }
    {
        Path p = Path("foo:/x/y/") / Path("bar:/z/foo.bar");
        EXPECT_EQ(p.string(), "foo:/x/y/z/foo.bar");
    }
    {
        Path p = Path("foo:/x/y") / Path("/z/foo.bar");
        EXPECT_EQ(p.string(), "foo:/x/y/z/foo.bar");
    }
}

TEST(vfs_test, path_parents)
{
    // Compared to std::filesystem::path (in GCC 9.2), this behaves similarly,
    // except for the behavior at the end where std::fs removes the root directory
    // and root path, making the path fully empty at the end. We don't want to remove
    // the domain nor the root directory in case of an absolute path -- that'd
    // be an undue change of meaning for the path.
    {
        Path p(" foo :123/4567/8/9/123.foo/bar.baz.blah");
        Path p2 = p;
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :123/4567/8/9/123.foo");
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :123/4567/8/9");
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :123/4567/8");
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :123/4567");
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :123");
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :");
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :");
    }
    {
        Path p(" foo :/123/4567/8/9/123.foo/bar.baz.blah");
        Path p2 = p;
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :/123/4567/8/9/123.foo");
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :/123/4567/8/9");
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :/123/4567/8");
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :/123/4567");
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :/123");
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :/");
        p2 = p2.get_parent_path();
        EXPECT_STREQ(p2.c_str(), " foo :/");
    }
}

