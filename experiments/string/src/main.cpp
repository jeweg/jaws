#include "jaws/jaws.hpp"
#include "jaws/util/InstanceCounter.hpp"
#include <string>
#include <iostream>
#include <atomic>


#if 0
TODO: see stringliteral in vcpkg source code:

struct StringLiteral
{
    template<int N>
    constexpr StringLiteral(const char (&str)[N])
        : m_size(N - 1) /* -1 here accounts for the null byte at the end*/, m_cstr(str)
    {
    }

    constexpr const char* c_str() const { return m_cstr; }
    constexpr size_t size() const { return m_size; }

    operator CStringView() const { return m_cstr; }
    operator std::string() const { return m_cstr; }

private:
    size_t m_size;
    const char* m_cstr;
};

inline const char* to_printf_arg(const StringLiteral str) { return str.c_str(); }
#endif







#if 0
jaws::LoggerPtr logger;

namespace jaws {








class String
{
public:


    String(const char* ptr, std::size_t char_count)
    {
        if (!ptr || !char_count)
            _hash = 0;


        }

    }


    // Actual string interface

private:

    struct SharedData
    {
        char *_string;
        size_t _length;
        std::size_t _hash;
        std::atomic<int> _ref_count;
    };

    union {
        const char* _data_direct_chars;
        SharedData* _data_shared;
    };
    int _size;
    int _hash_value;


};

}


























// Here I'm experimenting with
// a string class.
// This heavily draws inspiration from the houdini cppcon 2018 talk.
// We want a string where copies share the same underlying chars
// which, btw, will be 8 bit. wchars are worse than utf-8 on chars.
// Copies of the sting share the data and derived information such as
// a hash value.
// the chars can come from two sources: static strings in which case
// we don't take ownership over the chars, but still have ref-counted derived data,
// and runtime data. In the latter case we take ownership and destroy the
// char array properly once it is (going to be) no longer referenced.
// This is not supposed to be a replacement for std::string, just an optimization
// for strings that are not changed.

// Holding the chars as a pointer + length has one drawback:
// if we actually need it as a std::string, the implementation must
// call strlen on it. We want to avoid that:
// if we hold the data as a std::string from the start, we can just
// serve that -- we should only hand out std::string if the caller actually
// intends to modify the string, though, otherwise we waste a copy.
// This leaves a question, though: how do we handle string literals?
// Maybe we can find an optmization here. For now let's try something.

// I suspect all this cn be done much better, but I need to have some own experiences, too.

// Now I've got a basic thing working, but what about string literals?
// compile-time hash computation is one thing, but can we also avoid string construction?
// This is of couse a trade-off -- holding the chars not as std::string might require multiple
// creations of std::string later on -- but if it's never needed as std::string, we wasted
// one creation.

class StringData : jaws::util::InstanceCounter<StringData>
{
public:
    explicit StringData(const char* chars)
    : _string(chars)
    {}
    explicit StringData(const char* chars, int length)
        : _string(chars, length)
    {}

    std::size_t GetHashValue() const {
        if (_hash_value == 0) {
             // Might want to replace this with another hash function.
             logger->info("==> Actual hash computation!");
            _hash_value = std::hash<std::string>{}(_string);
        }
        return _hash_value;
    }

    std::string _string;
    mutable std::size_t _hash_value = 0;
};


class ConstString : jaws::util::InstanceCounter<ConstString>
{
public:
    explicit ConstString(const char* chars)
    {
        _data = std::make_shared<StringData>(chars);
    }
    explicit ConstString(const char* chars, int length)
    {
        _data = std::make_shared<StringData>(chars, length);
    }

    ConstString(const ConstString&) = default;
    ConstString& operator=(const ConstString&) = default;

    ConstString(ConstString&&) = default;
    ConstString& operator=(ConstString&&) = default;

    std::size_t GetHashValue() const noexcept
    {
       return _data->GetHashValue();
    }

private:
    std::shared_ptr<StringData> _data;
};

namespace std {

template<>
struct hash<ConstString>
{
    std::size_t operator()(const ConstString &str) const noexcept {
        return str.GetHashValue();
    }
};

}

#define JAWS_CONST_STR(arg) ConstString(arg, xxhash::xxh64(arg, sizeof(arg), 0))

int main(int argc, char** argv)
{
    logger = jaws::GetLoggerPtr(jaws::Category::General);
    //logger->info("Hello, World!");

    {
        auto cs1 = ConstString("hello, world!");
        auto cs2 = cs1;
        auto cs3 = cs1;

        std::cout << std::hash<ConstString>{}(cs1) << "\n";
        std::cout << std::hash<ConstString>{}(cs2) << "\n";
        std::cout << std::hash<ConstString>{}(cs3) << "\n";
        std::cout << std::hash<ConstString>{}(cs2) << "\n";
    }

}
#endif
