#pragma once

#include "jaws/core.hpp"
#include <iosfwd>

namespace jaws::util {

// Based on https://stackoverflow.com/a/9600752/10311823
class JAWS_API IndentingOStream final : public std::ostream
{
    class JAWS_API IndentingStreambuf final : public std::streambuf
    {
        std::streambuf *_sink;
        std::string _indent_step;
        int _indent_level;
        std::ostream *_owner = nullptr;
        bool _at_line_start = true;

    protected:
        int overflow(int ch) override;

    public:
        explicit IndentingStreambuf(
            std::streambuf *sink, const std::string &indent_step = "    ", int initial_level = 0);

        void ChangeLevelBy(int delta);
        void ChangeLevelTo(int level);
        int GetCurrentLevel() const;
    };

    IndentingStreambuf _buf;

public:
    IndentingOStream(std::ostream &os, const std::string &indent_step = "    ", int initial_level = 0);

    IndentingOStream &ChangeLevelBy(int delta);
    IndentingOStream &ChangeLevelTo(int level);
    int GetCurrentLevel() const;
};

/// RAII class to indent output for a scope.
/// This class can only be used to change the indent level by delta, not to
/// an absolute number.
class Indenting
{
    IndentingOStream &_stream;
    int _level_delta;

public:
    Indenting(IndentingOStream &stream, int level_delta = 1);
    ~Indenting();
};

//=============================================================================
// Inline definitions:

inline int IndentingOStream::IndentingStreambuf::overflow(int ch)
{
    if (_at_line_start && ch != '\n') {
        for (int i = 0; i < _indent_level; ++i) { _sink->sputn(_indent_step.c_str(), _indent_step.size()); }
    }
    _at_line_start = ch == '\n';
    return _sink->sputc(ch);
}

inline IndentingOStream::IndentingStreambuf::IndentingStreambuf(
    std::streambuf *sink, const std::string &indent_step, int initial_level) :
    _sink(sink), _indent_step(indent_step), _indent_level(initial_level)
{}

inline void IndentingOStream::IndentingStreambuf::ChangeLevelBy(int delta)
{
    _indent_level += delta;
}

inline void IndentingOStream::IndentingStreambuf::ChangeLevelTo(int level)
{
    _indent_level = level;
}

inline int IndentingOStream::IndentingStreambuf::GetCurrentLevel() const
{
    return _indent_level;
}

inline IndentingOStream::IndentingOStream(std::ostream &os, const std::string &indent_step, int initial_level) :
    std::ostream(&_buf), _buf(os.rdbuf(), indent_step, initial_level)
{}

inline IndentingOStream &IndentingOStream::ChangeLevelBy(int delta)
{
    _buf.ChangeLevelBy(delta);
    return *this;
}

inline IndentingOStream &IndentingOStream::ChangeLevelTo(int level)
{
    _buf.ChangeLevelTo(level);
    return *this;
}

inline int IndentingOStream::GetCurrentLevel() const
{
    return _buf.GetCurrentLevel();
}

inline Indenting::Indenting(IndentingOStream &stream, int level_delta) : _stream(stream), _level_delta(level_delta)
{
    _stream.ChangeLevelBy(_level_delta);
}

inline Indenting::~Indenting()
{
    _stream.ChangeLevelBy(-_level_delta);
}

}
