#pragma once
#include "jaws/core.hpp"

#include "absl/container/flat_hash_map.h"
#include <string>
#include <vector>
#include <initializer_list>

namespace jaws::vulkan {

struct JAWS_API Extension
{
    std::string name;
    uint32_t version = 0;

    Extension(const char *name, uint32_t version = 0) : name(name), version(version) {}
    Extension(std::string name, uint32_t version = 0) : name(std::move(name)), version(version) {}
};

class JAWS_API ExtensionList
{
    using MapType = absl::flat_hash_map<std::string, Extension>;

public:
    ExtensionList() = default;
    ExtensionList(std::initializer_list<Extension>);
    ExtensionList(const char **, uint32_t);

    ExtensionList &add(Extension);
    ExtensionList &add(const ExtensionList &);
    bool contains(const Extension &) const;
    bool contains(const ExtensionList &) const;

    void clear() { _exts.clear(); }
    size_t size() const { return _exts.size(); }
    bool empty() const { return _exts.empty(); }

    // Careful, don't keep this around, it will not necessarily
    // survive another operation on this instance.
    std::vector<const char *> as_char_ptrs() const;

    friend ExtensionList operator-(const ExtensionList &, const ExtensionList &);

    static ExtensionList resolve(
        const ExtensionList &available,
        const ExtensionList &required,
        const ExtensionList &optional,
        std::string *out_error_msg);

    std::string to_string(bool verbose, const std::string &separator = ", ") const;

    class ConstIterator
    {
    public:
        friend bool operator==(const ConstIterator &lhs, const ConstIterator &rhs) { return lhs._iter == rhs._iter; }
        friend bool operator!=(const ConstIterator &lhs, const ConstIterator &rhs) { return !(lhs == rhs); }
        const Extension &operator*() { return _iter->second; }
        auto operator++()
        {
            ++_iter;
            return *this;
        }
        auto operator++(int)
        {
            auto before(*this);
            operator++();
            return before;
        }

    private:
        friend class ExtensionList;
        explicit ConstIterator(MapType::const_iterator iter) : _iter(iter) {}

        MapType::const_iterator _iter;
    };

    ConstIterator cbegin() const;
    ConstIterator cend() const;
    ConstIterator begin() const;
    ConstIterator end() const;

private:
    MapType _exts;
};

}
