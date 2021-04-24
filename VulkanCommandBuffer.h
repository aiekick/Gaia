#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
#include <mutex>
#include <thread>

namespace vkApi
{
	class VulkanCommandBuffer
	{
	public:
		static vk::CommandBuffer beginSingleTimeCommands(bool begin, vk::CommandPool* vCommandPool = 0);
		static void flushSingleTimeCommands(vk::CommandBuffer& cmd, bool end, vk::CommandPool* vCommandPool = 0);
		static VulkanCommandBuffer CreateCommandBuffer(vk::QueueFlagBits vQueueType, vk::CommandPool* vCommandPool = 0);
		static std::mutex VulkanCommandBuffer_Mutex;

	public:
		vk::CommandBuffer cmd;
		vk::Fence fence;
		vk::QueueFlagBits type;
		vk::Queue queue;
		uint32_t familyQueueIndex = 0;
		vk::Device device;
		vk::CommandPool commandpool;

	public:
		void DestroyCommandBuffer();
		void ResetFence();
		bool Begin();
		void End();
		void SubmitCmd(vk::PipelineStageFlags vDstStage);
		void SubmitCmd(vk::SubmitInfo vSubmitInfo);
	};
}