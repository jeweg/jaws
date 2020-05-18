#include "jaws/filesystem.hpp"
#include "absl/hash/hash.h"
#include "absl/container/flat_hash_set.h"
#include <iostream>
#include <functional>
#include <string>

namespace fs = std::filesystem;

struct UserType
{
    int foo;
    int bar;
    fs::path p;

    template <typename H>
    friend H AbslHashValue(H h, const UserType& c)
    {
        return H::combine(std::move(h), c.foo, c.bar, c.p);
    }
};

// This is required not only for std::hash to work, but also for
// abseil's hashing as abseil probes for a std hash specialization.
namespace std {
template <>
struct hash<std::filesystem::path>
{
    size_t operator()(std::filesystem::path const& s) const noexcept { return std::hash<std::string>{}(s.string()); }
};
} // namespace std

int main()
{
    using std::cout;

#if 1
    cout << std::hash<int>{}(1234) << "\n";
    cout << std::hash<float>{}(1234) << "\n";
    cout << std::hash<std::string>{}("abcd") << "\n";
    cout << std::hash<fs::path>{}("c:/abcd") << "\n";
    // cout << std::hash<UserType>{}(UserType{123, 654}) << "\n";
#endif

    // Abseil does not require a std::hash specialization for
    // user types, and I don't know why that works differently than
    // std::filesystem::path. Abseil does not seem to have to have a
    // specific specialization of absl::Hash for paths.

    cout << absl::Hash<int>{}(1234) << "\n";
    cout << absl::Hash<float>{}(1234) << "\n";
    cout << absl::Hash<std::string>{}("abcd") << "\n";
    cout << absl::Hash<fs::path>{}("c:/abcd") << "\n";
    cout << absl::Hash<UserType>{}(UserType{123, 654}) << "\n";

    absl::flat_hash_set<int> set1;
    absl::flat_hash_set<float> set2;
    absl::flat_hash_set<std::string> set3;
    absl::flat_hash_set<fs::path> set4;
    absl::flat_hash_set<UserType> set5;
};
