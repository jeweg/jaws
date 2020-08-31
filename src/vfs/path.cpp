#include "pch.hpp"
#include "jaws/vfs/path.hpp"

namespace jaws::vfs {

Path::Path(const std::string &s) : _str(s) {}


Path::Path(const char *chars) : _str(chars) {}


Path::Path(const Path &other) : _str(other._str), _hash(other._hash) {}


Path::Path(Path &&other) noexcept : _str(std::move(other._str)), _hash(other._hash) {}


Path &Path::operator=(const Path &p)
{
    _str = p._str;
    _hash = p._hash;
    set_parsed(false);
    return *this;
}


Path &Path::operator=(Path &&p) noexcept
{
    _str = std::move(p._str);
    _hash = p._hash;
    set_parsed(false);
    return *this;
}


Path Path::get_parent_path() const
{
    auto parents = get_part(Part::Parents);
    // Remove trailing slash from parents if that slash isn't the only char.
    if (!parents.empty() && !(parents.size() == 1 && parents[0] == '/')) { parents.remove_suffix(1); }
    return Path(absl::StrCat(get_part(Part::Domain), parents));
}


std::string_view Path::get_part(Part part) const
{
    JAWS_ASSUME(static_cast<size_t>(part) < _parts.size());
    parse();
    return _parts[static_cast<size_t>(part)];
}


bool Path::is_absolute() const
{
    parse();
    return _flags & 2u;
}


bool Path::is_relative() const
{
    return !is_absolute();
}


Path operator/(const Path &l, const Path &r)
{
    // We ignore the domain of r. If it isn't the same, the resulting path
    // doesn't make much sense, but that's on the user.
    // Compare with std::filesystem::Path's behaviour:
    // fs::path("d:/foo) / fs::path("f:/bar") // -> "d:/foo/f:/bar"
    std::string_view lp = l.get_path();
    std::string_view rp = r.get_path();
    if (rp.empty()) { return l; }
    if (lp.empty()) { return Path(absl::StrCat(l.get_domain(), rp)); }
    bool l_ends_with_slash = lp[lp.size() - 1] == '/';
    bool r_starts_with_slash = rp[0] == '/';
    if (l_ends_with_slash && r_starts_with_slash) {
        rp.remove_prefix(1);
        return Path(absl::StrCat(l.string_view(), rp));
    } else if (!l_ends_with_slash && !r_starts_with_slash) {
        return Path(absl::StrCat(l.string_view(), "/", rp));
    } else {
        return Path(absl::StrCat(l.string_view(), rp));
    }
}


void Path::parse() const
{
    if (is_parsed()) { return; }

    for (auto &part : _parts) { part = {}; };
    static constexpr auto np = std::string::npos;
    const size_t size = _str.size();
    if (size > 0) {
        const char *chs = _str.c_str();
        auto set_part_from_range = [this, chs, size](Path::Part part, size_t pos_begin, size_t pos_end_excl) {
            if (pos_begin != np && pos_end_excl != np && pos_begin < size && pos_end_excl > pos_begin) {
                _parts[static_cast<size_t>(part)] = std::string_view(&chs[pos_begin], pos_end_excl - pos_begin);
            }
        };

        /* This is
            const size_t pos_colon = _str.rfind(':');
            const size_t pos_last_slash = _str.rfind('/');
            const size_t pos_last_dot = _str.rfind('.');
        done manually in one loop: */
        size_t pos_colon = np;
        size_t pos_last_slash = np;
        size_t pos_last_dot = np;
        uint8_t num_set = 0;
        for (size_t p = size - 1;;) {
            const char ch = chs[p];
            if (ch == ':' && pos_colon == np) {
                pos_colon = p;
                ++num_set;
            } else if (ch == '/' && pos_last_slash == np) {
                pos_last_slash = p;
                ++num_set;
            } else if (ch == '.' && pos_last_dot == np) {
                pos_last_dot = p;
                ++num_set;
            }
            if (num_set == 3 || p == 0) { break; }
            --p;
        }

        if (pos_colon != np) { set_part_from_range(Part::Domain, 0, pos_colon + 1); }

        const size_t pos_path_begin = pos_colon == np ? 0 : pos_colon + 1;
        set_part_from_range(Part::Path, pos_path_begin, size);
        set_part_from_range(Part::Parents, pos_path_begin, pos_last_slash + 1);

        const size_t pos_filename_begin = std::max(pos_path_begin, pos_last_slash + 1);
        set_part_from_range(Part::Filename, pos_filename_begin, size);
        if (pos_last_dot != np && (pos_last_slash == np || pos_last_slash < pos_last_dot)) {
            // Extension present.
            set_part_from_range(Part::Stem, pos_filename_begin, pos_last_dot);
            set_part_from_range(Part::Extension, pos_last_dot, size);
        } else {
            // No extension => stem is full filename
            _parts[static_cast<size_t>(Part::Stem)] = _parts[static_cast<size_t>(Part::Filename)];
        }
        if (pos_path_begin < size && chs[pos_path_begin] == '/') {
            // Mark as absolute path.
            _flags |= 2u;
        }
    }
    set_parsed(true);
}


void Path::set_parsed(bool b) const noexcept
{
    if (b) {
        _flags |= 1u;
    } else {
        _flags &= ~1u;
    }
}


bool Path::is_parsed() const noexcept
{
    return _flags & 1u;
}


}
