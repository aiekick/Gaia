#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

#include <ctools/cTools.h>

#define USE_VSYNC 1
#define SWAPCHAIN_IMAGES_COUNT 3 // tripple buffering is interesting when vsync is enabled

// the deph here is not compatible wiht imgui viewport who is not suning depth is the wiewport swapchain
// this will create many validation layering issues
//#define SWAPCHAIN_USE_DEPTH

#include <string>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <array>
namespace vkApi
{
	class VulkanSwapChain
	{
	public:
		enum FrameType
		{
			COLOR = 0,
#ifdef SWAPCHAIN_USE_DEPTH
			DEPTH,
#endif
			FrameTypeSize
		};
		struct SwapChainFrameBuffer
		{
			std::array<vk::ImageView, FrameTypeSize> views;
			vk::Framebuffer frameBuffer;
		};
		struct DepthImageMem
		{
			vk::Image image;
			VmaAllocation meta;
			vk::ImageView view;
		};

	private:
		std::function<void()> m_ResizeFunction = 0;

	public:
		// swapchain
		vk::SurfaceKHR m_Surface;
		vk::SwapchainKHR m_Swapchain;
		vk::Rect2D m_RenderArea;
		vk::Viewport m_Viewport;
		vk::Extent2D m_OutputSize;
		vk::Format m_SurfaceColorFormat = vk::Format::eB8G8R8A8Unorm;
		vk::ColorSpaceKHR m_SurfaceColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
#ifdef SWAPCHAIN_USE_DEPTH
		vk::Format m_SurfaceDepthFormat = vk::Format::eD32SfloatS8Uint;
		DepthImageMem m_Depth;
#endif
		vk::SampleCountFlagBits m_SampleCount = vk::SampleCountFlagBits::e1;
		ct::frect m_DisplayRect;
		std::vector<SwapChainFrameBuffer> m_SwapchainFrameBuffers;
		uint32_t m_FrameIndex = 0;

		// sync objects : graphic
		std::vector<vk::Semaphore> m_PresentCompleteSemaphores;
		std::vector<vk::Semaphore> m_RenderCompleteSemaphores;
		std::vector<vk::Fence> m_WaitFences;

		// render pass
		vk::RenderPass m_RenderPass;
#ifdef SWAPCHAIN_USE_DEPTH
		std::array<vk::ClearValue, 2> m_ClearValues;
#else
		std::array<vk::ClearValue, 1> m_ClearValues;
#endif

	public:
		VulkanSwapChain() = default;
		~VulkanSwapChain() = default;

	public:
		void Init(std::function<void()> vResizeFunc);
		void Load();
		void Reload();
		void Unit();

	public: // get
		vk::SurfaceKHR			getSurface() const;
		vk::SwapchainKHR		getSwapchain() const;
		vk::Viewport			getViewport() const;
		vk::Rect2D				getRenderArea() const;
		uint32_t				getSwapchainFrameBuffers() const;
		vk::SampleCountFlagBits getSwapchainFrameBufferSampleCount() const;

	public: // Presentation
		bool AcquireNextImage();
		void Present();

	private:
		void CheckSurfaceFormat();
		void Resize();

		void CreateSurface();
		void CreateSyncObjects();
		void CreateFrameBuffers();
		void CreateRenderPass();

		void DestroySurface();
		void DestroySyncObjects();
		void DestroyFrameBuffers();
		void DestroyRenderPass();
	};
}
