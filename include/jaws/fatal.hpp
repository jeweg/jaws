#pragma once
#include "jaws/jaws.hpp"
#include <string>
#include <string_view>

// This draws inspiration from
// https://www.boost.org/doc/libs/1_66_0/doc/html/stacktrace/getting_started.html#stacktrace.getting_started.better_asserts
// and boost.assert.

// Fatal errors are for unrecoverable errors.
// Those are not (and arguably should not) be reported to calling code.

namespace jaws {

constexpr std::string_view JAWS_API to_string(FatalError e)
{
    switch (e) {
    case FatalError::Unspecified: return "unspecified";
    case FatalError::AssumptionFailed: return "assumption failed";
    case FatalError::UncaughtException: return "uncaught exception";
    default: return "<unknown fatal error>";
    }
}

// Helper building block for custom handlers. Logs the fatal error using our default format. Does nothing beyond that
// (like aborting the process).
void JAWS_API
LogFatalError(FatalError error_code, std::string_view msg, const char* function, const char* file, long line);

// Default handler. Logs the error using LogFatalError and then calls std::terminate.
void JAWS_API
DefaultFatalHandler(FatalError error_code, std::string_view msg, const char* function, const char* file, long line);

} // namespace jaws

#define JAWS_FATAL0()                               \
    (::jaws::get_fatal_handler()( \
        ::jaws::FatalError::Unspecified, {}, JAWS_CURRENT_FUNCTION, __FILE__, __LINE__))

#define JAWS_FATAL1(msg)                            \
    (::jaws::get_fatal_handler()( \
        ::jaws::FatalError::Unspecified, (msg), JAWS_CURRENT_FUNCTION, __FILE__, __LINE__))

#define JAWS_FATAL2(error_code, msg) \
    (::jaws::get_fatal_handler()((error_code), (msg), JAWS_CURRENT_FUNCTION, __FILE__, __LINE__))
