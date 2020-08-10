#include <jaws/jaws.hpp>
#include <jaws/fatal.hpp>


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
    // fatal works without jaws initialized.

    //JustFatalOut();
    JustFatalOutWithMsg();
}
