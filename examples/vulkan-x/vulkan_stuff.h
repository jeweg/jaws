#pragma once
#include "jaws/core.hpp"
#include "jaws/logging.hpp"
#if defined(JAWS_OS_WIN)
#    include "jaws/windows.hpp"
#endif

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

//=========================================================================

jaws::LoggerPtr Logger()
{
    return jaws::GetLoggerPtr(jaws::Category::Vulkan);
}

//=========================================================================

class VulkanLib
{
    static constexpr const char* LIB_NAME_WIN = "vulkan-1.dll";
    static constexpr const char* LIB_NAME_LINUX = "libvulkan.so.1";

#if defined(JAWS_OS_WIN)
    HINSTANCE _vulkan_lib;
#elif defined(JAWS_OS_LINUX)
    void* _vulkan_lib;
#endif

public:
    bool Init()
    {
        const char* vulkan_lib_name = nullptr;
#if defined(JAWS_OS_WIN)
        vulkan_lib_name = LIB_NAME_WIN;
        _vulkan_lib = LoadLibrary(vulkan_lib_name);
#elif defined(JAWS_OS_LINUX)
        // Something like this...
        vulkan_lib_name = LIB_NAME_LINUX;
        _vulkan_lib = dlopen(vulkan_lib_name, RTLD_NOW);
#endif

        if (nullptr == _vulkan_lib) {
            Logger()->error("vulkan lib \"{}\" could not be loaded", vulkan_lib_name);
            return false;
        }





        return true;
    }


    bool Destroy()
    {
#if defined(JAWS_OS_WIN)
        if (_vulkan_lib) {
            BOOL ok = FreeLibrary(_vulkan_lib);
            _vulkan_lib = nullptr;
        }
#endif
        return true;
    }

    ~VulkanLib() { Destroy(); }
};
//=========================================================================
//

/*
#if !defined(VK_EXPORTED_FUNCTION)
#    define VK_EXPORTED_FUNCTION(fun)
#endif
 */

VK_EXPORTED_FUNCTION(vkGetInstanceProcAddr)

namespace jaws::vulkan {
#define VK_EXPORTED_FUNCTION(fun) PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION(fun) PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION(fun) PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION(fun) PFN_##fun fun;

#include "ListOfFunctions.inl"

} // namespace jaws::vulkan

#if defined(JAWS_OS_WIN)
#define JAWS_LoadProcAddress GetProcAddress
#elif defined(JAWS_OS_LINUX)
#define JAWS_LoadProcAddress dlsym
#endif

#define VK_EXPORTED_FUNCTION( fun )                                                   \
if( !(fun = (PFN_##fun)JAWS_LoadProcAddress( VulkanLibrary, #fun )) ) {                \
  std::cout << "Could not load exported function: " << #fun << "!" << std::endl;  \
  return false;                                                                   \
}

#include "ListOfFunctions.inl"


#undef VK_EXPORTED_FUNCTION
