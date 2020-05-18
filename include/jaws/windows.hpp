#pragma once
#if defined(JAWS_OS_WIN)

#    include "jaws/core.hpp"

#    pragma warning(push)
// Avoid warning C4005: macro redefinition.
#    pragma warning(disable : 4005)
#    define NOMINMAX
#    define VC_EXTRALEAN
#    define WIN32_LEAN_AND_MEAN
#    define STRICT
#    pragma warning(pop)

// Avoid warning C4005: 'APIENTRY': macro redefinition.
// We define it in OpenGL.h to make glad independent from windows.h
// We will still get these warnings if windows.h is included elsewhere, though.
#    if defined(APIENTRY)
#        undef APIENTRY
#    endif

#    include <Windows.h>
#    include <comdef.h> // For _com_error
#    include <string>

namespace jaws {

extern JAWS_API HINSTANCE GetHInstance();

extern JAWS_API std::string HResultToString(HRESULT res);
extern JAWS_API std::string ErrorCodeToString(DWORD err);

} // namespace jaws

#endif
