#pragma once

#include <vulkan/vulkan.hpp>
#include "VulkanImage.h"
#include <ctools/cTools.h>

namespace vkApi
{
	class VulkanFrameBufferAttachment
	{
	public:
		std::shared_ptr<VulkanImageObject> attachment = nullptr;
		vk::ImageView attachmentView = {};
		vk::Sampler attachmentSampler = {};
		vk::DescriptorImageInfo attachmentDescriptorInfo = {};
		vk::AttachmentDescription attachmentDescription = {}; // pour al renderpass
		uint32_t mipLevelCount = 1U;
		uint32_t width = 0u;
		uint32_t height = 0u;
		vk::Format format = vk::Format::eR32G32B32A32Sfloat;
		float ratio = 0.0f;
		vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;

	public:
		VulkanFrameBufferAttachment() = default;
		~VulkanFrameBufferAttachment();

	public:
		bool InitColor2D(ct::uvec2 vSize, vk::Format vFormat, uint32_t vMipLevelCount, bool vNeedToClear, vk::SampleCountFlagBits vSampleCount);
		bool InitDepth(ct::uvec2 vSize, vk::Format vFormat, vk::SampleCountFlagBits vSampleCount);
		void Unit();
	};
}