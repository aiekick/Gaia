#pragma once

#include <ctools/cTools.h>

#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"
#include "VulkanCore.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VulkanImage.h"
#include "VulkanFrameBuffer.h"
#include "VulkanFrameBufferAttachment.h"

#include <string>
#include <vector>

#include <vkProfiler/Profiler.h> // for TracyVkCtx

namespace vkApi
{
	class VulkanFragmentRenderer
	{
	public:
		TracyVkCtx m_TracyContext = 0;

	public:
		vkApi::VulkanQueue m_Queue;
		vk::Device m_Device;
		vk::DescriptorPool m_DescriptorPool;
		vk::CommandPool m_CommandPool;

		ct::uvec2 m_OutputSize;
		uint32_t m_CountColorBuffers = 0;
		uint32_t m_RenderingIterationsCount = 0;

		vk::Rect2D m_RenderArea = {};
		vk::Viewport m_Viewport = {};
		vk::Format m_SurfaceColorFormat = vk::Format::eR32G32B32A32Sfloat;
		std::vector<vk::Semaphore> m_RenderCompleteSemaphores;
		std::vector<vk::Fence> m_WaitFences;
		std::vector<vk::CommandBuffer> m_CommandBuffers;
		std::vector<vkApi::VulkanFrameBuffer> m_FrameBuffers;

		bool m_UseDepth = false;
		bool m_NeedToClear = false;
		ct::fvec4 m_ClearColor = 0.0f;

		vk::RenderPass m_RenderPass = {};
		std::vector<vk::ClearValue> m_ClearColorValues;

		uint32_t m_CurrentFrame = 0; // current frame of 2d effects
		
		bool m_Loaded = false;
		bool m_CanWeRender = true;
		bool m_FirstRender = true;

	public:
		VulkanFragmentRenderer();
		~VulkanFragmentRenderer();

		bool Init(
			ct::uvec2 vSize,
			uint32_t vCountColorBuffer,
			bool vUseDepth = false,
			bool vNeedToClear = false,
			ct::fvec4 vClearColor = 0.0f,
			vk::CommandPool *vCommandPool = 0,
			vk::DescriptorPool *vDescriptorPool = 0);
		void Unit();

		// render in order
		void Resize(ct::uvec2 vNewSize, uint32_t vCountColorBuffer);
		void ResetFence();
		vk::CommandBuffer* GetCommandBuffer();
		void BeginTracyFrame(const char * vFrameName);
		void ResetCommandBuffer();
		void BeginCommandBuffer();
		void BeginRenderPass();
		void ClearAttachmentsIfNeeded(); // clear if clear is needed internally (set by ClearAttachments)
		void EndRenderPass();
		void EndCommandBuffer();
		void Submit();
		void WaitFence();
		void Swap();
		
		void ClearAttachments(); // set clear flag for clearing at next render
		void SetClearColorValue(ct::fvec4 vColor);

		vk::Viewport GetViewport() const;
		vk::Rect2D GetRenderArea() const;
		vk::DescriptorImageInfo GetBackImageInfo(uint32_t vIndex);
		vk::DescriptorImageInfo GetFrontImageInfo(uint32_t vIndex);
		vkApi::VulkanFrameBuffer* GetBackFbo();
		vkApi::VulkanFrameBuffer* GetFrontFbo();
		std::vector<vkApi::VulkanFrameBufferAttachment>* GetBufferAttachments(uint32_t *vMaxBuffers = 0);

	private:
		bool CreateCommanBuffer();
		void DestroyCommanBuffer();
		bool CreateSyncObjects();
		void DestroySyncObjects();
		bool CreateFrameBuffers(
			ct::uvec2 vSize,
			uint32_t vCountColorBuffer,
			bool vUseDepth = false,
			bool vNeedToClear = false,
			ct::fvec4 vClearColor = 0.0f);
		void DestroyFrameBuffers();
	};
}
