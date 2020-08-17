#include "pch.hpp"
#include "jaws/errors.hpp"

namespace jaws {

std::string to_string(ResultCode r)
{
    switch (r) {
    case ResultCode::Success: return "Success";
    case ResultCode::UnspecifiedFailure: return "Unspecified failure";
    default: return "<unknown result code>";
    }
}

} // namespace jaws
