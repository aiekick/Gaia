#pragma once

#include <vulkan/vulkan.hpp>
#include "VulkanFrameBufferAttachment.h"
#include <ctools/cTools.h>

namespace vkApi
{
	class VulkanFrameBuffer
	{
	public:
		std::vector<VulkanFrameBufferAttachment> attachments;
		std::vector<vk::ImageView> attachmentViews;
		std::vector<vk::ClearAttachment> attachmentClears;
		std::vector<vk::ClearValue> clearColorValues;
		std::vector<vk::ClearRect> rectClears;
		uint32_t mipLevelCount = 1u;
		uint32_t width = 0u;
		uint32_t height = 0u;
		vk::Format format = vk::Format::eR32G32B32A32Sfloat;
		float ratio = 0.0f;
		vk::Framebuffer framebuffer;
		bool neverCleared = true;
		bool neverToClear = false;
		vk::SampleCountFlagBits sampleCount;
		uint32_t depthAttIndex = 0U;

	public:
		VulkanFrameBuffer();
		~VulkanFrameBuffer();

	public:
		bool Init(
			ct::uvec2 vSize,
			uint32_t vCount,
			vk::RenderPass& vRenderPass,
			bool vCreateRenderPass,
			bool vUseDepth = false,
			bool vNeedToClear = false,
			ct::fvec4 vClearColor = 0.0f,
			vk::SampleCountFlagBits vSampleCount = vk::SampleCountFlagBits::e1);
		void Unit();

		VulkanFrameBufferAttachment* GetDepthAttachment();
	};
}