#pragma once

#include <vulkan/vulkan.hpp>
#include "VulkanImage.h"
#include <ctools/cTools.h>

namespace vkApi
{
	class VulkanComputeImageTarget
	{
	public:
		std::shared_ptr<VulkanImageObject> target = nullptr;
		vk::ImageView targetView = {};
		vk::Sampler targetSampler = {};
		vk::DescriptorImageInfo targetDescriptorInfo = {};
		uint32_t mipLevelCount = 1u;
		uint32_t width = 0u;
		uint32_t height = 0u;
		vk::Format format = vk::Format::eR32G32B32A32Sfloat;
		float ratio = 0.0f;
		bool neverCleared = true;
		vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;

	public:
		VulkanComputeImageTarget() = default;
		~VulkanComputeImageTarget();

	public:
		bool InitTarget2D(ct::uvec2 vSize, vk::Format vFormat, uint32_t vMipLevelCount, vk::SampleCountFlagBits vSampleCount);
		void Unit();
	};
}