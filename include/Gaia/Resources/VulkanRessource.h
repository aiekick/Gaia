/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#pragma warning(disable : 4251)

#include <Gaia/gaia.h>
#include <Gaia/Core/VulkanCore.h>
#include <Gaia/gaia.h>

#include <array>
#include <vector>
#include <string>

struct GAIA_API VulkanAccelStructObject {
    vk::AccelerationStructureKHR handle = nullptr;
    vk::Buffer buffer = nullptr;
    VmaAllocation alloc_meta = nullptr;
    VmaMemoryUsage alloc_usage = VMA_MEMORY_USAGE_UNKNOWN;
    vk::BufferUsageFlags buffer_usage;
    uint64_t device_address = 0U;
};
typedef std::shared_ptr<VulkanAccelStructObject> VulkanAccelStructObjectPtr;

struct GAIA_API VulkanImageObject {
    vk::Image image = nullptr;
    VmaAllocation alloc_meta = nullptr;
};
typedef std::shared_ptr<VulkanImageObject> VulkanImageObjectPtr;

class GAIA_API VulkanBufferObject {
public:
    vk::Buffer buffer = nullptr;
    VmaAllocation alloc_meta = nullptr;
    VmaMemoryUsage alloc_usage = VMA_MEMORY_USAGE_UNKNOWN;
    vk::BufferUsageFlags buffer_usage;
    uint64_t device_address = 0U;
    vk::BufferView bufferView = VK_NULL_HANDLE;

public:
    bool MapMemory(void* vMappedMemory);
    void UnmapMemory();
};
typedef std::shared_ptr<VulkanBufferObject> VulkanBufferObjectPtr;

namespace GaiApi {
class GAIA_API VulkanRessource {
public:
    static vk::DescriptorBufferInfo* getEmptyDescriptorBufferInfo() {
        // is working only when nulldescriptor feature is enabled
        // and range must be VK_WHOLE_SIZE in this case
        static vk::DescriptorBufferInfo bufferInfo = {VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};
        return &bufferInfo;
    }

public:  // image
    static void copy(VulkanCoreWeak vVulkanCore,
        vk::Image dst,
        vk::Buffer src,
        const vk::BufferImageCopy& region,
        vk::ImageLayout layout = vk::ImageLayout::eTransferDstOptimal);
    static void copy(VulkanCoreWeak vVulkanCore,
        vk::Image dst,
        vk::Buffer src,
        const std::vector<vk::BufferImageCopy>& regions,
        vk::ImageLayout layout = vk::ImageLayout::eTransferDstOptimal);
    static void copy(VulkanCoreWeak vVulkanCore,
        vk::Buffer dst,
        vk::Image src,
        const vk::BufferImageCopy& region,
        vk::ImageLayout layout = vk::ImageLayout::eTransferSrcOptimal);
    static void copy(VulkanCoreWeak vVulkanCore,
        vk::Buffer dst,
        vk::Image src,
        const std::vector<vk::BufferImageCopy>& regions,
        vk::ImageLayout layout = vk::ImageLayout::eTransferSrcOptimal);

    static VulkanImageObjectPtr createSharedImageObject(
        VulkanCoreWeak vVulkanCore, const vk::ImageCreateInfo& image_info, const VmaAllocationCreateInfo& alloc_info, const char* vDebugLabel);
    static VulkanImageObjectPtr createTextureImage2D(VulkanCoreWeak vVulkanCore,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevelCount,
        vk::Format format,
        void* hostdata_ptr,
        const char* vDebugLabel);
    static VulkanImageObjectPtr createTextureImage3D(VulkanCoreWeak vVulkanCore,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        vk::Format format,
        void* hostdata_ptr,
        const char* vDebugLabel);
    static VulkanImageObjectPtr createTextureImageCube(VulkanCoreWeak vVulkanCore,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevelCount,
        vk::Format format,
        std::array<std::vector<uint8_t>, 6U> hostdatas,
        const char* vDebugLabel);
    static VulkanImageObjectPtr createColorAttachment2D(VulkanCoreWeak vVulkanCore,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevelCount,
        vk::Format format,
        vk::SampleCountFlagBits vSampleCount,
        const char* vDebugLabel);
    static VulkanImageObjectPtr createComputeTarget2D(VulkanCoreWeak vVulkanCore,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevelCount,
        vk::Format format,
        vk::SampleCountFlagBits vSampleCount,
        const char* vDebugLabel);
    static VulkanImageObjectPtr createDepthAttachment(VulkanCoreWeak vVulkanCore,
        uint32_t width,
        uint32_t height,
        vk::Format format,
        vk::SampleCountFlagBits vSampleCount,
        const char* vDebugLabel);

