#pragma once
#include "jaws/core.hpp"
#include "jaws/filesystem.hpp"
#include <memory>
#include <limits>

// https://stackoverflow.com/questions/54024915/how-to-monitor-file-changes-via-win-api

namespace jaws::util {

namespace detail {
class FileObserverImpl;
}

class JAWS_API FileObserver
{
public:
    FileObserver();
    ~FileObserver();

    using Handle = std::uint64_t;
    static constexpr Handle INVALID_HANDLE = std::numeric_limits<std::uint64_t>::max();

    Handle AddObservedFile(const std::filesystem::path &);
    bool RemoveObservedFile(Handle);

    enum class Event
    {
        Changed,
        Created,
        Deleted
    };

    struct Result
    {
        Handle handle;
        std::filesystem::path canon_path;
        Event event;
    };

    std::vector<Result> Poll();

private:
    std::unique_ptr<detail::FileObserverImpl> _impl;
};

} // namespace jaws::util
