// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "VulkanComputeImageTarget.h"
#include "VulkanCore.h"

#define TRACE_MEMORY
#include <Helper/Profiler.h>

namespace vkApi
{
	VulkanComputeImageTarget::~VulkanComputeImageTarget()
	{
		ZoneScoped;

		Unit();
	}

	bool VulkanComputeImageTarget::InitTarget2D(ct::uvec2 vSize, vk::Format vFormat, uint32_t vMipLevelCount, vk::SampleCountFlagBits vSampleCount)
	{
		ZoneScoped;

		bool res = false;

		ct::uvec2 size = ct::clamp(vSize, 1u, 8192u);
		if (!size.emptyOR())
		{
			mipLevelCount = vMipLevelCount;
			width = size.x;
			height = size.y;
			format = vFormat;
			ratio = (float)height / (float)width; 
			sampleCount = vSampleCount;

			target = VulkanImage::createComputeTarget2D(width, height, mipLevelCount, format, sampleCount);

			vk::ImageViewCreateInfo imViewInfo = {};
			imViewInfo.flags = vk::ImageViewCreateFlags();
			imViewInfo.image = target->image;
			imViewInfo.viewType = vk::ImageViewType::e2D;
			imViewInfo.format = format;
			imViewInfo.components = vk::ComponentMapping();
			imViewInfo.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipLevelCount, 0, 1);
			targetView = VulkanCore::Instance()->getDevice().createImageView(imViewInfo);

			vk::SamplerCreateInfo samplerInfo = {};
			samplerInfo.flags = vk::SamplerCreateFlags();
			samplerInfo.magFilter = vk::Filter::eLinear;
			samplerInfo.minFilter = vk::Filter::eLinear;
			samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
			samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge; // U
			samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge; // V
			samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge; // W
			targetSampler = VulkanCore::Instance()->getDevice().createSampler(samplerInfo);

			targetDescriptorInfo.sampler = targetSampler;
			targetDescriptorInfo.imageView = targetView;
			targetDescriptorInfo.imageLayout = vk::ImageLayout::eGeneral;

			res = true;
		}

		return res;
	}

	void VulkanComputeImageTarget::Unit()
	{
		ZoneScoped;

		target.reset();
		VulkanCore::Instance()->getDevice().destroyImageView(targetView);
		VulkanCore::Instance()->getDevice().destroySampler(targetSampler);
	}
}