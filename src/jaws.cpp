#include "pch.hpp"
#include "jaws/jaws.hpp"
#include "jaws/fatal.hpp"
#include "jaws/assume.hpp"
#include "jaws/logging.hpp"
#include "jaws/vfs/vfs.hpp"

#include <memory>
#include <iostream>

namespace jaws {

//-------------------------------------------------------------------------

static_assert(sizeof(uint8_t) == sizeof(unsigned char), "");
static_assert(sizeof(size_t) == sizeof(uint64_t), "");

//-------------------------------------------------------------------------

// Outside the context so that they can be used
// to report init() failures.
FatalHandler g_fatal_handler;
Logging g_logging;

struct GlobalContext
{
    std::vector<std::string> cmd_line_args;
    vfs::Vfs vfs;
};

std::unique_ptr<GlobalContext> g_context;

//-------------------------------------------------------------------------

bool init(int argc, char **argv)
{
    if (g_context) {
        JAWS_FATAL1("jaws already initialized!");
        return false;
    }
    g_context = std::make_unique<GlobalContext>();
    g_context->cmd_line_args.reserve(argc);
    for (int i = 0; i < argc; ++i) { g_context->cmd_line_args.emplace_back(argv[i]); }
    return true;
}


void destroy()
{
    if (!g_context) { JAWS_FATAL1("jaws not yet initialized!"); }
    g_context.reset();
}


InitGuard::InitGuard(int argc, char **argv)
{
    JAWS_ASSUME(jaws::init(argc, argv));
}


InitGuard::~InitGuard()
{
    jaws::destroy();
}


bool is_initialized()
{
    return !!g_context;
}


Logger &get_logger(Category cat)
{
    return g_logging.get_logger(cat);
}


vfs::Vfs &get_vfs()
{
    return g_context->vfs;
}


const std::vector<std::string> &get_cmd_line_args()
{
    return g_context->cmd_line_args;
}


void set_fatal_handler(FatalHandler fh)
{
    g_fatal_handler = std::move(fh);
}


FatalHandler get_fatal_handler()
{
    if (!g_fatal_handler) {
        // Always return a valid handler so things like JAWS_FATAL work.
        return &DefaultFatalHandler;
    }
    return g_fatal_handler;
}

}
