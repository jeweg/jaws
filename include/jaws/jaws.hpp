#pragma once
#include "jaws/core.hpp"
#include "jaws/fwd.hpp"

#include <string>
#include <vector>

namespace jaws {

extern JAWS_API bool init(int argc = 0, char **argv = nullptr);
extern JAWS_API void destroy();

extern JAWS_API bool is_initialized();

extern JAWS_API Logger &get_logger(Category = Category::User);

extern JAWS_API vfs::Vfs &get_vfs();

extern JAWS_API const std::vector<std::string> &get_cmd_line_args();

extern JAWS_API void set_fatal_handler(FatalHandler);
extern JAWS_API FatalHandler get_fatal_handler();

struct InitGuard final
{
    InitGuard(int argc, char **argv);
    ~InitGuard();
};

}
