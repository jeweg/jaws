#pragma once
#include "jaws/core.hpp"
#include <string>

namespace jaws {

enum class ResultCode
{
    Success = 0,
    UnspecifiedFailure = 9999
};

JAWS_API std::string to_string(ResultCode);

inline bool Failed(ResultCode result)
{
    return result != ResultCode::Success;
}

} // namespace jaws
