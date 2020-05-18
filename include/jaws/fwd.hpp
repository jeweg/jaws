#pragma once
#include "vfs/fwd.hpp"
#include "vulkan/fwd.hpp"
#include "spdlog/logger.h"
#include <functional>
#include <string>

namespace jaws {

class Jaws;

enum class FatalError : int32_t
{
    Unspecified = 0,
    AssumptionFailed,
    UncaughtException,
};

using FatalHandler = std::function<void(
    FatalError error_code, const std::string& msg, const char* function, const char* file, long line)>;


class Logging;
enum class Category
{
    User = 0,
    General,
    UncaughtException,
    OpenGL,
    Vulkan
};
using Logger = spdlog::logger;
using LoggerPtr = std::shared_ptr<spdlog::logger>;
using LogLevel = spdlog::level::level_enum;

} // namespace jaws
