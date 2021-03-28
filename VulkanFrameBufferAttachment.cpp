// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "VulkanFrameBufferAttachment.h"
#include "VulkanCore.h"

#define TRACE_MEMORY
#include <Profiler/Profiler.h>

namespace vkApi
{
	VulkanFrameBufferAttachment::~VulkanFrameBufferAttachment()
	{
		ZoneScoped;

		Unit();
	}

	bool VulkanFrameBufferAttachment::InitColor2D(
		ct::uvec2 vSize,
		vk::Format vFormat,
		uint32_t vMipLevelCount,
		bool vNeedToClear,
		vk::SampleCountFlagBits vSampleCount)
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

			attachment = VulkanImage::createColorAttachment2D(width, height, mipLevelCount, format, sampleCount);

			vk::ImageViewCreateInfo imViewInfo = {};
			imViewInfo.flags = vk::ImageViewCreateFlags();
			imViewInfo.image = attachment->image;
			imViewInfo.viewType = vk::ImageViewType::e2D;
			imViewInfo.format = format;
			imViewInfo.components = vk::ComponentMapping();
			imViewInfo.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipLevelCount, 0, 1);
			attachmentView = VulkanCore::Instance()->getDevice().createImageView(imViewInfo);

			vk::SamplerCreateInfo samplerInfo = {};
			samplerInfo.flags = vk::SamplerCreateFlags();
			samplerInfo.magFilter = vk::Filter::eLinear;
			samplerInfo.minFilter = vk::Filter::eLinear;
			samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
			samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge; // U
			samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge; // V
			samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge; // W
			//samplerInfo.mipLodBias = 0.0f;
			//samplerInfo.anisotropyEnable = false;
			//samplerInfo.maxAnisotropy = 0.0f;
			//samplerInfo.compareEnable = false;
			//samplerInfo.compareOp = vk::CompareOp::eAlways;
			//samplerInfo.minLod = 0.0f;
			//samplerInfo.maxLod = static_cast<float>(m_MipLevelCount);
			//samplerInfo.unnormalizedCoordinates = false;
			attachmentSampler = VulkanCore::Instance()->getDevice().createSampler(samplerInfo);

			attachmentDescriptorInfo.sampler = attachmentSampler;
			attachmentDescriptorInfo.imageView = attachmentView;
			attachmentDescriptorInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

			attachmentDescription.flags = vk::AttachmentDescriptionFlags();
			attachmentDescription.format = format;
			attachmentDescription.samples = vSampleCount;
			if (vNeedToClear)
				attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
			else
				attachmentDescription.loadOp = vk::AttachmentLoadOp::eLoad;
			attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
			attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
			attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
			attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
			attachmentDescription.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

			res = true;
		}

		return res;
	}

	bool VulkanFrameBufferAttachment::InitDepth(ct::uvec2 vSize, vk::Format vFormat, vk::SampleCountFlagBits vSampleCount)
	{
		ZoneScoped;

		bool res = false;

		ct::uvec2 size = ct::clamp(vSize, 1u, 8192u);
		if (!size.emptyOR())
		{
			mipLevelCount = 1U;
			width = size.x;
			height = size.y;
			format = vFormat;
			ratio = (float)height / (float)width;
			sampleCount = vSampleCount;

			attachment = VulkanImage::createDepthAttachment(width, height, format, sampleCount);

			vk::ImageViewCreateInfo imViewInfo = {};
			imViewInfo.flags = vk::ImageViewCreateFlags();
			imViewInfo.image = attachment->image;
			imViewInfo.viewType = vk::ImageViewType::e2D;
			imViewInfo.format = format;
			imViewInfo.components = vk::ComponentMapping();
			imViewInfo.subresourceRange = vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
				0, 
				1, 
				0, 
				1);
			attachmentView = VulkanCore::Instance()->getDevice().createImageView(imViewInfo);

			vk::SamplerCreateInfo samplerInfo = {};
			samplerInfo.flags = vk::SamplerCreateFlags();
			samplerInfo.magFilter = vk::Filter::eLinear;
			samplerInfo.minFilter = vk::Filter::eLinear;
			samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
			samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge; // U
			samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge; // V
			samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge; // W
			//samplerInfo.mipLodBias = 0.0f;
			//samplerInfo.anisotropyEnable = false;
			//samplerInfo.maxAnisotropy = 0.0f;
			//samplerInfo.compareEnable = false;
			//samplerInfo.compareOp = vk::CompareOp::eAlways;
			//samplerInfo.minLod = 0.0f;
			//samplerInfo.maxLod = static_cast<float>(m_MipLevelCount);
			//samplerInfo.unnormalizedCoordinates = false;
			attachmentSampler = VulkanCore::Instance()->getDevice().createSampler(samplerInfo);

			attachmentDescriptorInfo.sampler = attachmentSampler;
			attachmentDescriptorInfo.imageView = attachmentView;
			attachmentDescriptorInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			
			attachmentDescription.flags = vk::AttachmentDescriptionFlags();
			attachmentDescription.format = format;
			attachmentDescription.samples = sampleCount;
			attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
			attachmentDescription.storeOp = vk::AttachmentStoreOp::eDontCare;
			attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
			attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
			attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
			attachmentDescription.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

			res = true;
		}

		return res;
	}

	void VulkanFrameBufferAttachment::Unit()
	{
		ZoneScoped;

		attachment.reset();
		VulkanCore::Instance()->getDevice().destroyImageView(attachmentView);
		VulkanCore::Instance()->getDevice().destroySampler(attachmentSampler);
	}
}