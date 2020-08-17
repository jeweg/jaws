#include "jaws/assume.hpp"

#pragma once
#if defined(_WIN32)
#    define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__)
#    define VK_USE_PLATFORM_XCB_KHR
#endif

#include "volk.h"

#define VMA_USE_STL_CONTAINERS 0
#define VMA_USE_STL_SHARED_MUTEX 0
#define VMA_ASSERT(expr) JAWS_ASSUME(expr)
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"

#include "jaws/fatal.hpp"
#include <limits>


#define JAWS_VK_HANDLE_RETURN(vk_result, return_value) \
    if (false) {                                       \
    } else if (vk_result < 0) {                        \
        return return_value;                           \
    }


#define JAWS_VK_HANDLE_FATAL(vk_result) \
    if (false) {                        \
    } else if (vk_result < 0) {         \
        JAWS_FATAL0();                  \
    }


namespace jaws::vulkan {

constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

// The logic is modelled after similar code in Khronos' vulkan.hpp.
// Note that in almost all vkEnumerate... style functions the count and output data always seem to
// be the last two parameters. We use that fact here.
// I say almost all because there's at last vkEnumerateInstanceVersion that completely falls out
// of this pattern.
template <typename ElemType, typename... FirstArgs, typename FuncPtrType>
std::vector<ElemType> enumerated(FuncPtrType func_ptr, ElemType elem_prototype, FirstArgs... first_args)
{
    VkResult result;
    std::vector<ElemType> result_list;
    uint32_t count = std::numeric_limits<uint32_t>::max();
    do {
        result = func_ptr(first_args..., &count, nullptr);
        JAWS_VK_HANDLE_RETURN(result, result_list);
        if (result == VK_SUCCESS && count > 0) {
            result_list.assign(count, elem_prototype);
            result = func_ptr(first_args..., &count, result_list.data());
            JAWS_VK_HANDLE_RETURN(result, result_list);
        }
    } while (result == VK_INCOMPLETE);
    if (result == VK_SUCCESS) {
        JAWS_ASSUME(count <= result_list.size());
        result_list.resize(count);
    }
    return result_list;
}

// version of enumerated() for void-returning function (e.g. vkGetPhysicalDeviceQueueFamilyProperties2).
template <typename ElemType, typename... FirstArgs, typename FuncPtrType>
std::vector<ElemType> enumerated_void(FuncPtrType func_ptr, ElemType elem_prototype, FirstArgs... first_args)
{
    std::vector<ElemType> result_list;
    uint32_t count = std::numeric_limits<uint32_t>::max();
    func_ptr(first_args..., &count, nullptr);
    if (count > 0) {
        result_list.assign(count, elem_prototype);
        func_ptr(first_args..., &count, result_list.data());
    }
    JAWS_ASSUME(count <= result_list.size());
    result_list.resize(count);
    return result_list;
}

} // namespace jaws::vulkan
