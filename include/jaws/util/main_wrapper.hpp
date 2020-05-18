#pragma once

#include "jaws/core.hpp"
#include "jaws/fatal.hpp"

#include <stdexcept>

namespace jaws::util {

template <typename UserMain>
int Main(int argc, char** argv, UserMain user_main)
{
    try {
        return user_main(argc, argv);
    } catch (const std::exception& e) {
        JAWS_FATAL2(FatalError::UncaughtException, std::string("uncaught std::exception: ") + e.what());
        return -2;
    } catch (...) {
        JAWS_FATAL2(FatalError::UncaughtException, "uncaught unknown exception");
        return -3;
    }
}

} // namespace jaws::util
