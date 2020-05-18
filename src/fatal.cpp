#include "pch.hpp"
#include "jaws/jaws.hpp"
#include "jaws/fatal.hpp"
#include "debugbreak.h"
#include "absl/debugging/stacktrace.h"
#include "absl/debugging/symbolize.h"
#include "fmt/format.h"
#include <sstream>

namespace jaws {
namespace detail {

std::string get_formatted_stackframes(int stackframes_skipped)
{
    static auto init = []() -> bool {
        auto cmd = Jaws::instance()->get_cmd_line_args();
        if (cmd.empty()) {
            absl::InitializeSymbolizer("");
        } else {
            absl::InitializeSymbolizer(cmd[0].c_str());
        }
        return true;
    }();

    static void* result[50];
    static int sizes[50];
    int num_frames = absl::GetStackFrames(result, sizes, sizeof(result) / sizeof(result[0]), stackframes_skipped);
    static char buffer[2000];
    std::ostringstream oss;

    if (stackframes_skipped > 0) {
        oss << fmt::format("...{} stackframes skipped...\n", stackframes_skipped);
    }

    for (int i = 0; i < num_frames; ++i) {
        absl::Symbolize(result[i], buffer, sizeof(buffer));
        oss << fmt::format("[{}] {}\n", stackframes_skipped + i, buffer);
    }
    return oss.str();
}

} // namespace detail


/// Building block for fatal handler.
void LogFatalError(FatalError error_code, std::string_view msg, const char* function, const char* file, long line,
    bool stackframes, int stackframes_skipped)
{
    using std::to_string;
    auto& logger = get_logger(Category::General);
    if (stackframes) {

        logger.critical(
            "jaws fatal error: \"{}\" ({}) in function '{}' at {}:{}\nBacktrace:\n{}",
            msg,
            to_string(error_code),
            function,
            file,
            line,
            detail::get_formatted_stackframes(stackframes_skipped));

    } else {

        logger.critical(
            "jaws fatal error: \"{}\" ({}) in function '{}' at {}:{}",
            msg,
            to_string(error_code),
            function,
            file,
            line);

    }
    logger.flush();
}


void DefaultFatalHandler(FatalError error_code, std::string_view msg, const char* function, const char* file, long line)
{
    LogFatalError(error_code, msg, function, file, line, true, 5);

    // TODO: make this configurable:
    debug_break();
    // and also maybe on Windows: force a segmentation fault here so we get the option of starting a debugger
    // if we're not already running in one (in which case debug_break will not do anything).

    std::abort();
}

} // namespace jaws
