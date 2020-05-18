#include <jaws/jaws.hpp>

//#include <spdlog/spdlog.h>
//#include <boost/scope_exit.hpp>
//#include <iostream>


void JustFatalOut()
{
    JAWS_FATAL0();
}


void JustFatalOutWithMsg()
{
    JAWS_FATAL1("well, we crash");
}


int main(int argc, char** argv)
{
    jaws::Instance instance;
    instance.create(argc, argv);

    //JustFatalOut();
    JustFatalOutWithMsg();


    /*
    // TODO: Why is the global set_level ignored here?
    spdlog::set_level(spdlog::level::debug);

    // This works, though.
    jaws::GetLogger(jaws::Category::General).set_level(spdlog::level::debug);

    // Piggypacking onto one of jaws' loggers here.
    auto& logger = jaws::GetLogger(jaws::Category::General);
    */
}
