#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

#include <vector>

class VulkanBufferObject
{
public:
	vk::Buffer buffer;
	VmaAllocation alloc_meta;
	VmaMemoryUsage alloc_usage;
};
typedef std::shared_ptr<VulkanBufferObject> VulkanBufferObjectPtr;

namespace vkApi
{
	class VulkanBuffer
	{
	public:
		static void copy(vk::Buffer dst, vk::Buffer src, const vk::BufferCopy& region, vk::CommandPool* vCommandPool = 0);
		static void copy(vk::Buffer dst, vk::Buffer src, const std::vector<vk::BufferCopy>& regions, vk::CommandPool* vCommandPool = 0);
		static void upload(VulkanBufferObject& dst_hostVisable, void* src_host, size_t size_bytes, size_t dst_offset = 0);

		static VulkanBufferObjectPtr createSharedBufferObject(const vk::BufferCreateInfo& bufferinfo, const VmaAllocationCreateInfo& alloc_info);
		static VulkanBufferObjectPtr createUniformBufferObject(uint64_t vSize);
		static VulkanBufferObjectPtr createStagingBufferObject(uint64_t vSize);
		static VulkanBufferObjectPtr createStorageBufferObject(uint64_t vSize, VmaMemoryUsage vMemoryUsage);
		static VulkanBufferObjectPtr createGPUOnlyStorageBufferObject(void* vData, uint64_t vSize);

		template<class T> static VulkanBufferObjectPtr createVertexBufferObject(const std::vector<T>& data, bool vUseSSBO = false, bool vUseTransformFeedback = false);
		template<class T> static VulkanBufferObjectPtr createIndexBufferObject(const std::vector<T>& data, bool vUseSSBO = false, bool vUseTransformFeedback = false);
	};

	template<class T>
	VulkanBufferObjectPtr VulkanBuffer::createVertexBufferObject(const std::vector<T>& data, bool vUseSSBO, bool vUseTransformFeedback)
	{
		vk::BufferCreateInfo stagingBufferInfo = {};
		VmaAllocationCreateInfo stagingAllocInfo = {};
		stagingBufferInfo.size = data.size() * sizeof(T);
		stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
		stagingAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
		auto stagebuffer = createSharedBufferObject(stagingBufferInfo, stagingAllocInfo);
		upload(*stagebuffer, (void*)data.data(), data.size() * sizeof(T));

		vk::BufferCreateInfo vboInfo = {};
		VmaAllocationCreateInfo vboAllocInfo = {};
		vboInfo.size = data.size() * sizeof(T);
		vboInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		if (vUseSSBO) vboInfo.usage = vboInfo.usage | vk::BufferUsageFlagBits::eStorageBuffer;
		if (vUseTransformFeedback) vboInfo.usage = vboInfo.usage | vk::BufferUsageFlagBits::eTransformFeedbackBufferEXT;

		vboAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
		auto vbo = createSharedBufferObject(vboInfo, vboAllocInfo);

		vk::BufferCopy region = {};
		region.size = data.size() * sizeof(T);
		copy(vbo->buffer, stagebuffer->buffer, region);

		return vbo;
	}

	template<class T>
	VulkanBufferObjectPtr VulkanBuffer::createIndexBufferObject(const std::vector<T>& data, bool vUseSSBO, bool vUseTransformFeedback)
	{
		vk::BufferCreateInfo stagingBufferInfo = {};
		VmaAllocationCreateInfo stagingAllocInfo = {};
		stagingBufferInfo.size = data.size() * sizeof(T);
		stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
		stagingAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
		auto stagebuffer = createSharedBufferObject(stagingBufferInfo, stagingAllocInfo);
		upload(*stagebuffer, (void*)data.data(), data.size() * sizeof(T));

		vk::BufferCreateInfo vboInfo = {};
		VmaAllocationCreateInfo vboAllocInfo = {};
		vboInfo.size = data.size() * sizeof(T);
		vboInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		if (vUseSSBO) vboInfo.usage = vboInfo.usage | vk::BufferUsageFlagBits::eStorageBuffer;
		if (vUseTransformFeedback) vboInfo.usage = vboInfo.usage | vk::BufferUsageFlagBits::eTransformFeedbackBufferEXT;

		vboAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
		auto vbo = createSharedBufferObject(vboInfo, vboAllocInfo);

		vk::BufferCopy region = {};
		region.size = data.size() * sizeof(T);
		copy(vbo->buffer, stagebuffer->buffer, region);

		return vbo;
	}
}