#include "jaws/vfs/file_system_vfs.hpp"
#include "absl/hash/hash.h"
#include "xxhash.h"

#include <cstdio>

namespace fs = std::filesystem;

namespace jaws::vfs {

namespace detail {

// RAII
struct FileHandle
{
    FILE *handle = nullptr;
    FileHandle(const fs::path &path) { handle = fopen(path.string().c_str(), "r"); }
    ~FileHandle()
    {
        if (handle) { fclose(handle); }
    }
};

} // namespace detail

FileSystemVfs::FileSystemVfs(const std::filesystem::path &root) : _root(root) {}


fs::path FileSystemVfs::to_actual_path(const Path &path) const
{
    fs::path fs_path = _root / fs::path(path.get_path());
    while (fs::is_symlink(fs_path)) { fs_path = fs::read_symlink(fs_path); }
    return fs_path;
}


bool FileSystemVfs::is_file(const Path &path) const
{
    std::error_code ec;
    bool r = fs::is_regular_file(to_actual_path(path), ec);
    if (ec) return false;
    return r;
}


bool FileSystemVfs::is_dir(const Path &path) const
{
    std::error_code ec;
    bool r = fs::is_directory(to_actual_path(path));
    if (ec) return false;
    return r;
}


size_t FileSystemVfs::get_file_size(const Path &path) const
{
    std::error_code ec;
    size_t r = fs::file_size(to_actual_path(path));
    if (ec) return false;
    return r;
}


size_t FileSystemVfs::read_full(const Path &path, uint8_t *write_ptr, size_t *out_fingerprint) const
{
    fs::path p = to_actual_path(path);
    detail::FileHandle fh(p);
    if (!fh.handle) { return 0; }
    std::fseek(fh.handle, 0, SEEK_END);
    size_t size = std::ftell(fh.handle);
    std::fseek(fh.handle, 0, SEEK_SET);
    size_t read_bytes = std::fread(write_ptr, 1, size, fh.handle);
    if (out_fingerprint && read_bytes > 0) { *out_fingerprint = XXH64(write_ptr, read_bytes, 0); }
    return read_bytes;
}


size_t FileSystemVfs::read_bytes(const Path &path, uint8_t *write_ptr, size_t offset, size_t num_bytes) const
{
    fs::path p = to_actual_path(path);
    detail::FileHandle fh(p);
    if (!fh.handle) { return 0; }

    std::fseek(fh.handle, 0, SEEK_END);
    size_t file_size = std::ftell(fh.handle);
    if (offset >= file_size) { return 0; }
    std::fseek(fh.handle, offset, SEEK_SET);

    size_t to_read = std::min(file_size - offset, num_bytes);
    return std::fread(write_ptr, 1, to_read, fh.handle);
}

// TODO: clean up the other read functions, use the RAII class, think about useful overloads


bool FileSystemVfs::get_file_contents(
    const Path &path, std::vector<uint8_t> *out_contents, size_t offset_bytes, size_t num_bytes) const
{
    fs::path p = to_actual_path(path);
    return get_file_contents(p, out_contents, offset_bytes, num_bytes);
}


bool FileSystemVfs::get_file_contents(
    const std::filesystem::path &p, std::vector<uint8_t> *out_contents, size_t offset_bytes, size_t num_bytes) const
{
    FILE *handle = fopen(p.string().c_str(), "r");
    if (!handle) { return false; }
    std::fseek(handle, 0, SEEK_END);
    size_t file_size = std::ftell(handle);
    if (offset_bytes >= file_size) { return false; }
    std::fseek(handle, offset_bytes, SEEK_SET);
    std::rewind(handle);
    size_t to_read = std::min(file_size - offset_bytes, num_bytes);
    out_contents->reserve(to_read);
    if (out_contents->size() < to_read) { out_contents->resize(to_read); }
    size_t read_bytes = std::fread(&(*out_contents)[0], 1, to_read, handle);
    std::fclose(handle);
    return true;
}

size_t FileSystemVfs::get_file_fingerprint(const Path &path, bool force_exact) const
{
    constexpr size_t MAX_BYTES_TO_HASH = 1024 * 1024 * 1; // 1 MiB

    std::error_code ec;
    fs::path p = to_actual_path(path);
    size_t file_size = fs::file_size(p, ec);
    if (ec) return 0;
    size_t to_hash = force_exact ? file_size : std::min(MAX_BYTES_TO_HASH, file_size);

    static std::vector<uint8_t> contents;
    contents.clear();
    bool ok = get_file_contents(p, &contents, 0, to_hash);
    if (!ok) return 0;

    size_t hash_value = XXH64(contents.data(), to_hash, 0);

    // If not all content went into the hash, we also combine the last write time and file size into the hash.
    if (to_hash < file_size) {
        auto lwt = fs::last_write_time(p, ec);
        hash_value = jaws::util::combined_hash(jaws::util::Prehashed(hash_value), file_size /*, lwt*/);
    }
    return hash_value;
}

} // namespace jaws::vfs
