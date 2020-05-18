#include "pch.hpp"
#include "jaws/util/file_observing.hpp"
#include "jaws/filesystem.hpp"

namespace fs = std::filesystem;

namespace jaws::util::file_observing {

/*
{
    None,
    Changed,
    Created,
    Deleted
};
*/


Event get_file_event(const std::filesystem::path& path, State& in_out_state)
{
    std::error_code ec;
    State now{true};
    now.exists = fs::exists(path, ec);

    Event evt = [&]() {
        if (now.exists) { now.last_write_time = fs::last_write_time(path, ec); }
        if (!previous_state.valid) { return Event::None; }
        if (previous_state.exists != now.exists) {
            if (now.exists) {
                return Event::Created;
            } else {
                return Event::Deleted;
            }
        }
        if (now.exists && previous_state.last_write_time != now.last_write_time) { return Event::Changed; }
        return Event::None;
    }();

    if (out_event) *out_event = evt;
    return now;
}


} // namespace jaws::util::file_observing
