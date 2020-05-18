#include "jaws/jaws.hpp"
#include "celero\Celero.h"
#include "xxhash/xxhash.hpp"
#include "xxhash/xxhash_cx.h"
#include <utility> // std::hash
#include <string>
#include <unordered_map>

// Here I'm experimenting with xxHash.
jaws::LoggerPtr logger;

#if 0

using namespace std::literals::string_literals;
auto foo(int count)
{
    std::vector<std::size_t> result;
    for (int i = 0; i < count; ++i) {
        result.push_back(xxh::xxhash<64>("small"s));
        result.push_back(xxh::xxhash<64>("a little longer string blablabla"s));
        result.push_back(xxh::xxhash<64>(
            "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore "
            "et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea "
            "rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum "
            "dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod temp"s));
    }
    return result;
}
auto bar(int count)
{
    std::vector<std::size_t> result;
    auto hasher = std::hash<std::string>{};
    for (int i = 0; i < count; ++i) {
        result.push_back(hasher("small"));
        result.push_back(hasher("a little longer string blablabla"));
        result.push_back(
            hasher("Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut "
                   "labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo "
                   "dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit "
                   "amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod temp"));
    }
    return result;
}
BASELINE(xxHash, Baseline10, 10, 100)
{
    foo(10);
}
BENCHMARK(xxHash, Baseline1000, 10, 100)
{
    foo(10000);
}
BASELINE(msvc, Baseline10, 10, 100)
{
    bar(10);
}
BENCHMARK(msvc, Baseline1000, 10, 100)
{
    bar(10000);
}
CELERO_MAIN;

#else

constexpr auto get_hash()
{
    return xxhash::xxh64("hello", 5, 0);
}

class ConstString
{
public:
    ConstString(jaws::string_view sv, std::size_t hash_value) : _hash_value(hash_value)
    {
        // possibly also make a copy of the string for
        // debugging.
        if (true) { _str = sv; }
    }

    std::string _str;
    std::size_t _hash_value;
};

namespace std {

template<> struct hash<ConstString>
{
    std::size_t operator()(const ConstString& cs) const noexcept
    {
        return cs._hash_value;
    }
};
} // namespace std


#    define JAWS_CONST_STR(arg) ConstString(arg, xxhash::xxh64(arg, sizeof(arg), 0))

int main(int argc, char** argv)
{
    logger = jaws::GetLoggerPtr(jaws::Category::General);
    logger->info("Hello, World!");

    std::unordered_map<ConstString, int> foo;
    foo[JAWS_CONST_STR("hello")] = 3;
    foo[JAWS_CONST_STR("hello")] = 2;


    // logger->info("{}", hv);
}

#endif
