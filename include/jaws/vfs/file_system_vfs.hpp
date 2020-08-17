#pragma once
#include "jaws/core.hpp"
#include "jaws/vfs/vfs_backend.hpp"
#include "jaws/filesystem.hpp"

namespace jaws::vfs {
/*
virtual bool is_file(const Path&) const = 0;
virtual bool is_dir(const Path&) const = 0;

virtual size_t get_file_size(const Path&) const = 0;
virtual uint64_t get_file_fingerprint(const Path& path) = 0;

virtual bool get_file_contents(const Path&, std::vector<uint8_t>*) const = 0;
virtual std::string get_file_contents(const Path& path) const
{
    std::vector<uint8_t> bytes;
    get_file_contents(path, &bytes);
    static_assert(sizeof(char) == sizeof(uint8_t));
    return std::string(reinterpret_cast<char*>(&bytes[0]), bytes.size());
}
 */

class JAWS_API FileSystemVfs : public VfsBackend
{
public:
    FileSystemVfs(const std::filesystem::path &root);

    bool is_file(const Path &) const override;
    bool is_dir(const Path &) const override;

    size_t get_file_size(const Path &) const override;
    size_t get_file_fingerprint(const Path &path, bool force_exact = false) const override;

    size_t read_full(const Path &, uint8_t *write_ptr, size_t *out_fingerprint = nullptr) const override;

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
