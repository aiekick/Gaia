// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "VulkanFragmentRenderer.h"
#include <ctools/Logger.h>
#include "VulkanSubmitter.h"

#define TRACE_MEMORY
#include <vkProfiler/Profiler.h>

namespace vkApi
{
#define COUNT_BUFFERS 2

	VulkanFragmentRenderer::VulkanFragmentRenderer()
	{
		ZoneScoped;
	}

	VulkanFragmentRenderer::~VulkanFragmentRenderer()
	{
		ZoneScoped;
	}

	bool VulkanFragmentRenderer::Init(
		ct::uvec2 vSize,
		uint32_t vCountColorBuffer,
		bool vUseDepth,
		bool vNeedToClear,
		ct::fvec4 vClearColor,
		vk::CommandPool* vCommandPool,
		vk::DescriptorPool* vDescriptorPool)
	{
		ZoneScoped;

		m_Loaded = false;

		ct::uvec2 size = ct::clamp(vSize, 1u, 8192u);
		if (!size.emptyOR())
		{
			m_Device = VulkanCore::Instance()->getDevice();
			m_Queue = VulkanCore::Instance()->getQueue(vk::QueueFlagBits::eGraphics);
			m_DescriptorPool = VulkanCore::Instance()->getDescriptorPool();
			m_CommandPool = m_Queue.cmdPools;

			if (vCommandPool) m_CommandPool = *vCommandPool;
			if (vDescriptorPool) m_DescriptorPool = *vDescriptorPool;

			m_OutputSize = size.x;
			m_RenderArea = vk::Rect2D(vk::Offset2D(), vk::Extent2D(m_OutputSize.x, m_OutputSize.y));
			m_Viewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(m_OutputSize.x), static_cast<float>(m_OutputSize.y), 0, 1.0f);

			m_UseDepth = vUseDepth;
			m_NeedToClear = vNeedToClear;
			m_ClearColor = vClearColor;

			if (CreateFrameBuffers(vSize, vCountColorBuffer, m_UseDepth, m_NeedToClear, m_ClearColor)) // renderpass est créé dans createFrameBuffers
				if (CreateCommanBuffer())
					if (CreateSyncObjects())
						m_Loaded = true;
		}

		if (m_Loaded)
		{
			m_TracyContext = TracyVkContext(
				VulkanCore::Instance()->getPhysicalDevice(),
				m_Device,
				m_Queue.vkQueue,
				m_CommandBuffers[0]);
		}

