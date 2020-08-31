#pragma once
#include "jaws/vfs/path.hpp"
#include "jaws/vfs/vfs_backend.hpp"
#include "jaws/util/lru_cache.hpp"
#include "jaws/util/timer.hpp"
#include "absl/container/flat_hash_map.h"
#include <string>
#include <memory>

namespace jaws::vfs {

class JAWS_API Vfs
{
public:
    uint64_t get_fingerprint(const jaws::vfs::Path &path, bool allow_cached = true) const;

    std::string read_text_file(const Path &path, uint64_t *out_fingerprint = nullptr) const;

    Path make_canonical(Path path) const;

    //-------------------------------------------------------------------------
    // Backend management

    template <typename T, typename... Args>
    void make_backend(const std::string &domain, Args &&... args);

    void add_backend(const std::string &domain, std::unique_ptr<vfs::VfsBackend>);
    bool remove_backend(const std::string &domain);

    const vfs::VfsBackend *get_backend(const std::string &domain) const;
    const vfs::VfsBackend *lookup_backend(const jaws::vfs::Path &path) const;

private:
    // Map of named backends
    absl::flat_hash_map<std::string, std::unique_ptr<vfs::VfsBackend>> _vfs_backends;
};

template <typename T, typename... Args>
void Vfs::make_backend(const std::string &domain, Args &&... args)
{
    add_backend(domain, std::make_unique<T>(std::forward<Args>(args)...));
}

}
