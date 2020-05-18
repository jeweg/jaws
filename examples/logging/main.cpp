
// Why doesn't this work?
/*
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define SPDLOG_DEBUG_ON
#define SPDLOG_TRACE_ON
 */

#include "jaws/jaws.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

/*
The compile-time log levels.
SPDLOG_ACTIVE_LEVEL must be set before spdlog is included.

#define SPDLOG_LEVEL_TRACE 0
#define SPDLOG_LEVEL_DEBUG 1
#define SPDLOG_LEVEL_INFO 2
#define SPDLOG_LEVEL_WARN 3
#define SPDLOG_LEVEL_ERROR 4
#define SPDLOG_LEVEL_CRITICAL 5
#define SPDLOG_LEVEL_OFF 6
#if !defined(SPDLOG_ACTIVE_LEVEL)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif
*/

void Bar(float value)
{
    SPDLOG_DEBUG("compile-time trace log level");
}

void Foo(int value)
{
    SPDLOG_DEBUG("compile-time debug log level");
    Bar(static_cast<float>(value));
}


int main(int argc, char** argv)
{
    jaws::Instance instance;
    instance.create(argc, argv);
    spdlog::set_level(spdlog::level::trace);

    for (int i = 0; i < 2; ++i) {
        if (i == 1) {
            auto file_logger = spdlog::basic_logger_mt("basic_logger", "d:/example_log.txt");
            spdlog::set_default_logger(file_logger);
        }

        // Set the default logger to file logger
        spdlog::info("Welcome to spdlog!");
        spdlog::error("Some error message with arg: {}", 1);

        spdlog::warn("Easy padding in numbers like {:08d}", 12);
        spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
        spdlog::info("Support for floats {:03.2f}", 1.23456);
        spdlog::info("Positional args are {1} {0}..", "too", "supported");
        spdlog::info("{:<30}", "left aligned");

        spdlog::set_level(spdlog::level::debug);
        spdlog::debug("This message should be displayed..");
        spdlog::set_level(spdlog::level::info);
        spdlog::debug("This message should NOT be displayed..");

        // change log pattern
        spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");

        // Compile time log levels
        // define SPDLOG_ACTIVE_LEVEL to desired level
        SPDLOG_TRACE("Some trace message with param {}", 3);
        SPDLOG_DEBUG("Some debug message");
        SPDLOG_TRACE("1");
        SPDLOG_TRACE("2");
        SPDLOG_TRACE("3");
        Foo(123);
    }
}
