#pragma once
#include "vulkan/vulkan.hpp"
#include "absl/strings/string_view.h"
#include <sstream>

void PrintPhysicalDeviceInfo(
    const vk::PhysicalDevice& dev, std::ostream& oss, absl::string_view indent, absl::string_view line_prefix)
{
    VkPhysicalDeviceProperties props{};
    vk.GetPhysicalDeviceProperties(dev, &props);

    template<typename T> auto line = [&](absl::string_view caption, T thing) { oss << caption << ": " << thing; };

    line("device name", props.deviceName);
}


#if 0
            VkPhysicalDeviceProperties props{};
            vk.GetPhysicalDeviceProperties(physical_devices[i], &props);
            LOG(INFO) << "  [Vulkan device " << i << "]: " << props.deviceName;
            LOG(INFO) << "    API version: " << VK_VERSION_MAJOR(props.apiVersion) << "."
                      << VK_VERSION_MINOR(props.apiVersion) << "." << VK_VERSION_PATCH(props.apiVersion);
            LOG(INFO) << "    vendorID: " << props.vendorID << " (" << GetPciSigVendorName(props.vendorID) << ")";
            LOG(INFO) << "    deviceID: " << props.deviceID;
            LOG(INFO) << "    driver version: " << props.driverVersion;

            const auto& l = props.limits;
            const char* indent = "      ";
            LOG(INFO) << "    limits:";
            LOG(INFO) << indent << "maxImageDimension1D: " << l.maxImageDimension1D;
            LOG(INFO) << indent << "maxImageDimension2D: " << l.maxImageDimension2D;
            LOG(INFO) << indent << "maxImageDimension3D: " << l.maxImageDimension3D;
            LOG(INFO) << indent << "maxImageDimensionCube: " << l.maxImageDimensionCube;
            LOG(INFO) << indent << "maxImageArrayLayers: " << l.maxImageArrayLayers;
            LOG(INFO) << indent << "maxTexelBufferElements: " << l.maxTexelBufferElements;
            LOG(INFO) << indent << "maxUniformBufferRange: " << l.maxUniformBufferRange;
            LOG(INFO) << indent << "maxStorageBufferRange: " << l.maxStorageBufferRange;
            LOG(INFO) << indent << "maxPushConstantsSize: " << l.maxPushConstantsSize;
            LOG(INFO) << indent << "maxMemoryAllocationCount: " << l.maxMemoryAllocationCount;
            LOG(INFO) << indent << "maxSamplerAllocationCount: " << l.maxSamplerAllocationCount;
            LOG(INFO) << indent << "bufferImageGranularity: " << l.bufferImageGranularity;
            LOG(INFO) << indent << "sparseAddressSpaceSize: " << l.sparseAddressSpaceSize;
            LOG(INFO) << indent << "maxBoundDescriptorSets: " << l.maxBoundDescriptorSets;
            LOG(INFO) << indent << "maxPerStageDescriptorSamplers: " << l.maxPerStageDescriptorSamplers;
            LOG(INFO) << indent << "maxPerStageDescriptorUniformBuffers: " << l.maxPerStageDescriptorUniformBuffers;
            LOG(INFO) << indent << "maxPerStageDescriptorStorageBuffers: " << l.maxPerStageDescriptorStorageBuffers;
            LOG(INFO) << indent << "maxPerStageDescriptorSampledImages: " << l.maxPerStageDescriptorSampledImages;
            LOG(INFO) << indent << "maxPerStageDescriptorStorageImages: " << l.maxPerStageDescriptorStorageImages;
            LOG(INFO) << indent << "maxPerStageDescriptorInputAttachments: " << l.maxPerStageDescriptorInputAttachments;
            LOG(INFO) << indent << "maxPerStageResources: " << l.maxPerStageResources;
            LOG(INFO) << indent << "maxDescriptorSetSamplers: " << l.maxDescriptorSetSamplers;
            LOG(INFO) << indent << "maxDescriptorSetUniformBuffers: " << l.maxDescriptorSetUniformBuffers;
            LOG(INFO) << indent << "maxDescriptorSetUniformBuffersDynamic: " << l.maxDescriptorSetUniformBuffersDynamic;
            LOG(INFO) << indent << "maxDescriptorSetStorageBuffers: " << l.maxDescriptorSetStorageBuffers;
            LOG(INFO) << indent << "maxDescriptorSetStorageBuffersDynamic: " << l.maxDescriptorSetStorageBuffersDynamic;
            LOG(INFO) << indent << "maxDescriptorSetSampledImages: " << l.maxDescriptorSetSampledImages;
            LOG(INFO) << indent << "maxDescriptorSetStorageImages: " << l.maxDescriptorSetStorageImages;
            LOG(INFO) << indent << "maxDescriptorSetInputAttachments: " << l.maxDescriptorSetInputAttachments;
            LOG(INFO) << indent << "maxVertexInputAttributes: " << l.maxVertexInputAttributes;
            LOG(INFO) << indent << "maxVertexInputBindings: " << l.maxVertexInputBindings;
            LOG(INFO) << indent << "maxVertexInputAttributeOffset: " << l.maxVertexInputAttributeOffset;
            LOG(INFO) << indent << "maxVertexInputBindingStride: " << l.maxVertexInputBindingStride;
            LOG(INFO) << indent << "maxVertexOutputComponents: " << l.maxVertexOutputComponents;
            LOG(INFO) << indent << "maxTessellationGenerationLevel: " << l.maxTessellationGenerationLevel;
            LOG(INFO) << indent << "maxTessellationPatchSize: " << l.maxTessellationPatchSize;
            LOG(INFO) << indent << "maxTessellationControlPerVertexInputComponents: "
                      << l.maxTessellationControlPerVertexInputComponents;
            LOG(INFO) << indent << "maxTessellationControlPerVertexOutputComponents: "
                      << l.maxTessellationControlPerVertexOutputComponents;
            LOG(INFO) << indent << "maxTessellationControlPerPatchOutputComponents: "
                      << l.maxTessellationControlPerPatchOutputComponents;
            LOG(INFO) << indent << "maxTessellationControlTotalOutputComponents: "
                      << l.maxTessellationControlTotalOutputComponents;
            LOG(INFO) << indent
                      << "maxTessellationEvaluationInputComponents: " << l.maxTessellationEvaluationInputComponents;
            LOG(INFO) << indent
                      << "maxTessellationEvaluationOutputComponents: " << l.maxTessellationEvaluationOutputComponents;
            LOG(INFO) << indent << "maxGeometryShaderInvocations: " << l.maxGeometryShaderInvocations;
            LOG(INFO) << indent << "maxGeometryInputComponents: " << l.maxGeometryInputComponents;
            LOG(INFO) << indent << "maxGeometryOutputComponents: " << l.maxGeometryOutputComponents;
            LOG(INFO) << indent << "maxGeometryOutputVertices: " << l.maxGeometryOutputVertices;
            LOG(INFO) << indent << "maxGeometryTotalOutputComponents: " << l.maxGeometryTotalOutputComponents;
            LOG(INFO) << indent << "maxFragmentInputComponents: " << l.maxFragmentInputComponents;
            LOG(INFO) << indent << "maxFragmentOutputAttachments: " << l.maxFragmentOutputAttachments;
            LOG(INFO) << indent << "maxFragmentDualSrcAttachments: " << l.maxFragmentDualSrcAttachments;
            LOG(INFO) << indent << "maxFragmentCombinedOutputResources: " << l.maxFragmentCombinedOutputResources;
            LOG(INFO) << indent << "maxComputeSharedMemorySize: " << l.maxComputeSharedMemorySize;
            LOG(INFO) << indent << "maxComputeWorkGroupCount[3]: " << l.maxComputeWorkGroupCount;
            LOG(INFO) << indent << "maxComputeWorkGroupInvocations: " << l.maxComputeWorkGroupInvocations;
            LOG(INFO) << indent << "maxComputeWorkGroupSize[3]: " << l.maxComputeWorkGroupSize;
            LOG(INFO) << indent << "subPixelPrecisionBits: " << l.subPixelPrecisionBits;
            LOG(INFO) << indent << "subTexelPrecisionBits: " << l.subTexelPrecisionBits;
            LOG(INFO) << indent << "mipmapPrecisionBits: " << l.mipmapPrecisionBits;
            LOG(INFO) << indent << "maxDrawIndexedIndexValue: " << l.maxDrawIndexedIndexValue;
            LOG(INFO) << indent << "maxDrawIndirectCount: " << l.maxDrawIndirectCount;
            LOG(INFO) << indent << "maxSamplerLodBias: " << l.maxSamplerLodBias;
            LOG(INFO) << indent << "maxSamplerAnisotropy: " << l.maxSamplerAnisotropy;
            LOG(INFO) << indent << "maxViewports: " << l.maxViewports;
            LOG(INFO) << indent << "maxViewportDimensions[2]: " << l.maxViewportDimensions;
            LOG(INFO) << indent << "viewportBoundsRange[2]: " << l.viewportBoundsRange;
            LOG(INFO) << indent << "viewportSubPixelBits: " << l.viewportSubPixelBits;
            LOG(INFO) << indent << "minMemoryMapAlignment: " << l.minMemoryMapAlignment;
            LOG(INFO) << indent << "minTexelBufferOffsetAlignment: " << l.minTexelBufferOffsetAlignment;
            LOG(INFO) << indent << "minUniformBufferOffsetAlignment: " << l.minUniformBufferOffsetAlignment;
            LOG(INFO) << indent << "minStorageBufferOffsetAlignment: " << l.minStorageBufferOffsetAlignment;
            LOG(INFO) << indent << "minTexelOffset: " << l.minTexelOffset;
            LOG(INFO) << indent << "maxTexelOffset: " << l.maxTexelOffset;
            LOG(INFO) << indent << "minTexelGatherOffset: " << l.minTexelGatherOffset;
            LOG(INFO) << indent << "maxTexelGatherOffset: " << l.maxTexelGatherOffset;
            LOG(INFO) << indent << "minInterpolationOffset: " << l.minInterpolationOffset;
            LOG(INFO) << indent << "maxInterpolationOffset: " << l.maxInterpolationOffset;
            LOG(INFO) << indent << "subPixelInterpolationOffsetBits: " << l.subPixelInterpolationOffsetBits;
            LOG(INFO) << indent << "maxFramebufferWidth: " << l.maxFramebufferWidth;
            LOG(INFO) << indent << "maxFramebufferHeight: " << l.maxFramebufferHeight;
            LOG(INFO) << indent << "maxFramebufferLayers: " << l.maxFramebufferLayers;
            LOG(INFO) << indent << "framebufferColorSampleCounts: " << l.framebufferColorSampleCounts;
            LOG(INFO) << indent << "framebufferDepthSampleCounts: " << l.framebufferDepthSampleCounts;
            LOG(INFO) << indent << "framebufferStencilSampleCounts: " << l.framebufferStencilSampleCounts;
            LOG(INFO) << indent << "framebufferNoAttachmentsSampleCounts: " << l.framebufferNoAttachmentsSampleCounts;
            LOG(INFO) << indent << "maxColorAttachments: " << l.maxColorAttachments;
            LOG(INFO) << indent << "sampledImageColorSampleCounts: " << l.sampledImageColorSampleCounts;
            LOG(INFO) << indent << "sampledImageIntegerSampleCounts: " << l.sampledImageIntegerSampleCounts;
            LOG(INFO) << indent << "sampledImageDepthSampleCounts: " << l.sampledImageDepthSampleCounts;
            LOG(INFO) << indent << "sampledImageStencilSampleCounts: " << l.sampledImageStencilSampleCounts;
            LOG(INFO) << indent << "storageImageSampleCounts: " << l.storageImageSampleCounts;
            LOG(INFO) << indent << "maxSampleMaskWords: " << l.maxSampleMaskWords;
            LOG(INFO) << indent << "timestampComputeAndGraphics: " << l.timestampComputeAndGraphics;
            LOG(INFO) << indent << "timestampPeriod: " << l.timestampPeriod;
            LOG(INFO) << indent << "maxClipDistances: " << l.maxClipDistances;
            LOG(INFO) << indent << "maxCullDistances: " << l.maxCullDistances;
            LOG(INFO) << indent << "maxCombinedClipAndCullDistances: " << l.maxCombinedClipAndCullDistances;
            LOG(INFO) << indent << "discreteQueuePriorities: " << l.discreteQueuePriorities;
            LOG(INFO) << indent << "pointSizeRange[2]: " << l.pointSizeRange;
            LOG(INFO) << indent << "lineWidthRange[2]: " << l.lineWidthRange;
            LOG(INFO) << indent << "pointSizeGranularity: " << l.pointSizeGranularity;
            LOG(INFO) << indent << "lineWidthGranularity: " << l.lineWidthGranularity;
            LOG(INFO) << indent << "strictLines: " << l.strictLines;
            LOG(INFO) << indent << "standardSampleLocations: " << l.standardSampleLocations;
            LOG(INFO) << indent << "optimalBufferCopyOffsetAlignment: " << l.optimalBufferCopyOffsetAlignment;
            LOG(INFO) << indent << "optimalBufferCopyRowPitchAlignment: " << l.optimalBufferCopyRowPitchAlignment;
            LOG(INFO) << indent << "nonCoherentAtomSize: " << l.nonCoherentAtomSize;

            const auto& s = props.sparseProperties;
            LOG(INFO) << "    sparseProperties:";
            LOG(INFO) << indent << "residencyStandard2DBlockShape: " << s.residencyStandard2DBlockShape;
            LOG(INFO) << indent
                      << "residencyStandard2DMultisampleBlockShape: " << s.residencyStandard2DMultisampleBlockShape;
            LOG(INFO) << indent << "residencyStandard3DBlockShape: " << s.residencyStandard3DBlockShape;
            LOG(INFO) << indent << "residencyAlignedMipSize: " << s.residencyAlignedMipSize;
            LOG(INFO) << indent << "residencyNonResidentStrict: " << s.residencyNonResidentStrict;

            VkPhysicalDeviceFeatures f;
            vk.GetPhysicalDeviceFeatures(physical_devices[i], &f);

            LOG(INFO) << "    Features:";
            LOG(INFO) << indent << "robustBufferAccess: " << f.robustBufferAccess;
            LOG(INFO) << indent << "fullDrawIndexUint32: " << f.fullDrawIndexUint32;
            LOG(INFO) << indent << "imageCubeArray: " << f.imageCubeArray;
            LOG(INFO) << indent << "independentBlend: " << f.independentBlend;
            LOG(INFO) << indent << "geometryShader: " << f.geometryShader;
            LOG(INFO) << indent << "tessellationShader: " << f.tessellationShader;
            LOG(INFO) << indent << "sampleRateShading: " << f.sampleRateShading;
            LOG(INFO) << indent << "dualSrcBlend: " << f.dualSrcBlend;
            LOG(INFO) << indent << "logicOp: " << f.logicOp;
            LOG(INFO) << indent << "multiDrawIndirect: " << f.multiDrawIndirect;
            LOG(INFO) << indent << "drawIndirectFirstInstance: " << f.drawIndirectFirstInstance;
            LOG(INFO) << indent << "depthClamp: " << f.depthClamp;
            LOG(INFO) << indent << "depthBiasClamp: " << f.depthBiasClamp;
            LOG(INFO) << indent << "fillModeNonSolid: " << f.fillModeNonSolid;
            LOG(INFO) << indent << "depthBounds: " << f.depthBounds;
            LOG(INFO) << indent << "wideLines: " << f.wideLines;
            LOG(INFO) << indent << "largePoints: " << f.largePoints;
            LOG(INFO) << indent << "alphaToOne: " << f.alphaToOne;
            LOG(INFO) << indent << "multiViewport: " << f.multiViewport;
            LOG(INFO) << indent << "samplerAnisotropy: " << f.samplerAnisotropy;
            LOG(INFO) << indent << "textureCompressionETC2: " << f.textureCompressionETC2;
            LOG(INFO) << indent << "textureCompressionASTC_LDR: " << f.textureCompressionASTC_LDR;
            LOG(INFO) << indent << "textureCompressionBC: " << f.textureCompressionBC;
            LOG(INFO) << indent << "occlusionQueryPrecise: " << f.occlusionQueryPrecise;
            LOG(INFO) << indent << "pipelineStatisticsQuery: " << f.pipelineStatisticsQuery;
            LOG(INFO) << indent << "vertexPipelineStoresAndAtomics: " << f.vertexPipelineStoresAndAtomics;
            LOG(INFO) << indent << "fragmentStoresAndAtomics: " << f.fragmentStoresAndAtomics;
            LOG(INFO) << indent
                      << "shaderTessellationAndGeometryPointSize: " << f.shaderTessellationAndGeometryPointSize;
            LOG(INFO) << indent << "shaderImageGatherExtended: " << f.shaderImageGatherExtended;
            LOG(INFO) << indent << "shaderStorageImageExtendedFormats: " << f.shaderStorageImageExtendedFormats;
            LOG(INFO) << indent << "shaderStorageImageMultisample: " << f.shaderStorageImageMultisample;
            LOG(INFO) << indent << "shaderStorageImageReadWithoutFormat: " << f.shaderStorageImageReadWithoutFormat;
            LOG(INFO) << indent << "shaderStorageImageWriteWithoutFormat: " << f.shaderStorageImageWriteWithoutFormat;
            LOG(INFO) << indent
                      << "shaderUniformBufferArrayDynamicIndexing: " << f.shaderUniformBufferArrayDynamicIndexing;
            LOG(INFO) << indent
                      << "shaderSampledImageArrayDynamicIndexing: " << f.shaderSampledImageArrayDynamicIndexing;
            LOG(INFO) << indent
                      << "shaderStorageBufferArrayDynamicIndexing: " << f.shaderStorageBufferArrayDynamicIndexing;
            LOG(INFO) << indent
                      << "shaderStorageImageArrayDynamicIndexing: " << f.shaderStorageImageArrayDynamicIndexing;
            LOG(INFO) << indent << "shaderClipDistance: " << f.shaderClipDistance;
            LOG(INFO) << indent << "shaderCullDistance: " << f.shaderCullDistance;
            LOG(INFO) << indent << "shaderFloat64: " << f.shaderFloat64;
            LOG(INFO) << indent << "shaderInt64: " << f.shaderInt64;
            LOG(INFO) << indent << "shaderInt16: " << f.shaderInt16;
            LOG(INFO) << indent << "shaderResourceResidency: " << f.shaderResourceResidency;
            LOG(INFO) << indent << "shaderResourceMinLod: " << f.shaderResourceMinLod;
            LOG(INFO) << indent << "sparseBinding: " << f.sparseBinding;
            LOG(INFO) << indent << "sparseResidencyBuffer: " << f.sparseResidencyBuffer;
            LOG(INFO) << indent << "sparseResidencyImage2D: " << f.sparseResidencyImage2D;
            LOG(INFO) << indent << "sparseResidencyImage3D: " << f.sparseResidencyImage3D;
            LOG(INFO) << indent << "sparseResidency2Samples: " << f.sparseResidency2Samples;
            LOG(INFO) << indent << "sparseResidency4Samples: " << f.sparseResidency4Samples;
            LOG(INFO) << indent << "sparseResidency8Samples: " << f.sparseResidency8Samples;
            LOG(INFO) << indent << "sparseResidency16Samples: " << f.sparseResidency16Samples;
            LOG(INFO) << indent << "sparseResidencyAliased: " << f.sparseResidencyAliased;
            LOG(INFO) << indent << "variableMultisampleRate: " << f.variableMultisampleRate;
            LOG(INFO) << indent << "inheritedQueries: " << f.inheritedQueries;

#endif
