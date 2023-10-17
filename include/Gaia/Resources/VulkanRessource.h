/*
Copyright 2022-2022 Stephane Cuillerdier (aka aiekick)

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

#include <vulkan/vulkan.hpp>
#include <Gaia/Core/VulkanCore.h>
#include <Gaia/Core/vk_mem_alloc.h>
#include <Gaia/gaia.h>

#include <array>
#include <vector>
#include <string>

struct GAIA_API VulkanAccelStructObject
{
	vk::AccelerationStructureKHR handle = nullptr;
	vk::Buffer buffer = nullptr;
	VmaAllocation alloc_meta = nullptr;
	VmaMemoryUsage alloc_usage = VMA_MEMORY_USAGE_UNKNOWN;
	vk::BufferUsageFlags buffer_usage;
	uint64_t device_address = 0U;
};
typedef std::shared_ptr<VulkanAccelStructObject> VulkanAccelStructObjectPtr;

struct GAIA_API VulkanImageObject
{
	vk::Image image = nullptr;
	VmaAllocation alloc_meta = nullptr;
};
typedef std::shared_ptr<VulkanImageObject> VulkanImageObjectPtr;

class GAIA_API VulkanBufferObject
{
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
class GAIA_API VulkanRessource
{
public:
	static vk::DescriptorBufferInfo* getEmptyDescriptorBufferInfo() 
	{
		// is working only when nulldescriptor feature is enabled
		// and range must be VK_WHOLE_SIZE in this case
		static vk::DescriptorBufferInfo bufferInfo = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
		return &bufferInfo;
	}

public: // image
	static void copy(VulkanCorePtr vVulkanCorePtr, vk::Image dst, vk::Buffer src, const vk::BufferImageCopy& region, vk::ImageLayout layout = vk::ImageLayout::eTransferDstOptimal);
	static void copy(VulkanCorePtr vVulkanCorePtr, vk::Image dst, vk::Buffer src, const std::vector<vk::BufferImageCopy>& regions, vk::ImageLayout layout = vk::ImageLayout::eTransferDstOptimal);
	static void copy(VulkanCorePtr vVulkanCorePtr, vk::Buffer dst, vk::Image  src, const vk::BufferImageCopy& region, vk::ImageLayout layout = vk::ImageLayout::eTransferSrcOptimal);
	static void copy(VulkanCorePtr vVulkanCorePtr, vk::Buffer dst, vk::Image  src, const std::vector<vk::BufferImageCopy>& regions, vk::ImageLayout layout = vk::ImageLayout::eTransferSrcOptimal);

	static VulkanImageObjectPtr createSharedImageObject(VulkanCorePtr vVulkanCorePtr, const vk::ImageCreateInfo& image_info, const VmaAllocationCreateInfo& alloc_info);
	static VulkanImageObjectPtr createTextureImage2D(VulkanCorePtr vVulkanCorePtr, uint32_t width, uint32_t height, uint32_t mipLevelCount, vk::Format format, void* hostdata_ptr);
	static VulkanImageObjectPtr createTextureImageCube(VulkanCorePtr vVulkanCorePtr, uint32_t width, uint32_t height, uint32_t mipLevelCount, vk::Format format, std::array<std::vector<uint8_t>, 6U> hostdatas);
	static VulkanImageObjectPtr createColorAttachment2D(VulkanCorePtr vVulkanCorePtr, uint32_t width, uint32_t height, uint32_t mipLevelCount, vk::Format format, vk::SampleCountFlagBits vSampleCount);
	static VulkanImageObjectPtr createComputeTarget2D(VulkanCorePtr vVulkanCorePtr, uint32_t width, uint32_t height, uint32_t mipLevelCount, vk::Format format, vk::SampleCountFlagBits vSampleCount);
	static VulkanImageObjectPtr createDepthAttachment(VulkanCorePtr vVulkanCorePtr, uint32_t width, uint32_t height, vk::Format format, vk::SampleCountFlagBits vSampleCount);

	static void GenerateMipmaps(VulkanCorePtr vVulkanCorePtr, vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	static void transitionImageLayout(VulkanCorePtr vVulkanCorePtr, vk::Image image, vk::Format format, uint32_t mipLevel, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t layersCount = 1U);
	static void transitionImageLayout(VulkanCorePtr vVulkanCorePtr, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageSubresourceRange subresourceRange);

	static bool hasStencilComponent(vk::Format format);

	static void getDatasFromTextureImage2D(VulkanCorePtr vVulkanCorePtr, uint32_t width, uint32_t height, vk::Format format, std::shared_ptr<VulkanImageObject> vImage, void* vDatas, uint32_t* vSize);

public: // buffers
	static void copy(VulkanCorePtr vVulkanCorePtr, vk::Buffer dst, vk::Buffer src, const vk::BufferCopy& region, vk::CommandPool* vCommandPool = 0);
	static void copy(VulkanCorePtr vVulkanCorePtr, vk::Buffer dst, vk::Buffer src, const std::vector<vk::BufferCopy>& regions, vk::CommandPool* vCommandPool = 0);
	static void upload(VulkanCorePtr vVulkanCorePtr, VulkanBufferObjectPtr dstHostVisiblePtr, void* src_host, size_t size_bytes, size_t dst_offset = 0);
	static void download(GaiApi::VulkanCorePtr vVulkanCorePtr, VulkanBufferObjectPtr srcHostVisiblePtr, void* dst_host, size_t size_bytes);

	// will set deveic adress of buffer in vVulkanBufferObjectPtr
	static void SetDeviceAddress(const vk::Device& vDevice, VulkanBufferObjectPtr vVulkanBufferObjectPtr);
	static VulkanBufferObjectPtr createSharedBufferObject(VulkanCorePtr vVulkanCorePtr, const vk::BufferCreateInfo& bufferinfo, const VmaAllocationCreateInfo& alloc_info);
	static VulkanBufferObjectPtr createUniformBufferObject(VulkanCorePtr vVulkanCorePtr, uint64_t vSize);
	static VulkanBufferObjectPtr createStagingBufferObject(VulkanCorePtr vVulkanCorePtr, uint64_t vSize);
	static VulkanBufferObjectPtr createStorageBufferObject(VulkanCorePtr vVulkanCorePtr, uint64_t vSize, vk::BufferUsageFlags vBufferUsageFlags, VmaMemoryUsage vMemoryUsage);
	static VulkanBufferObjectPtr createStorageBufferObject(VulkanCorePtr vVulkanCorePtr, uint64_t vSize, VmaMemoryUsage vMemoryUsage);
	static VulkanBufferObjectPtr createGPUOnlyStorageBufferObject(VulkanCorePtr vVulkanCorePtr, void* vData, uint64_t vSize); 
	static VulkanBufferObjectPtr createBiDirectionalStorageBufferObject(GaiApi::VulkanCorePtr vVulkanCorePtr, void* vData, uint64_t vSize);
	static VulkanBufferObjectPtr createTexelBuffer(GaiApi::VulkanCorePtr vVulkanCorePtr,	vk::Format vFormat, uint64_t vDataSize, void* vDataPtr = nullptr);

	template<class T> static VulkanBufferObjectPtr createVertexBufferObject(VulkanCorePtr vVulkanCorePtr, const std::vector<T>& data, bool vUseSSBO = false, bool vUseTransformFeedback = false, bool vUseRTX = false);
	static VulkanBufferObjectPtr createEmptyVertexBufferObject(VulkanCorePtr vVulkanCorePtr, const size_t& vByteSize, bool vUseSSBO = false, bool vUseTransformFeedback = false, bool vUseRTX = false);
	template<class T> static VulkanBufferObjectPtr createIndexBufferObject(VulkanCorePtr vVulkanCorePtr, const std::vector<T>& data, bool vUseSSBO = false, bool vUseTransformFeedback = false, bool vUseRTX = false);
	static VulkanBufferObjectPtr createEmptyIndexBufferObject(VulkanCorePtr vVulkanCorePtr, const size_t& vByteSize, bool vUseSSBO = false, bool vUseTransformFeedback = false, bool vUseRTX = false);

public: // RTX Accel Structure
	// will set deveic adress of buffer in vVulkanBufferObjectPtr
	static void SetDeviceAddress(const vk::Device& vDevice, VulkanAccelStructObjectPtr vVulkanAccelStructObjectPtr);
	static VulkanAccelStructObjectPtr createAccelStructureBufferObject(VulkanCorePtr vVulkanCorePtr, uint64_t vSize, VmaMemoryUsage vMemoryUsage);
};

template<class T>
VulkanBufferObjectPtr VulkanRessource::createVertexBufferObject(VulkanCorePtr vVulkanCorePtr, const std::vector<T>& data, bool vUseSSBO, bool vUseTransformFeedback, bool vUseRTX)
{
	vk::BufferCreateInfo stagingBufferInfo = {};
	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingBufferInfo.size = data.size() * sizeof(T);
	stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	stagingAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	auto stagebufferPtr = createSharedBufferObject(vVulkanCorePtr, stagingBufferInfo, stagingAllocInfo);
	if (stagebufferPtr)
	{
		upload(vVulkanCorePtr, stagebufferPtr, (void*)data.data(), data.size() * sizeof(T));

		vk::BufferCreateInfo vboInfo = {};
		VmaAllocationCreateInfo vboAllocInfo = {};
		vboInfo.size = data.size() * sizeof(T);
		vboInfo.usage = 
			vk::BufferUsageFlagBits::eVertexBuffer | // VBO
			vk::BufferUsageFlagBits::eTransferDst; // CPU to GPU
		if (vUseSSBO) vboInfo.usage = vboInfo.usage |
			vk::BufferUsageFlagBits::eStorageBuffer;
		if (vUseTransformFeedback) vboInfo.usage = vboInfo.usage |
			vk::BufferUsageFlagBits::eTransformFeedbackBufferEXT;
		if (vUseRTX) vboInfo.usage = vboInfo.usage |
			vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
			vk::BufferUsageFlagBits::eStorageBuffer |
			vk::BufferUsageFlagBits::eShaderDeviceAddress;

		vboAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
		auto vboPtr = createSharedBufferObject(vVulkanCorePtr, vboInfo, vboAllocInfo);
		if (vboPtr)
		{
			vk::BufferCopy region = {};
			region.size = data.size() * sizeof(T);
			copy(vVulkanCorePtr, vboPtr->buffer, stagebufferPtr->buffer, region);

			return vboPtr;
		}
	}
	
	return nullptr;
}

template<class T>
VulkanBufferObjectPtr VulkanRessource::createIndexBufferObject(VulkanCorePtr vVulkanCorePtr, const std::vector<T>& data, bool vUseSSBO, bool vUseTransformFeedback, bool vUseRTX)
{
	vk::BufferCreateInfo stagingBufferInfo = {};
	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingBufferInfo.size = data.size() * sizeof(T);
	stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	stagingAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	auto stagebufferPtr = createSharedBufferObject(vVulkanCorePtr, stagingBufferInfo, stagingAllocInfo);
	if (stagebufferPtr)
	{
		upload(vVulkanCorePtr, stagebufferPtr, (void*)data.data(), data.size() * sizeof(T));

		vk::BufferCreateInfo vboInfo = {};
		VmaAllocationCreateInfo vboAllocInfo = {};
		vboInfo.size = data.size() * sizeof(T);
		vboInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		if (vUseSSBO) vboInfo.usage = vboInfo.usage |
			vk::BufferUsageFlagBits::eStorageBuffer;
		if (vUseTransformFeedback) vboInfo.usage = vboInfo.usage |
			vk::BufferUsageFlagBits::eTransformFeedbackBufferEXT;
		if (vUseRTX) vboInfo.usage = vboInfo.usage |
			vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
			vk::BufferUsageFlagBits::eStorageBuffer |
			vk::BufferUsageFlagBits::eShaderDeviceAddress;

		vboAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
		auto vboPtr = createSharedBufferObject(vVulkanCorePtr, vboInfo, vboAllocInfo);
		if (vboPtr)
		{
			vk::BufferCopy region = {};
			region.size = data.size() * sizeof(T);
			copy(vVulkanCorePtr, vboPtr->buffer, stagebufferPtr->buffer, region);

			return vboPtr;
		}
	}

	return nullptr;
}
}