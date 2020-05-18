#include <jaws/assume.hpp>

int main(int argc, char** argv)
{
    jaws::Jaws j;
    j.create(argc, argv);

    // This is evaluated in both debug and release builds.
    JAWS_ASSUME(31 == 1 + 4 - 2);

    // This is only evaluated in non-release builds (if NDEBUG is NOT defined).
    JAWS_DEBUG_ASSUME(3 == 1 + 4 - 1);
}

