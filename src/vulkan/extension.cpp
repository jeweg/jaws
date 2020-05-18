#include "pch.hpp"
#include "jaws/vulkan/extension.hpp"
#include "jaws/assume.hpp"
#include "to_string.hpp"

#include <sstream>

namespace jaws::vulkan {

ExtensionList::ExtensionList(std::initializer_list<Extension> es)
{
    for (const auto& e : es) { add(e); }
}

ExtensionList::ExtensionList(const char** names, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) { add(Extension(names[i])); }
}


ExtensionList& ExtensionList::add(Extension e)
{
    JAWS_ASSUME(!e.name.empty());

    auto iter = _exts.find(e.name);
    if (iter != _exts.end()) {
        if (e.version != 0) {
            if (iter->second.version == 0 || e.version > iter->second.version) {
                // e's version wins.
                iter->second.version = e.version;
            }
        }
    } else {
        std::string e_name = e.name;
        _exts.insert(std::make_pair(e_name, std::move(e)));
    }
    return *this;
}


ExtensionList& ExtensionList::add(const ExtensionList& el)
{
    for (const auto& e : el) { add(e); }
    return *this;
}


bool ExtensionList::contains(const Extension& e) const
{
    JAWS_ASSUME(!e.name.empty());

    auto iter = _exts.find(e.name);
    if (iter != _exts.end()) {
        if (iter->second.version == 0 || e.version == 0 || e.version <= iter->second.version) { return true; }
    }
    return false;
}


ExtensionList operator-(const ExtensionList& as, const ExtensionList& bs)
{
    ExtensionList leftover;
    if (!as.empty()) {
        for (const auto& a : as) {
            if (!bs.contains(a)) { leftover.add(a); }
        }
    }
    return leftover;
}


bool ExtensionList::contains(const ExtensionList& el) const
{
    return (el - *this).empty();
}


std::vector<const char*> ExtensionList::as_char_ptrs() const
{
    std::vector<const char*> r;
    r.reserve(_exts.size());
    for (const auto& e : _exts) { r.push_back(e.second.name.c_str()); }
    return r;
}


ExtensionList::ConstIterator ExtensionList::cbegin() const
{
    return ConstIterator(_exts.cbegin());
}


ExtensionList::ConstIterator ExtensionList::cend() const
{
    return ConstIterator(_exts.cend());
}


ExtensionList::ConstIterator ExtensionList::begin() const
{
    return cbegin();
}


ExtensionList::ConstIterator ExtensionList::end() const
{
    return cend();
}


std::string ExtensionList::to_string(bool verbose, const std::string& separator) const
{
    std::ostringstream oss;
    bool first = true;
    for (const auto& e : _exts) {
        if (!first) { oss << ", "; }
        oss << e.second.name;
        if (verbose && e.second.version != 0) { oss << " (" << version_to_string(e.second.version) << ")"; }
        first = false;
    }
    return oss.str();
}


ExtensionList ExtensionList::resolve(
    const ExtensionList& available,
    const ExtensionList& required,
    const ExtensionList& optional,
    std::string* out_error_msg)
{
    JAWS_ASSUME(out_error_msg);
    ExtensionList missing_req = required - available;
    if (!missing_req.empty()) {
        *out_error_msg = "missing: " + missing_req.to_string(true);
        return {};
    };

    ExtensionList r = required;
    for (const auto& e : optional) {
        if (!r.contains(e) && available.contains(e)) { r.add(e); }
    }
    return r;
}

} // namespace jaws::vulkan
