#include "pch.hpp"
#include "jaws/util/file_observing.hpp"
#include "jaws/filesystem.hpp"

namespace fs = std::filesystem;

namespace jaws::util::file_observing {

Event get_file_event(const std::filesystem::path& path, State& in_out_state)
{
    std::error_code ec;
    State now;
    now.exists = fs::exists(path, ec);
    if (now.exists) { now.last_write_time = fs::last_write_time(path, ec); }

    Event evt = [&]() {
        if (!in_out_state.valid) {
            if (now.exists) { return Event::Created; }
            return Event::None;
        }
        if (in_out_state.exists != now.exists) {
            if (now.exists) {
                return Event::Created;
            } else {
                return Event::Deleted;
            }
        }
        if (now.exists && in_out_state.last_write_time != now.last_write_time) { return Event::Changed; }
        return Event::None;
    }();

    in_out_state = std::move(now);
    return evt;
}


} // namespace jaws::util::file_observing
