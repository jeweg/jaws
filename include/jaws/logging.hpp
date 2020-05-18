#pragma once
#include "jaws/core.hpp"
#include "jaws/fwd.hpp"
#include "jaws/assume.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/logger.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <cinttypes>
#include <string>
#include <memory>

namespace jaws {

/*
Previously:

using Logger = spdlog::logger;
using LoggerPtr = std::shared_ptr<Logger>;

using LogLevel = spdlog::level::level_enum;
extern JAWS_API Logger& GetLogger(Category);
extern JAWS_API LoggerPtr GetLoggerPtr(Category);

extern JAWS_API std::string GetLoggerName(Category);
*/

class JAWS_API Logging
{
public:
    Logging() :
        _logger_data{LoggerData("User"),
                     LoggerData("General"),
                     LoggerData("UncaughtException"),
                     LoggerData("OpenGL"),
                     LoggerData("Vulkan")}
    {}

    LoggerPtr get_logger_ptr(Category cat)
    {
        size_t i = static_cast<size_t>(cat);
        JAWS_ASSUME(i < _logger_data.size());
        return _logger_data[i].get_logger();
    }

    Logger& get_logger(Category cat)
    {
        return *get_logger_ptr(cat);
    }

private:
    // spdlog has a lookup facility to look up loggers by name.
    // we make this stronger typed and wrap it into a lookup-by-enum thingie.
    // TODO: spdlog and static lib. where does the global data go? can we avoid globals?
    struct LoggerData
    {
        std::string name;
        // shared_ptr because spdlog.
        LoggerPtr logger;

        explicit LoggerData(std::string name) : name(std::move(name)) {}

        LoggerPtr get_logger()
        {
            if (!logger) {
                logger = spdlog::get(name);
                if (!logger) {
                    // Init logger with default settings
                    std::vector<spdlog::sink_ptr> sinks;
                    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
                    logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
                    // Auto-flush on error. Seems useful.
                    logger->flush_on(spdlog::level::err);
                    spdlog::register_logger(logger);
                }
            }
            return logger;
        }
    };
    std::array<LoggerData, 5> _logger_data;
};


}; // namespace jaws
