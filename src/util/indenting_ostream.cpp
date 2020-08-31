#include "pch.hpp"
#include "jaws/util/indenting_ostream.hpp"

#if 0
namespace jaws { namespace util {

int IndentingOStream::IndentingStreambuf::overflow(int ch)
{
    if (_at_line_start && ch != '\n') {
        for (int i = 0; i < _indent_level; ++i) { _sink->sputn(_indent_step.c_str(), _indent_step.size()); }
    }
    _at_line_start = ch == '\n';
    return _sink->sputc(ch);
}

IndentingOStream::IndentingStreambuf::IndentingStreambuf(
    std::streambuf* sink, const std::string& indent_step, int initial_level) :
    _sink(sink),
    _at_line_start(true)
    //, myIndent( indent, ' ' )
    ,
    _owner(nullptr),
    _indent_step(indent_step),
    _indent_level(initial_level)
{}

void IndentingOStream::IndentingStreambuf::ChangeLevelBy(int delta)
{
    _indent_level += delta;
}

void IndentingOStream::IndentingStreambuf::ChangeLevelTo(int level)
{
    _indent_level = level;
}

IndentingOStream::IndentingOStream(std::ostream& os, const std::string& indent_step, int initial_level) :
    _buf(os.rdbuf(), indent_step, initial_level),
    std::ostream(&_buf)
{}

IndentingOStream& IndentingOStream::ChangeLevelBy(int delta)
{
    _buf.ChangeLevelBy(delta);
    return *this;
}

IndentingOStream& IndentingOStream::ChangeLevelTo(int level)
{
    _buf.ChangeLevelTo(level);
    return *this;
}

}}
#endif
