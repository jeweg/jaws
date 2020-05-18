#include "pch.hpp"
#include "jaws/jaws.hpp"
#include "jaws/fatal.hpp"
#include "jaws/logging.hpp"
#include "jaws/vfs/vfs.hpp"
#include <memory>
#include <iostream>

namespace jaws {

//-------------------------------------------------------------------------

static_assert(sizeof(uint8_t) == sizeof(unsigned char), "");
static_assert(sizeof(size_t) == sizeof(uint64_t), "");

//-------------------------------------------------------------------------

Jaws* Jaws::_the_instance = nullptr;

Jaws::Jaws() {}

Jaws::~Jaws()
{
    destroy();
}

Jaws* Jaws::instance()
{
    if (!_the_instance) {
        std::cerr << "No Jaws instance created, did you invoke Jaws::create?" << std::endl;
    }
    return _the_instance;
}

void Jaws::create(int argc, char** argv)
{
    if (_the_instance) {
        // Hmmm!
    }
    _the_instance = this;

    _cmd_line_args.reserve(argc);
    for (size_t i = 0; i < argc; ++i) {
        _cmd_line_args.emplace_back(argv[i]);
    }
}

void Jaws::destroy() {}


const std::vector<std::string>& Jaws::get_cmd_line_args() const
{
    return _cmd_line_args;
}


void Jaws::set_fatal_handler(FatalHandler fh)
{
    _fatal_handler = std::move(fh);
}


FatalHandler Jaws::get_fatal_handler() const
{
    if (!_fatal_handler) {
        _fatal_handler = &DefaultFatalHandler;
    }
    return _fatal_handler;
}


vfs::Vfs& Jaws::get_vfs()
{
    return const_cast<vfs::Vfs&>(const_cast<const Jaws*>(this)->get_vfs());
}

const vfs::Vfs& Jaws::get_vfs() const
{
    if (!_vfs) { _vfs = std::make_unique<vfs::Vfs>(); }
    return *_vfs;
}

Logger& Jaws::get_logger(Category cat) const
{
    if (!_logging) { _logging = std::make_unique<Logging>(); }
    return _logging->get_logger(cat);
}

LoggerPtr Jaws::get_logger_ptr(Category cat) const
{
    if (!_logging) { _logging = std::make_unique<Logging>(); }
    return _logging->get_logger_ptr(cat);
}

Logger& get_logger(Category cat)
{
    return Jaws::instance()->get_logger(cat);
}

LoggerPtr get_logger_ptr(Category cat)
{
    return Jaws::instance()->get_logger_ptr(cat);
}

} // namespace jaws
