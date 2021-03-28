#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
#include <glm/glm.hpp>

#include <ctools/cTools.h>

#include "VulkanWindow.h"
#include "VulkanSwapChain.h"
#include "VulkanDevice.h"

#include <string>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <array>

#include <Profiler/Profiler.h> // for TracyVkCtx

namespace vkApi
{
	class VulkanCore
	{
	protected:
		VulkanSwapChain m_VulkanSwapChain;
		VulkanDevice m_VulkanDevice;
		VmaAllocator m_Allocator;
		TracyVkCtx m_TracyContext = 0;
		
	protected:
		std::vector<vk::CommandBuffer> m_CommandBuffers;
		std::vector<vk::Semaphore> m_ComputeCompleteSemaphores;
		std::vector<vk::Fence> m_ComputeWaitFences;
		std::vector<vk::CommandBuffer> m_ComputeCommandBuffers;
		vk::DescriptorPool m_DescriptorPool;
		vk::PipelineCache m_PipelineCache = nullptr;

	public:
		static void check_error(vk::Result result);
		static void check_error(VkResult result);

	public:
		void Init();
		void Unit();

	public: // get
		vk::Instance getInstance() const;
		vk::PhysicalDevice getPhysicalDevice() const;
		vk::Device getDevice() const;
		vk::DescriptorPool getDescriptorPool() const;
		vk::RenderPass getMainRenderPass() const;
		vk::CommandBuffer getGraphicCommandBuffer() const;
		vk::CommandBuffer getComputeCommandBuffer() const;
		vk::SurfaceKHR getSurface() const;
		VulkanSwapChain getSwapchain() const;
		std::vector<vk::Semaphore> getPresentSemaphores();
		std::vector<vk::Semaphore> getRenderSemaphores();
		vk::Viewport getViewport() const;
		vk::Rect2D getRenderArea() const;
		VmaAllocator getMemAllocator() const;
		VulkanQueue getQueue(vk::QueueFlagBits vQueueType);
		TracyVkCtx getTracyContext();
		vk::SampleCountFlagBits getSwapchainFrameBufferSampleCount() const;

		// from device
		vk::SampleCountFlagBits GetMaxUsableSampleCount();

	public:
		void resize();
		
	public:// graphic
		void frameBegin();
		void beginMainRenderPass();
		void endMainRenderPass();
		void frameEnd();

	public:// compute
		void resetComputeFence();
		bool computeBegin();
		void computeEnd();
		void submitComputeCmd(vk::CommandBuffer vCmd);

	public: // KHR
		bool AcquireNextImage();
		void Present();
		uint32_t getSwapchainFrameBuffers() const;
		bool justGainFocus();
		ct::frect* getDisplayRect();

	protected:
		void setupMemoryAllocator();
		
		void setupGraphicCommandsAndSynchronization();
		void destroyGraphicCommandsAndSynchronization();
		
		void setupComputeCommandsAndSynchronization();
		void destroyComputeCommandsAndSynchronization();
		
		void setupDescriptorPool();

	public: // singleton
		static VulkanCore *Instance()
		{
			static VulkanCore _instance;
			return &_instance;
		}

	protected:
		VulkanCore() {} // Prevent construction
		VulkanCore(const VulkanCore&) {}; // Prevent construction by copying
		VulkanCore& operator =(const VulkanCore&) { return *this; }; // Prevent assignment
		~VulkanCore() {} // Prevent unwanted destruction
	};

	
}