		return m_Loaded;
	}

	void VulkanFragmentRenderer::Unit()
	{
		ZoneScoped;

		m_Device.waitIdle();

		DestroySyncObjects();
		DestroyCommanBuffer();
		DestroyFrameBuffers();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//// PUBLIC / RESIZE ///////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void VulkanFragmentRenderer::Resize(ct::uvec2 vNewSize, uint32_t vCountColorBuffer)
	{
		ZoneScoped;

		if (!m_Loaded) return;

		if (m_Loaded)
		{
			TracyVkDestroy(m_TracyContext);
		}

		DestroyFrameBuffers();
		CreateFrameBuffers(vNewSize, vCountColorBuffer, m_UseDepth, m_NeedToClear, m_ClearColor);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//// PUBLIC / RENDER ///////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void VulkanFragmentRenderer::ResetFence()
	{
		ZoneScoped;

		if (!m_Loaded) return;

		m_Device.resetFences(1, &m_WaitFences[m_CurrentFrame]);
	}

	void VulkanFragmentRenderer::WaitFence()
	{
		ZoneScoped;

		if (!m_Loaded) return;

		m_Device.waitForFences(1, &m_WaitFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
	}

	vk::CommandBuffer* VulkanFragmentRenderer::GetCommandBuffer()
	{
		return &m_CommandBuffers[m_CurrentFrame];
	}

	void VulkanFragmentRenderer::BeginTracyFrame(const char* vFrameName)
	{
		FrameMarkNamed(vFrameName);
	}

	void VulkanFragmentRenderer::ResetCommandBuffer()
	{
		auto cmd = GetCommandBuffer();
		cmd->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	}

	void VulkanFragmentRenderer::BeginCommandBuffer()
	{
		auto cmd = GetCommandBuffer();
		cmd->begin(vk::CommandBufferBeginInfo());
		cmd->setViewport(0, 1, &m_Viewport);
		cmd->setScissor(0, 1, &m_RenderArea);

		{
			TracyVkZone(m_TracyContext, *cmd, "Record Command buffer");
		}
	}

	void VulkanFragmentRenderer::BeginRenderPass()
	{
		auto fbo = GetFrontFbo();
		auto cmd = GetCommandBuffer();

		cmd->beginRenderPass(
			vk::RenderPassBeginInfo(
				m_RenderPass,
				fbo->framebuffer,
				m_RenderArea,
				static_cast<uint32_t>(m_ClearColorValues.size()),
				m_ClearColorValues.data()
			),
			vk::SubpassContents::eInline);
	}

	void VulkanFragmentRenderer::ClearAttachmentsIfNeeded()
	{
		auto cmd = GetCommandBuffer();
		auto fbo = GetFrontFbo();
		if (fbo->neverCleared)
		{
			cmd->clearAttachments(
				static_cast<uint32_t>(fbo->attachmentClears.size()),
				fbo->attachmentClears.data(),
				static_cast<uint32_t>(fbo->rectClears.size()),
				fbo->rectClears.data()
			);
			fbo->neverCleared = false;
		}
	}

	void VulkanFragmentRenderer::EndRenderPass()
	{
		auto cmd = GetCommandBuffer();
		cmd->endRenderPass();
	}

	void VulkanFragmentRenderer::EndCommandBuffer()
	{
		auto cmd = GetCommandBuffer();
		{
			TracyVkCollect(m_TracyContext, *cmd);
		}
		cmd->end();
	}

	void VulkanFragmentRenderer::Submit()
	{
		ZoneScoped;

		if (!m_Loaded) return;

		vk::SubmitInfo submitInfo;
		vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		submitInfo
			.setPWaitDstStageMask(&waitDstStageMask)
			.setCommandBufferCount(1)
			.setPCommandBuffers(&m_CommandBuffers[m_CurrentFrame])
			.setSignalSemaphoreCount(1)
			.setPSignalSemaphores(&m_RenderCompleteSemaphores[0]);

		if (!m_FirstRender)
		{
			submitInfo
				.setWaitSemaphoreCount(1)
				.setPWaitSemaphores(&m_RenderCompleteSemaphores[0]);
		}
		else
		{
			m_FirstRender = false;
		}

		VulkanSubmitter::Submit(vk::QueueFlagBits::eGraphics, submitInfo, m_WaitFences[m_CurrentFrame]);
	}

	void VulkanFragmentRenderer::Swap()
	{
		ZoneScoped;

		if (!m_Loaded) return;

		m_CurrentFrame = 1 - m_CurrentFrame;
	}

	void VulkanFragmentRenderer::ClearAttachments()
	{
		for (auto& fbo : m_FrameBuffers)
		{
			fbo.neverCleared = true;
		}
	}

	void VulkanFragmentRenderer::SetClearColorValue(ct::fvec4 vColor)
	{
		if (!m_ClearColorValues.empty())
		{
			m_ClearColorValues[0] = vk::ClearColorValue(std::array<float, 4>{ vColor.x, vColor.y, vColor.z, vColor.w });
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//// PUBLIC / GET //////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	vk::Viewport VulkanFragmentRenderer::GetViewport() const
	{
		ZoneScoped;

		return m_Viewport;
	}

	vk::Rect2D VulkanFragmentRenderer::GetRenderArea() const
	{
		ZoneScoped;

		return m_RenderArea;
	}

	vk::DescriptorImageInfo VulkanFragmentRenderer::GetBackImageInfo(uint32_t vIndex)
	{
		return GetBackFbo()->attachments[0].attachmentDescriptorInfo;
	}

	vk::DescriptorImageInfo VulkanFragmentRenderer::GetFrontImageInfo(uint32_t vIndex)
	{
		return GetFrontFbo()->attachments[0].attachmentDescriptorInfo;
	}

	vkApi::VulkanFrameBuffer* VulkanFragmentRenderer::GetBackFbo()
	{
		ZoneScoped;

		return &m_FrameBuffers[1 - m_CurrentFrame];
	}

	vkApi::VulkanFrameBuffer* VulkanFragmentRenderer::GetFrontFbo()
	{
		ZoneScoped;

		return &m_FrameBuffers[m_CurrentFrame];
	}

	std::vector<vkApi::VulkanFrameBufferAttachment>* VulkanFragmentRenderer::GetBufferAttachments(uint32_t* vMaxBuffers)
	{
		if (vMaxBuffers)
			*vMaxBuffers = (uint32_t)m_FrameBuffers[m_CurrentFrame].attachments.size();
		return &m_FrameBuffers[m_CurrentFrame].attachments;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//// PRIVATE / COMMANDBUFFER ///////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool VulkanFragmentRenderer::CreateCommanBuffer()
	{
		ZoneScoped;

		m_CommandBuffers = m_Device.allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				m_CommandPool,
				vk::CommandBufferLevel::ePrimary,
				COUNT_BUFFERS
			)
		);

		return true;
	}

	void VulkanFragmentRenderer::DestroyCommanBuffer()
	{
		ZoneScoped;

		m_Device.freeCommandBuffers(m_CommandPool, m_CommandBuffers);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//// PRIVATE / SYNC OBJECTS ////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool VulkanFragmentRenderer::CreateSyncObjects()
	{
		ZoneScoped;

		m_RenderCompleteSemaphores.resize(COUNT_BUFFERS);
		m_WaitFences.resize(COUNT_BUFFERS);
		for (size_t i = 0; i < COUNT_BUFFERS; ++i)
		{
			m_RenderCompleteSemaphores[i] = m_Device.createSemaphore(vk::SemaphoreCreateInfo());
			m_WaitFences[i] = m_Device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
		}

		return true;
	}

	void VulkanFragmentRenderer::DestroySyncObjects()
	{
		ZoneScoped;

		for (size_t i = 0; i < COUNT_BUFFERS; ++i)
		{
			m_Device.destroySemaphore(m_RenderCompleteSemaphores[i]);
			m_Device.destroyFence(m_WaitFences[i]);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//// PRIVATE / FRAMEBUFFER /////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool VulkanFragmentRenderer::CreateFrameBuffers(ct::uvec2 vSize, uint32_t vCountColorBuffer, bool vUseDepth, bool vNeedToClear, ct::fvec4 vClearColor)
	{
		ZoneScoped;

		bool res = false;

		if (vCountColorBuffer == 0)
			vCountColorBuffer = m_CountColorBuffers;

		if (vCountColorBuffer > 0 && vCountColorBuffer <= 8)
		{
			ct::uvec2 size = ct::clamp(vSize, 1u, 8192u);
			if (!size.emptyOR())
			{
				m_CountColorBuffers = vCountColorBuffer;
				m_OutputSize = size;
				m_RenderArea = vk::Rect2D(vk::Offset2D(), vk::Extent2D(m_OutputSize.x, m_OutputSize.y));
				m_Viewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(m_OutputSize.x), static_cast<float>(m_OutputSize.y), 0, 1.0f);

				m_ClearColorValues.clear();

				if (vNeedToClear)
				{
					m_ClearColorValues.push_back(vk::ClearColorValue(std::array<float, 4>{ vClearColor.x, vClearColor.y, vClearColor.z, vClearColor.w }));
				}

				if (vUseDepth)
				{
					m_ClearColorValues.push_back(vk::ClearDepthStencilValue(1.0f, 0u));
				}

				m_FrameBuffers.clear();
				m_FrameBuffers.resize(COUNT_BUFFERS);

				res = true;

				for (int i = 0; i < COUNT_BUFFERS; ++i)
				{
					// on cree la rednerpass que pour le 1er fbo, apres on reutilisé la meme
					res &= m_FrameBuffers[i].Init(size, m_CountColorBuffers, m_RenderPass, i == 0, vUseDepth, vNeedToClear, vClearColor);
				}
			}
			else
			{
				LogVarDebug("Size is empty on one chnannel at least : x:%u,y:%u", size.x, size.y);
			}
		}
		else
		{
			LogVarDebug("CountColorBuffer must be between 0 and 8. here => %u", vCountColorBuffer);
		}

		return res;
	}

	void VulkanFragmentRenderer::DestroyFrameBuffers()
	{
		ZoneScoped;

		m_FrameBuffers.clear();

		m_Device.destroyRenderPass(m_RenderPass);
	}
}