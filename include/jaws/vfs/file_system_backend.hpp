#pragma once
#include "jaws/core.hpp"
#include "jaws/vfs/vfs_backend.hpp"
#include "jaws/filesystem.hpp"

namespace jaws::vfs {

class JAWS_API FileSystemBackend : public VfsBackend
{
public:
    FileSystemBackend(const std::filesystem::path &root);

    bool is_file(const Path &) const override;
    bool is_dir(const Path &) const override;

    size_t get_file_size(const Path &) const override;
    uint64_t get_file_fingerprint(const Path &path, bool force_exact = false) const override;

    size_t read_full(const Path &, uint8_t *write_ptr, uint64_t *out_fingerprint = nullptr) const override;

    size_t read_bytes(const Path &, uint8_t *write_ptr, size_t offset = 0, size_t num_bytes = -1) const override;

    bool get_file_contents(
        const Path &, std::vector<uint8_t> *out_contents, size_t offset_bytes, size_t num_bytes) const override;

private:
    bool get_file_contents(
        const std::filesystem::path &p,
        std::vector<uint8_t> *out_contents,
        size_t offset_bytes,
        size_t num_bytes) const;
    std::filesystem::path to_actual_path(const Path &path) const;
    std::filesystem::path _root;
};

}; // namespace jaws::vfs
