#include "pch.hpp"
#include "jaws/vfs/vfs.hpp"
#include "absl/strings/str_cat.h"

namespace jaws::vfs {

namespace detail {

// A proper domain name ends with a ":" because of symmetry with
// Path::get_domain. We fix this here.
std::string fixed_domain(const std::string &d)
{
    if (!d.empty() && d[d.size() - 1] != ':') { return d + ":"; }
    return d;
}

} // namespace detail

Vfs::Vfs() : _fingerprint_cache(1000, 100) {}


void Vfs::add_backend(const std::string &domain, std::unique_ptr<vfs::VfsBackend> ptr)
{
    _vfs_backends.insert_or_assign(detail::fixed_domain(domain), std::move(ptr));
}


bool Vfs::remove_backend(const std::string &domain)
{
    return _vfs_backends.erase(detail::fixed_domain(domain)) == 1;
}


const vfs::VfsBackend *Vfs::get_backend(const std::string &name) const
{
    auto iter = _vfs_backends.find(name);
    if (iter == _vfs_backends.end()) { return nullptr; }
    return iter->second.get();
}


const vfs::VfsBackend *Vfs::lookup_backend(const jaws::vfs::Path &path) const
{
    auto domain = path.get_domain();
    if (domain.empty()) { return nullptr; }
    auto iter = _vfs_backends.find(domain);
    if (iter == _vfs_backends.end()) { return nullptr; }
    return iter->second.get();
}


size_t Vfs::get_fingerprint(const jaws::vfs::Path &path, bool allow_cached) const
{
    // TODO: what was I thinking here?
#if 0
    constexpr uint64_t MAX_CACHE_REALTIME_AGE_MS = 500;
    auto now = jaws::util::get_time_point();
    if (allow_cached) {
        _fingerprint_cache.tick_clock();
        if (CachedFingerprint *cached = _fingerprint_cache.lookup_unless(path, [&](auto, CachedFingerprint *value) {
                return util::get_delta_time_ms(now, value->updated_time_point) <= MAX_CACHE_REALTIME_AGE_MS;
            })) {
            return cached->fingerprint;
        }
    }
#endif
    // We must actually compute the fingerprint.
    if (const VfsBackend *backend = lookup_backend(path)) {
        size_t fp = backend->get_file_fingerprint(path);
        //_fingerprint_cache.insert(path, CachedFingerprint{fp, now});
        return fp;
    }
    return 0;
}


std::string Vfs::read_text_file(const Path &path, size_t *out_fingerprint) const
{
    if (const VfsBackend *backend = lookup_backend(path)) {
        if (backend->is_file(path)) {
            size_t size = backend->get_file_size(path);
            std::string str(size, '\0');
            if (backend->read_full(path, reinterpret_cast<uint8_t *>(&str[0]), out_fingerprint) == 0) { return {}; }
            return str;
        }
    }
    return {};
}


Path Vfs::make_canonical(Path path) const
{
    // Fix domain if not present
    std::string domain(path.get_domain());
    if (domain.empty()) {
        std::string backend_name;
        for (const auto &kv : _vfs_backends) {
            if (kv.second->is_file(path) || kv.second->is_dir(path)) {
                backend_name = kv.first;
                break;
            }
        }
        if (!backend_name.empty()) { path = Path(absl::StrCat(backend_name, path.get_path())); }
    }
    return path;
}


} // namespace jaws::vfs
