#include "jaws/jaws.hpp"

jaws::LoggerPtr logger;

// Here I'm experimenting with
// ...

int main(int argc, char** argv)
{
    logger = jaws::GetLoggerPtr(jaws::Category::General);
    logger->info("Hello, World!");
}
