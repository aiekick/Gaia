#pragma once

#include <vulkan/vulkan.hpp>
#include <unordered_map>

#define VULKAN_DEBUG 1
#define VULKAN_DEBUG_FEATURES 0
#define VULKAN_GPU_ID 0

namespace vkApi
{
	struct VulkanQueue
	{
		uint32_t familyQueueIndex;
		vk::Queue vkQueue;
		vk::CommandPool cmdPools;
	};
	class VulkanDevice
	{
	public:
		std::unordered_map<vk::QueueFlagBits, VulkanQueue> m_Queues;

	public:
		vk::Instance m_Instance;
		vk::DispatchLoaderDynamic m_Dldy;
		vk::DebugReportCallbackEXT m_DebugReport;
		vk::PhysicalDeviceFeatures m_PhysDeviceFeatures;
		vk::PhysicalDevice m_PhysDevice;
		vk::Device m_LogDevice;

	public:
		static void findBestExtensions(const std::vector<vk::ExtensionProperties>& installed, const std::vector<const char*>& wanted, std::vector<const char*>& out);
		static void findBestLayers(const std::vector<vk::LayerProperties>& installed, const std::vector<const char*>& wanted, std::vector<const char*>& out);
		static uint32_t getQueueIndex(vk::PhysicalDevice& physicalDevice, vk::QueueFlags flags, bool standalone);

	public:
		VulkanDevice();
		~VulkanDevice();

		bool Init();
		void Unit();

		void WaitIdle();

		VulkanQueue getQueue(vk::QueueFlagBits vQueueType);

	private:
		void CreateVulkanInstance();
		void DestroyVulkanInstance();

		void CreatePhysicalDevice();
		void DestroyPhysicalDevice();

		void CreateLogicalDevice();
		void DestroyLogicalDevice();
	};
}
