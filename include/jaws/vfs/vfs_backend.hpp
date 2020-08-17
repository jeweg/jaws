#pragma once
#include "jaws/core.hpp"
#include "jaws/vfs/path.hpp"
#include "jaws/util/hashing.hpp"
#include "absl/hash/hash.h"
#include <string>
#include <vector>

namespace jaws::vfs {

/// Interface of a simple read-only file system.
class VfsBackend
{
public:
    virtual ~VfsBackend() = default;

    virtual bool is_file(const Path &) const = 0;
    virtual bool is_dir(const Path &) const = 0;

    virtual size_t get_file_size(const Path &) const = 0;
    virtual size_t get_file_fingerprint(const Path &path, bool force_exact = false) const = 0;

    // The fingerprint output makes sense here b/c we read the whole file anyway. This is not true
    // for the region-limited version below.
    virtual size_t read_full(const Path &, uint8_t *write_ptr, size_t *out_fingerprint = nullptr) const = 0;

    virtual size_t read_bytes(const Path &, uint8_t *write_ptr, size_t offset = 0, size_t num_bytes = -1) const = 0;

    virtual bool get_file_contents(
        const Path &, std::vector<uint8_t> *out_contents, size_t offset_bytes = 0, size_t num_bytes = -1) const = 0;

    virtual std::vector<uint8_t> get_file_contents(const Path &path) const
    {
        std::vector<uint8_t> bytes;
        get_file_contents(path, &bytes);
        static_assert(sizeof(char) == sizeof(uint8_t));
        return bytes;
    }
};

}; // namespace jaws::vfs
