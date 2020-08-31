#pragma once
#include "jaws/core.hpp"
#include "jaws/logging.hpp"

#include <string>

namespace jaws::util {

class Indentation
{
    static constexpr const char DefaultIndentStep[] = "    ";

public:
    // We interpret the empty string as the default increment step here.
    // This saves us from repeating the default everywhhere.
    // Also it makes little sense to have a class called "Indentation" do no
    // indentation by default, so I don't think it's a confusing interpretation of the default.
    Indentation(const int initial_indent_level = 0, const std::string_view indent_step = {}) :
        _indent_level(initial_indent_level), _indent_step(indent_step)
    {
        if (_indent_step.empty()) { _indent_step = DefaultIndentStep; }
    }

    void ChangeLevel(int delta)
    {
        if (delta != 0) {
            _indent_level += delta;
            _cached_indent_dirty = true;
        }
    }

    std::string Get() const
    {
        if (_cached_indent_dirty) {
            _cached_indent = "";
            for (int i = 0; i < _indent_level; ++i) { _cached_indent += _indent_step; }
        }
        return _cached_indent;
    }

public:
    struct Scoped
    {
        Scoped(Indentation &indentation, int delta = 1) : _indentation(indentation), _delta(delta)
        {
            _indentation.ChangeLevel(_delta);
        }

        ~Scoped() { _indentation.ChangeLevel(-_delta); }

        Scoped(Scoped &) = delete;
        Scoped &operator=(Scoped &) = delete;

        // Can't write to the wrapped reference.
        Scoped &operator=(Scoped &&rhs) = delete;

        // This allows us to return Scoped objects from e.g.
        // factory functions while still balacing out the indentation
        // level properly.
        Scoped(Scoped &&rhs) : _indentation(rhs._indentation), _delta(rhs._delta)
        {
            // Effectively neuter the moved-from object.
            rhs._delta = 0;
        }

    private:
        Indentation &_indentation;
        int _delta;
    };

    Scoped Indented(int delta) { return std::move(Scoped(*this, delta)); }

private:
    std::string _indent_step;
    int _indent_level;
    mutable std::string _cached_indent;
    mutable bool _cached_indent_dirty{true};
};


class IndentationLogger
{
    jaws::Logger &_logger;
    jaws::LogLevel _log_level;
    Indentation _indentation;

public:
    IndentationLogger(
        jaws::Logger &logger,
        const jaws::LogLevel log_level = spdlog::level::level_enum::info,
        const int initial_indent_level = 0,
        const std::string &indent_step = {}) :
        _logger(logger), _log_level(log_level), _indentation(initial_indent_level, indent_step)
    {}

    void ChangeLevel(int delta) { _indentation.ChangeLevel(delta); }

    Indentation::Scoped Indented(int delta = 1) { return std::move(_indentation.Indented(delta)); }

    template <typename... Args>
    void operator()(const char *format, const Args &... args)
    {
        Log(format, args...);
    }

    template <typename... Args>
    void Log(const char *format, const Args &... args)
    {
        _logger.log(_log_level, (_indentation.Get() + format).c_str(), args...);
    }
};


}
