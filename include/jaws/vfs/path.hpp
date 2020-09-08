#pragma once
#include "jaws/core.hpp"
#include "jaws/assume.hpp"
#include "jaws/util/hashing.hpp"
#include "absl/hash/hash.h"
#include "absl/strings/str_cat.h"
#include <fmt/format.h>
#include <string>
#include <array>

namespace jaws::vfs {

class JAWS_API Path
{
public:
    enum class Part
    {
        Domain = 0,
        Path,
        Parents,
        Filename,
        Stem,
        Extension
    };

    Path() = default;

    Path(const std::string &s);
    Path(const char *chars);

    Path(const Path &other);
    Path(Path &&other) noexcept;

    Path &operator=(const Path &p);
    Path &operator=(Path &&p) noexcept;

    Path get_parent_path() const;

    std::string_view get_domain() const { return get_part(Part::Domain); }
    std::string_view get_path() const { return get_part(Part::Path); }
    std::string_view get_parents() const { return get_part(Part::Parents); }
    std::string_view get_filename() const { return get_part(Part::Filename); }
    std::string_view get_stem() const { return get_part(Part::Stem); }
    std::string_view get_extension() const { return get_part(Part::Extension); }

    std::string_view get_part(Part part) const;

    bool is_absolute() const;
    bool is_relative() const;

    std::string string() const { return _str; }
    std::string_view string_view() const { return _str; }
    const char *c_str() const { return _str.c_str(); }

    friend bool operator==(const Path &l, const Path &r) { return l._str == r._str; }
    friend bool operator!=(const Path &l, const Path &r) { return !(l == r); }

    size_t get_hash_value() const
    {
        if (!_hash) { _hash = absl::Hash<std::string>{}(_str); }
        return _hash;
    }

    friend Path operator/(const Path &l, const Path &r);

    template <typename H>
    friend H AbslHashValue(H h, const Path &p)
    {
        return H::combine(std::move(h), jaws::util::Prehashed(p.get_hash_value()));
    }

private:
    void parse() const;
    void set_parsed(bool b) const noexcept;
    bool is_parsed() const noexcept;

    std::string _str;
    mutable size_t _hash = 0;
    // bit 0: already parsed? bit 1: absolute path?
    mutable uint8_t _flags = 0;
    mutable std::array<std::string_view, 6> _parts;
};


}

namespace std {

template <>
struct hash<jaws::vfs::Path>
{
    size_t operator()(const jaws::vfs::Path &p) const { return p.get_hash_value(); }
};

}

template <>
struct fmt::formatter<jaws::vfs::Path> : formatter<absl::string_view>
{
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const jaws::vfs::Path &p, FormatContext &ctx)
    {
        return formatter<absl::string_view>::format(p.string_view(), ctx);
    }
};
