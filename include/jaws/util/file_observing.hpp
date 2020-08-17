#pragma once

#include "jaws/filesystem.hpp"

namespace jaws::util::file_observing {

enum class Event
{
    None,
    Changed,
    Created,
    Deleted
};

struct State
{
    bool valid = false;
    bool exists = false;
    std::filesystem::file_time_type last_write_time;
};


JAWS_API Event get_file_event(const std::filesystem::path &path, State &in_out_state);

} // namespace jaws::util::file_observing
