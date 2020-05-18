#include "jaws/util/main_wrapper.hpp"
#include <iostream>

#if 0

// Standard exception thrown:
int main(int argc, char** argv)
{
    return jaws::util::Main(argc, argv, [](auto...) {
        std::cout << "Hello, World!\n";
        throw std::logic_error("xyz");
        return 0;
        });
}

#elif 0

// Custom (unknown to the wrapper) exception thrown:
struct A {};
int main(int argc, char** argv)
{
    return jaws::util::Main(argc, argv, [](auto...) {
        std::cout << "Hello, World!\n";
        throw A{};
        return 0;
        });
}

#elif 1

// Not using a lambda. The extra indentation might not be desired.

int MyMain(int argc, char** argv)
{
    std::cout << "Hello, World!\n";
    return 0;
}

int main(int argc, char** argv)
{
    return jaws::util::Main(argc, argv, MyMain);
}

#endif
