#if _WIN32

#    include "pch.hpp"
#    include "jaws/windows.hpp"

// Used to access the hinstance of the module we're linked into.
// See https://blogs.msdn.microsoft.com/oldnewthing/20041025-00/?p=37483
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace jaws {

HINSTANCE GetHInstance()
{
    return (HINSTANCE)&__ImageBase;
}


std::string HResultToString(HRESULT res)
{
    // TODO
    // auto ce = _com_error(res);
    // auto foo = ce.ErrorMessage();
    return "";
}


std::string ErrorCodeToString(DWORD err)
{
    static char buffer[10 * 1024] = {'\0'};
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)buffer,
        sizeof(buffer) - 1, // Retain a safety null byte at the end of the buffer.
        NULL);
    return std::string(buffer);
}

}

#endif