    static void GenerateMipmaps(
        VulkanCoreWeak vVulkanCore, vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    static void transitionImageLayout(VulkanCoreWeak vVulkanCore,
        vk::Image image,
        vk::Format format,
        uint32_t mipLevel,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        uint32_t layersCount = 1U);
    static void transitionImageLayout(VulkanCoreWeak vVulkanCore,
        vk::Image image,
        vk::Format format,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::ImageSubresourceRange subresourceRange);

    static bool hasStencilComponent(vk::Format format);

    static void getDatasFromTextureImage2D(VulkanCoreWeak vVulkanCore,
        uint32_t width,
        uint32_t height,
        vk::Format format,
        std::shared_ptr<VulkanImageObject> vImage,
        void* vDatas,
        uint32_t* vSize);

public:  // buffers
    static void copy(VulkanCoreWeak vVulkanCore, vk::Buffer dst, vk::Buffer src, const vk::BufferCopy& region, vk::CommandPool* vCommandPool = 0);
    static void copy(
        VulkanCoreWeak vVulkanCore, vk::Buffer dst, vk::Buffer src, const std::vector<vk::BufferCopy>& regions, vk::CommandPool* vCommandPool = 0);
    static bool upload(VulkanCoreWeak vVulkanCore, VulkanBufferObjectPtr dstHostVisiblePtr, void* src_host, size_t size_bytes, size_t dst_offset = 0);
    static bool download(GaiApi::VulkanCoreWeak vVulkanCore, VulkanBufferObjectPtr srcHostVisiblePtr, void* dst_host, size_t size_bytes);

    // will set deveic adress of buffer in vVulkanBufferObjectPtr
    static void SetDeviceAddress(const vk::Device& vDevice, VulkanBufferObjectPtr vVulkanBufferObjectPtr);
    static VulkanBufferObjectPtr createSharedBufferObject(
        VulkanCoreWeak vVulkanCore, const vk::BufferCreateInfo& bufferinfo, const VmaAllocationCreateInfo& alloc_info, const char* vDebugLabel);
    static VulkanBufferObjectPtr createUniformBufferObject(VulkanCoreWeak vVulkanCore, uint64_t vSize, const char* vDebugLabel);
    static VulkanBufferObjectPtr createStagingBufferObject(VulkanCoreWeak vVulkanCore, uint64_t vSize, const char* vDebugLabel);
    static VulkanBufferObjectPtr createStorageBufferObject(
        VulkanCoreWeak vVulkanCore, uint64_t vSize, vk::BufferUsageFlags vBufferUsageFlags, VmaMemoryUsage vMemoryUsage, const char* vDebugLabel);
    static VulkanBufferObjectPtr createStorageBufferObject(
        VulkanCoreWeak vVulkanCore, uint64_t vSize, VmaMemoryUsage vMemoryUsage, const char* vDebugLabel);
    static VulkanBufferObjectPtr createGPUOnlyStorageBufferObject(VulkanCoreWeak vVulkanCore, void* vData, uint64_t vSize, const char* vDebugLabel);
    static VulkanBufferObjectPtr createBiDirectionalStorageBufferObject(
        GaiApi::VulkanCoreWeak vVulkanCore, void* vData, uint64_t vSize, const char* vDebugLabel);
    static VulkanBufferObjectPtr createTexelBuffer(
        GaiApi::VulkanCoreWeak vVulkanCore, vk::Format vFormat, uint64_t vDataSize, void* vDataPtr, const char* vDebugLabel);

    template <class T>
    static VulkanBufferObjectPtr createVertexBufferObject(
        VulkanCoreWeak vVulkanCore, const std::vector<T>& data, bool vUseSSBO, bool vUseTransformFeedback, bool vUseRTX, const char* vDebugLabel);
    static VulkanBufferObjectPtr createEmptyVertexBufferObject(
        VulkanCoreWeak vVulkanCore, const size_t& vByteSize, bool vUseSSBO, bool vUseTransformFeedback, bool vUseRTX, const char* vDebugLabel);
    template <class T>
    static VulkanBufferObjectPtr createIndexBufferObject(
        VulkanCoreWeak vVulkanCore, const std::vector<T>& data, bool vUseSSBO, bool vUseTransformFeedback, bool vUseRTX, const char* vDebugLabel);
    static VulkanBufferObjectPtr createEmptyIndexBufferObject(
        VulkanCoreWeak vVulkanCore, const size_t& vByteSize, bool vUseSSBO, bool vUseTransformFeedback, bool vUseRTX, const char* vDebugLabel);

public:  // RTX Accel Structure
    // will set deveic adress of buffer in vVulkanBufferObjectPtr
    static void SetDeviceAddress(const vk::Device& vDevice, VulkanAccelStructObjectPtr vVulkanAccelStructObjectPtr);
    static VulkanAccelStructObjectPtr createAccelStructureBufferObject(
        VulkanCoreWeak vVulkanCore, uint64_t vSize, VmaMemoryUsage vMemoryUsage, const char* vDebugLabel);
};

template <class T>
VulkanBufferObjectPtr VulkanRessource::createVertexBufferObject(
    VulkanCoreWeak vVulkanCore, const std::vector<T>& data, bool vUseSSBO, bool vUseTransformFeedback, bool vUseRTX, const char* vDebugLabel) {
    vk::BufferCreateInfo stagingBufferInfo = {};
    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingBufferInfo.size = data.size() * sizeof(T);
    stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    stagingAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
    auto stagebufferPtr = createSharedBufferObject(vVulkanCore, stagingBufferInfo, stagingAllocInfo, vDebugLabel);
    if (stagebufferPtr) {
        upload(vVulkanCore, stagebufferPtr, (void*)data.data(), stagingBufferInfo.size);

        vk::BufferCreateInfo vboInfo = {};
        VmaAllocationCreateInfo vboAllocInfo = {};
        vboInfo.size = data.size() * sizeof(T);
        vboInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer |  // VBO
                        vk::BufferUsageFlagBits::eTransferSrc |   // GPU to CPU
                        vk::BufferUsageFlagBits::eTransferDst;    // CPU to GPU
        if (vUseSSBO)
            vboInfo.usage = vboInfo.usage | vk::BufferUsageFlagBits::eStorageBuffer;
        if (vUseTransformFeedback)
            vboInfo.usage = vboInfo.usage | vk::BufferUsageFlagBits::eTransformFeedbackBufferEXT;
        if (vUseRTX)
            vboInfo.usage = vboInfo.usage | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;

        vboAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
        auto vboPtr = createSharedBufferObject(vVulkanCore, vboInfo, vboAllocInfo, vDebugLabel);
        if (vboPtr) {
            vk::BufferCopy region = {};
            region.size = stagingBufferInfo.size;
            copy(vVulkanCore, vboPtr->buffer, stagebufferPtr->buffer, region);
            return vboPtr;
        }
    }

    return nullptr;
}

template <class T>
VulkanBufferObjectPtr VulkanRessource::createIndexBufferObject(
    VulkanCoreWeak vVulkanCore, const std::vector<T>& data, bool vUseSSBO, bool vUseTransformFeedback, bool vUseRTX, const char* vDebugLabel) {
    vk::BufferCreateInfo stagingBufferInfo = {};
    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingBufferInfo.size = data.size() * sizeof(T);
    stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    stagingAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
    auto stagebufferPtr = createSharedBufferObject(vVulkanCore, stagingBufferInfo, stagingAllocInfo, vDebugLabel);
    if (stagebufferPtr) {
        upload(vVulkanCore, stagebufferPtr, (void*)data.data(), stagingBufferInfo.size);

        vk::BufferCreateInfo vboInfo = {};
        VmaAllocationCreateInfo vboAllocInfo = {};
        vboInfo.size = data.size() * sizeof(T);
        vboInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer |  // IBO
                        vk::BufferUsageFlagBits::eTransferSrc |  // GPU to CPU
                        vk::BufferUsageFlagBits::eTransferDst;   // CPU to GPU
        if (vUseSSBO)
            vboInfo.usage = vboInfo.usage |  //
                            vk::BufferUsageFlagBits::eStorageBuffer;
        if (vUseTransformFeedback)
            vboInfo.usage = vboInfo.usage |  //
                            vk::BufferUsageFlagBits::eTransformFeedbackBufferEXT;
        if (vUseRTX)
            vboInfo.usage = vboInfo.usage |                                                         //
                            vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |  //
                            vk::BufferUsageFlagBits::eStorageBuffer |                               //
                            vk::BufferUsageFlagBits::eShaderDeviceAddress;

        vboAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
        auto vboPtr = createSharedBufferObject(vVulkanCore, vboInfo, vboAllocInfo, vDebugLabel);
        if (vboPtr) {
            vk::BufferCopy region = {};
            region.size = stagingBufferInfo.size;
            copy(vVulkanCore, vboPtr->buffer, stagebufferPtr->buffer, region);

            return vboPtr;
        }
    }

    return nullptr;
}

}  // namespace GaiApi
