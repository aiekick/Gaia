#pragma once

#include <vulkan/vulkan.hpp>

// unique_lock::lock/unlock
#include <iostream>       // std::cout
#include <thread>         // std::thread
#include <mutex>          // std::mutex, std::unique_lock, std::defer_lock


namespace vkApi
{
	class VulkanSubmitter
	{
	public:
		static std::mutex criticalSectionMutex;

	public:
		VulkanSubmitter() = default;
		~VulkanSubmitter() = default;

	public:
		static bool Submit(vk::QueueFlagBits vQueueType, vk::SubmitInfo vSubmitInfo, vk::Fence vWaitFence);
	};
}
