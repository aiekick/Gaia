// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "VulkanBuffer.h"
#include "VulkanCore.h"
#include <ctools/Logger.h>
#include "VulkanCommandBuffer.h"

#define TRACE_MEMORY
#include <vkProfiler/Profiler.h>

namespace vkApi
{
	void VulkanBuffer::copy(vk::Buffer dst, vk::Buffer src, const vk::BufferCopy& region, vk::CommandPool* vCommandPool)
	{
		ZoneScoped;

		auto cmd = VulkanCommandBuffer::beginSingleTimeCommands(true, vCommandPool);
		cmd.copyBuffer(src, dst, region);
		VulkanCommandBuffer::flushSingleTimeCommands(cmd, true, vCommandPool);
	}

	void VulkanBuffer::upload(VulkanBufferObject& dst_hostVisable, void* src_host, size_t size_bytes, size_t dst_offset)
	{
		ZoneScoped;

		if (dst_hostVisable.alloc_usage == VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY)
		{
			LogVarDebug("upload not done because it is VMA_MEMORY_USAGE_GPU_ONLY");
			return;
		}
		void* dst = nullptr;
		VulkanCore::check_error(vmaMapMemory(VulkanCore::Instance()->getMemAllocator(), dst_hostVisable.alloc_meta, &dst));
		memcpy((uint8_t*)dst + dst_offset, src_host, size_bytes);
		vmaUnmapMemory(VulkanCore::Instance()->getMemAllocator(), dst_hostVisable.alloc_meta);
	}

	void VulkanBuffer::copy(vk::Buffer dst, vk::Buffer src, const std::vector<vk::BufferCopy>& regions, vk::CommandPool* vCommandPool)
	{
		ZoneScoped;

		auto cmd = VulkanCommandBuffer::beginSingleTimeCommands(true, vCommandPool);
		cmd.copyBuffer(src, dst, regions);
		VulkanCommandBuffer::flushSingleTimeCommands(cmd, true, vCommandPool);
	}

	std::shared_ptr<VulkanBufferObject> VulkanBuffer::createSharedBufferObject(const vk::BufferCreateInfo& bufferinfo, const VmaAllocationCreateInfo& alloc_info)
	{
		ZoneScoped;

		auto data = std::shared_ptr<VulkanBufferObject>(new VulkanBufferObject, [](VulkanBufferObject* obj)
			{
				vmaDestroyBuffer(VulkanCore::Instance()->getMemAllocator(), (VkBuffer)obj->buffer, obj->alloc_meta);
			});
		data->alloc_usage = alloc_info.usage;
		VulkanCore::check_error(vmaCreateBuffer(VulkanCore::Instance()->getMemAllocator(), (VkBufferCreateInfo*)&bufferinfo, &alloc_info, (VkBuffer*)&data->buffer, &data->alloc_meta, nullptr));
		return data;
	}

	std::shared_ptr<VulkanBufferObject> VulkanBuffer::createUniformBufferObject(uint64_t vSize)
	{
		ZoneScoped;

		vk::BufferCreateInfo sbo_create_info = {};
		VmaAllocationCreateInfo sbo_alloc_info = {};
		sbo_create_info.size = vSize;
		sbo_create_info.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		sbo_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
		return createSharedBufferObject(sbo_create_info, sbo_alloc_info);
	}

	std::shared_ptr<VulkanBufferObject> VulkanBuffer::createStagingBufferObject(uint64_t vSize)
	{
		ZoneScoped;

		vk::BufferCreateInfo stagingBufferInfo = {};
		VmaAllocationCreateInfo stagingAllocInfo = {};
		stagingBufferInfo.size = vSize;
		stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
		stagingAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
		return createSharedBufferObject(stagingBufferInfo, stagingAllocInfo);
	}

	std::shared_ptr<VulkanBufferObject> VulkanBuffer::createStorageBufferObject(uint64_t vSize, VmaMemoryUsage vMemoryUsage)
	{
		ZoneScoped;

		vk::BufferCreateInfo storageBufferInfo = {};
		VmaAllocationCreateInfo storageAllocInfo = {};
		storageBufferInfo.size = vSize;
		storageBufferInfo.sharingMode = vk::SharingMode::eExclusive;
		storageAllocInfo.usage = vMemoryUsage;
		if (vMemoryUsage == VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU)
		{
			storageBufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
			return createSharedBufferObject(storageBufferInfo, storageAllocInfo);
		}
		else if (vMemoryUsage == VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_TO_CPU)
		{
			storageBufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc;
			return createSharedBufferObject(storageBufferInfo, storageAllocInfo);
		}
		else if (vMemoryUsage == VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY)
		{
			storageBufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer;
			return createSharedBufferObject(storageBufferInfo, storageAllocInfo);
		}
		return 0;
	}

	std::shared_ptr<VulkanBufferObject> VulkanBuffer::createGPUOnlyStorageBufferObject(void* vData, uint64_t vSize)
	{
		if (vData && vSize)
		{
			vk::BufferCreateInfo stagingBufferInfo = {};
			stagingBufferInfo.size = vSize;
			stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
			VmaAllocationCreateInfo stagingAllocInfo = {};
			stagingAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
			auto stagebuffer = createSharedBufferObject(stagingBufferInfo, stagingAllocInfo);
			upload(*stagebuffer, vData, vSize);

			vk::BufferCreateInfo storageBufferInfo = {};
			storageBufferInfo.size = vSize;
			storageBufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
			storageBufferInfo.sharingMode = vk::SharingMode::eExclusive;
			VmaAllocationCreateInfo vboAllocInfo = {};
			vboAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
			auto vbo = createSharedBufferObject(storageBufferInfo, vboAllocInfo);

			vk::BufferCopy region = {};
			region.size = vSize;
			copy(vbo->buffer, stagebuffer->buffer, region);

			return vbo;
		}

		return 0;
	}
}