#include "jaws/vfs/in_memory_backend.hpp"
#include "jaws/assume.hpp"
#include "absl/container/flat_hash_map.h"
#include "xxhash.h"
#include <optional>
//#include "absl/hash/hash.h"
//#include "xxhash.h"

namespace jaws::vfs {

struct InMemoryBackend::Impl
{
    struct File
    {
        const uint8_t *bytes = nullptr;
        size_t num_bytes = -1;
        bool owned = false;
        mutable std::optional<uint64_t> fingerprint;

        File() noexcept = default;

        File(const uint8_t *bytes, size_t num_bytes, bool owned) : owned(owned)
        {
            JAWS_ASSUME(bytes && num_bytes > 0);
            if (this->owned) {
                // Make a copy
                this->bytes = new uint8_t[num_bytes];
                std::memcpy(const_cast<uint8_t *>(this->bytes), bytes, num_bytes);
            } else {
                this->bytes = bytes;
                this->num_bytes = num_bytes;
            }
        }

        ~File()
        {
            if (owned) { delete[] bytes; }
        }

        uint64_t get_fingerprint() const
        {
            if (!fingerprint) { fingerprint = XXH64(bytes, num_bytes, 0); }
            return fingerprint.value();
        }
    };

    using FilePtr = std::unique_ptr<File>;
    absl::flat_hash_map<vfs::Path, FilePtr> _hash_map;

    File *get_file(const Path &path)
    {
        auto iter = _hash_map.find(path);
        if (iter == _hash_map.end()) { return nullptr; }
        return iter->second.get();
    }

    const File *get_file(const Path &path) const
    {
        auto iter = _hash_map.find(path);
        if (iter == _hash_map.end()) { return nullptr; }
        return iter->second.get();
    }


    bool set_file(const Path &path, const uint8_t *bytes, size_t num_bytes, bool owned)
    {
        if (!bytes) { return false; }
        _hash_map.insert(std::make_pair(path, std::make_unique<File>(bytes, num_bytes, owned)));
    }

    bool forget_file(const Path &path)
    {
        if (File *f = get_file(path)) {
            _hash_map.erase(path);
            return true;
        }
        return true;
    }
};


InMemoryBackend::InMemoryBackend() : _impl(std::make_unique<Impl>()) {}

InMemoryBackend::~InMemoryBackend() = default;

bool InMemoryBackend::set_file_external(const Path &path, uint8_t *bytes, size_t num_bytes)
{
    return _impl->set_file(path, bytes, num_bytes, false);
}


bool InMemoryBackend::set_file(const Path &path, const std::vector<uint8_t> &contents)
{
    return _impl->set_file(path, contents.data(), contents.size(), true);
}


bool InMemoryBackend::set_file(const Path &path, uint8_t *bytes, size_t num_bytes)
{
    return _impl->set_file(path, bytes, num_bytes, true);
}


bool InMemoryBackend::forget_file(const Path &path)
{
    return _impl->forget_file(path);
}


bool InMemoryBackend::is_file(const Path &path) const
{
    return _impl->get_file(path) != nullptr;
}


bool InMemoryBackend::is_dir(const Path &path) const
{
    // TOOO: consider dropping is_dir from the interface and backends.
    return false;
}


size_t InMemoryBackend::get_file_size(const Path &path) const
{
    if (Impl::File *f = _impl->get_file(path)) { return f->num_bytes; }
    return -1;
}


uint64_t InMemoryBackend::get_file_fingerprint(const Path &path, bool force_exact) const
{
    if (Impl::File *f = _impl->get_file(path)) { return f->get_fingerprint(); }
    return -1;
}


size_t InMemoryBackend::read_full(const Path &path, uint8_t *write_ptr, uint64_t *out_fingerprint) const
{
    return {};
}


size_t InMemoryBackend::read_bytes(const Path &path, uint8_t *write_ptr, size_t offset, size_t num_bytes) const
{
    return {};
}


bool InMemoryBackend::get_file_contents(
    const Path &path, std::vector<uint8_t> *out_contents, size_t offset_bytes, size_t num_bytes) const
{
    return {};
}


}
