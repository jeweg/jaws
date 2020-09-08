#pragma once
#include "vfs/fwd.hpp"
#include "spdlog/logger.h"
#include <functional>
#include <string>

namespace jaws {

enum class FatalError : int32_t
{
    Unspecified = 0,
    AssumptionFailed,
    UncaughtException,
};

using FatalHandler = std::function<void(
    FatalError error_code, const std::string &msg, const char *function, const char *file, long line)>;

class Logging;
enum class Category
{
    User = 0,
    General,
    UncaughtException,
    OpenGL,
    Vulkan,
    VulkanValidation
};
using Logger = spdlog::logger;
using LogLevel = spdlog::level::level_enum;

}
