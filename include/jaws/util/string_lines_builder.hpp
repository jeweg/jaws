#pragma once

#include "fmt/format.h"
#include "absl/strings/str_cat.h"
#include <string>
#include "jaws/util/type_traits.hpp"

namespace jaws::util {

class StringLinesBuilder
{
public:
    class IndentGuard
    {
    public:
        IndentGuard(StringLinesBuilder &sb, int delta_depth) : _sb(sb), _delta_depth(delta_depth)
        {
            _sb.indent(_delta_depth);
        }
        ~IndentGuard() { _sb.indent(-_delta_depth); }

    private:
        StringLinesBuilder &_sb;
        int _delta_depth;
    };

    StringLinesBuilder(const std::string indent_fill_step = "    ") : StringLinesBuilder(0, indent_fill_step) {}

    StringLinesBuilder(int indent_depth, const std::string indent_fill_step = "    ") :
        _indent_depth(indent_depth), _indent_fill_step(indent_fill_step)
    {}

    StringLinesBuilder &append()
    {
        absl::StrAppend(&_accum_string, "\n");
        return *this;
    }

    StringLinesBuilder &append(const std::string &line)
    {
        if (line.empty()) {
            return append();
        } else {
            for (int i = 0; i < _indent_depth; ++i) { absl::StrAppend(&_accum_string, _indent_fill_step); }
            if (line[line.size() - 1] != '\n') {
                absl::StrAppend(&_accum_string, line, "\n");
            } else {
                absl::StrAppend(&_accum_string, line);
            }
        }
        return *this;
    }

    // Just for the convenience of dropping "fmt::format(" really.
    template <typename S, typename... Args>
    StringLinesBuilder &append_format(const S &format_str, const Args &... args)
    {
        return append(fmt::format(format_str, args...));
    }

    std::string str() const { return _accum_string; }

    void indent(int delta_depth = 1) { _indent_depth += delta_depth; }

    IndentGuard indent_guard(int delta_depth = 1)
    {
        // Not sure right now.
        // return std::move(IndentGuard(*this, delta_depth));
        return IndentGuard(*this, delta_depth);
    }


private:
    std::string _accum_string;
    int _indent_depth = 0;
    std::string _indent_fill_step;
};

} // namespace jaws::util
