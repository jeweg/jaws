#pragma once
#include "jaws/util/file_observer.hpp"
#include "jaws/filesystem.hpp"
#include <functional>
#include "absl/container/flat_hash_map.h"
#include "absl/hash/hash.h"

// This is the naive implementation. Here are pointers to potentially more efficient ways:
// https://stackoverflow.com/questions/931093/how-do-i-make-my-program-watch-for-file-modification-in-c
// https://stackoverflow.com/questions/54024915/how-to-monitor-file-changes-via-win-api

namespace fs = std::filesystem;

namespace std {
template <>
struct hash<fs::path>
{
    size_t operator()(std::filesystem::path const &s) const noexcept { return std::hash<std::string>{}(s.string()); }
};
}

namespace jaws::util::detail {

struct FileStatus
{
    explicit FileStatus(const fs::path &path, FileObserver::Handle handle) :
        canon_file_path(path), handle(handle), existed(fs::exists(path))
    {
        if (existed) { last_write_time = fs::last_write_time(path); }
    }

    fs::path canon_file_path;
    FileObserver::Handle handle;
    bool existed = false;
    fs::file_time_type last_write_time;
};


class FileObserverImpl
{
    absl::flat_hash_map<FileObserver::Handle, FileStatus> file_status_map;

    static FileObserver::Handle GetHandleForPath(const fs::path &path)
    {
        return static_cast<FileObserver::Handle>(absl::Hash<std::string>{}(path.string()));
    }

public:
    FileObserver::Handle AddObservedFile(const fs::path &file_path)
    {
        fs::path canon_file_path = fs::canonical(file_path);
        FileObserver::Handle handle = GetHandleForPath(canon_file_path);
        auto iter = file_status_map.find(handle);
        if (iter != file_status_map.end()) {
            // Already observing this file, possibly under a different name.
            return handle;
        }
        file_status_map.insert(std::make_pair(handle, FileStatus(canon_file_path, handle)));
        return handle;
    }


    bool RemoveObservedFile(FileObserver::Handle handle)
    {
        auto iter = file_status_map.find(handle);
        if (iter == file_status_map.end()) {
            return false;
        } else {
            file_status_map.erase(iter);
            return true;
        }
    }


    std::vector<FileObserver::Result> Poll()
    {
        std::vector<FileObserver::Result> results;
        for (auto &iter : file_status_map) {
            FileStatus &file_status = iter.second;
            bool exists_now = fs::exists(file_status.canon_file_path);
            if (file_status.existed != exists_now) {
                results.emplace_back(FileObserver::Result{
                    file_status.handle,
                    file_status.canon_file_path,
                    exists_now ? FileObserver::Event::Created : FileObserver::Event::Deleted});
                file_status.existed = !file_status.existed;
            } else {
                auto last_write_time_now = fs::last_write_time(file_status.canon_file_path);
                if (last_write_time_now != file_status.last_write_time) {
                    results.emplace_back(FileObserver::Result{
                        file_status.handle, file_status.canon_file_path, FileObserver::Event::Changed});
                    file_status.last_write_time = last_write_time_now;
                }
            }
        }

        return results;
    }
};

}
