#pragma once
#include "jaws/core.hpp"
#include "jaws/fwd.hpp"
#include <memory>
#include <string>
#include <vector>

namespace jaws {

class JAWS_API Jaws
{
public:
    Jaws();
    ~Jaws();
    Jaws(const Jaws&) = delete;
    Jaws& operator=(const Jaws&) = delete;
    Jaws(Jaws&&) = default;
    Jaws& operator=(Jaws&&) = default;

    void create(int argc = 0, char** argv = nullptr);
    void destroy();

    static Jaws* instance();

    //-------------------------------------------------------------------------

    const std::vector<std::string>& get_cmd_line_args() const;

    void set_fatal_handler(FatalHandler);
    FatalHandler get_fatal_handler() const;

    //-------------------------------------------------------------------------
    // Vfs

    vfs::Vfs& get_vfs();
    const vfs::Vfs& get_vfs() const;

    //-------------------------------------------------------------------------
    // Logging

    Logger& get_logger(Category = Category::User) const;
    LoggerPtr get_logger_ptr(Category = Category::User) const;

private:
    static Jaws* _the_instance;

    std::vector<std::string> _cmd_line_args;

    mutable FatalHandler _fatal_handler;

    // Created on demand (possibly from const functions)
    mutable std::unique_ptr<vfs::Vfs> _vfs;
    mutable std::unique_ptr<Logging> _logging;
};

extern JAWS_API Logger& get_logger(Category = Category::User);
extern JAWS_API LoggerPtr get_logger_ptr(Category = Category::User);

} // namespace jaws
