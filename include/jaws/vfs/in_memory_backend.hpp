#pragma once
#include "jaws/core.hpp"
#include "jaws/vfs/vfs_backend.hpp"
#include <memory>

namespace jaws::vfs {

class JAWS_API InMemoryBackend : public VfsBackend
{
public:
    InMemoryBackend();
    ~InMemoryBackend();

    // Externally managed data; assumed to be valid until removed or until backend is destroyed.
    bool set_file_external(const Path &, uint8_t *bytes, size_t num_bytes);

    bool set_file(const Path &, const std::vector<uint8_t> &contents);
    // Data is copied and held internally
    bool set_file(const Path &, uint8_t *bytes, size_t num_bytes);

    bool forget_file(const Path &);

public:
    // VfsBackend implementation:

    bool is_file(const Path &) const override;
    bool is_dir(const Path &) const override;

    size_t get_file_size(const Path &) const override;
    uint64_t get_file_fingerprint(const Path &path, bool force_exact = false) const override;

    size_t read_full(const Path &, uint8_t *write_ptr, uint64_t *out_fingerprint = nullptr) const override;

    size_t read_bytes(const Path &, uint8_t *write_ptr, size_t offset = 0, size_t num_bytes = -1) const override;

    bool get_file_contents(
        const Path &, std::vector<uint8_t> *out_contents, size_t offset_bytes, size_t num_bytes) const override;

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

}; // namespace jaws::vfs
