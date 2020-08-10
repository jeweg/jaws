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

    Logger& get_logger(Category cat)
    {
        size_t i = static_cast<size_t>(cat);
        JAWS_ASSUME(i < _logger_data.size());
        return _logger_data[i].get_logger();
    }

private:
    // spdlog has a lookup facility to look up loggers by name.
    // we make this stronger typed and wrap it into a lookup-by-enum thingie.
    struct LoggerData
    {
        std::string name;

        // spdlog hands out shared pointers to loggers.
        // We don't actually want to share ownership with the user here so
        // we hold it by shared_ptr only here, binding the loggers to the lifetime
        // of jaws itself. We hand out references to the user.
        std::shared_ptr<spdlog::logger> spd_logger;

        explicit LoggerData(std::string name) : name(std::move(name)) {}

        Logger& get_logger()
        {
            // Create logger on demand.
            if (!spd_logger) {
                spd_logger = spdlog::get(name);
                if (!spd_logger) {
                    // Init logger with default settings
                    std::vector<spdlog::sink_ptr> sinks;
                    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
                    spd_logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
                    // Auto-flush on error. Seems useful.
                    spd_logger->flush_on(spdlog::level::err);
                    spdlog::register_logger(spd_logger);
                }
            }
            return *spd_logger;
        }
    };
    std::array<LoggerData, 5> _logger_data;
};


}; // namespace jaws
