#pragma once
#include "jaws/vulkan/map_guard.hpp"
#include "jaws/vulkan/device.hpp"

namespace jaws::vulkan {

MapGuard::MapGuard(Device *device, VmaAllocation vma_allocation) :
    _vma_allocator(device->get_vma_allocator()), _vma_allocation(vma_allocation)
{
    JAWS_ASSUME(device);
    VkResult result = vmaMapMemory(_vma_allocator, vma_allocation, &_ptr);
    if (result != VK_SUCCESS) {
        // Probably not necessary.
        _ptr = nullptr;
    }
}

MapGuard::~MapGuard()
{
    if (_ptr) { vmaUnmapMemory(_vma_allocator, _vma_allocation); }
}


}
