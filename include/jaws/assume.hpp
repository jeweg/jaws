#pragma once

#include "jaws/fatal.hpp"

// This is modelled after boost assert in case you're wondering.
#define JAWS_ASSUME(expr) (!!(expr) ? ((void)0) : JAWS_FATAL2(::jaws::FatalError::AssumptionFailed, #expr))

#if !defined(NDEBUG)
#define JAWS_DEBUG_ASSUME(expr) JAWS_ASSUME(expr)
#else
#define JAWS_DEBUG_ASSUME(expr) do {} while (false)
#endif



