#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

#include <vector>

struct VulkanBufferObject
{
	vk::Buffer buffer;
	VmaAllocation alloc_meta;
	VmaMemoryUsage alloc_usage;
};

namespace vkApi
{
	class VulkanBuffer
	{
	public:
		static void copy(vk::Buffer dst, vk::Buffer src, const vk::BufferCopy& region, vk::CommandPool *vCommandPool = 0);
		static void copy(vk::Buffer dst, vk::Buffer src, const std::vector<vk::BufferCopy>& regions, vk::CommandPool *vCommandPool = 0);
		static void upload(VulkanBufferObject& dst_hostVisable, void* src_host, size_t size_bytes, size_t dst_offset = 0);

		static std::shared_ptr<VulkanBufferObject> createSharedBufferObject(const vk::BufferCreateInfo& bufferinfo, const VmaAllocationCreateInfo& alloc_info);
		static std::shared_ptr<VulkanBufferObject> createUniformBufferObject(uint64_t vSize);
		static std::shared_ptr<VulkanBufferObject> createStagingBufferObject(uint64_t vSize);
		static std::shared_ptr<VulkanBufferObject> createStorageBufferObject(uint64_t vSize, VmaMemoryUsage vMemoryUsage);
		static std::shared_ptr<VulkanBufferObject> createGPUOnlyStorageBufferObject(void* vData, uint64_t vSize);

		template<class T> static std::shared_ptr<VulkanBufferObject> createVertexBufferObject(const std::vector<T>& data, bool vUseSSBO = false, bool vUseTransformFeedback = false);
		template<class T> static std::shared_ptr<VulkanBufferObject> createIndexBufferObject(const std::vector<T>& data);
	};

	template<class T>
	std::shared_ptr<VulkanBufferObject> VulkanBuffer::createVertexBufferObject(const std::vector<T>& data, bool vUseSSBO, bool vUseTransformFeedback)
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
	std::shared_ptr<VulkanBufferObject> VulkanBuffer::createIndexBufferObject(const std::vector<T>& data)
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
		vboAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
		auto vbo = createSharedBufferObject(vboInfo, vboAllocInfo);

		vk::BufferCopy region = {};
		region.size = data.size() * sizeof(T);
		copy(vbo->buffer, stagebuffer->buffer, region);

		return vbo;
	}
}