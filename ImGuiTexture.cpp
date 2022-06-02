#include "ImGuiTexture.h"
#include "VulkanImGuiRenderer.h"
#include <ctools/cTools.h>
#include <ctools/Logger.h>

#define TRACE_MEMORY
#include <vkProfiler/Profiler.h>

ImGuiTexturePtr ImGuiTexture::Create()
{
	ImGuiTexturePtr res = std::make_shared<ImGuiTexture>();
	res->m_This = res;
	return res;
}

ImGuiTexture::~ImGuiTexture()
{
	ZoneScoped;
}

void ImGuiTexture::SetDescriptor(vk::DescriptorImageInfo* vDescriptorImageInfo, float vRatio)
{
	ZoneScoped;

	if (vDescriptorImageInfo)
	{
		if (IS_FLOAT_DIFFERENT(vRatio, 0.0f))
			ratio = vRatio;

		if (firstLoad)
		{
			descriptor = VulkanImGuiRenderer::Instance()->CreateImGuiTexture(
				(VkSampler)vDescriptorImageInfo->sampler,
				(VkImageView)vDescriptorImageInfo->imageView,
				(VkImageLayout)vDescriptorImageInfo->imageLayout);
			firstLoad = false;

			//LogVarDebug("Debug : New imGui Descriptor");
		}
		else
		{
			descriptor = VulkanImGuiRenderer::Instance()->CreateImGuiTexture(
				(VkSampler)vDescriptorImageInfo->sampler,
				(VkImageView)vDescriptorImageInfo->imageView,
				(VkImageLayout)vDescriptorImageInfo->imageLayout,
				&descriptor);
		}

		canDisplayPreview = true;
	}
	else
	{
		ClearDescriptor();
	}
}

void ImGuiTexture::SetDescriptor(vkApi::VulkanFrameBufferAttachment* vVulkanFrameBufferAttachment)
{
	ZoneScoped;

	if (vVulkanFrameBufferAttachment)
	{
		ratio = (float)vVulkanFrameBufferAttachment->width / (float)vVulkanFrameBufferAttachment->height;

		if (firstLoad)
		{
			descriptor = VulkanImGuiRenderer::Instance()->CreateImGuiTexture(
				(VkSampler)vVulkanFrameBufferAttachment->attachmentDescriptorInfo.sampler,
				(VkImageView)vVulkanFrameBufferAttachment->attachmentDescriptorInfo.imageView,
				(VkImageLayout)vVulkanFrameBufferAttachment->attachmentDescriptorInfo.imageLayout);
			firstLoad = false;

			//LogVarDebug("Debug : New imGui Descriptor");
		}
		else
		{
			descriptor = VulkanImGuiRenderer::Instance()->CreateImGuiTexture(
				(VkSampler)vVulkanFrameBufferAttachment->attachmentSampler,
				(VkImageView)vVulkanFrameBufferAttachment->attachmentView,
				(VkImageLayout)vVulkanFrameBufferAttachment->attachmentDescriptorInfo.imageLayout,
				&descriptor);
		}

		canDisplayPreview = true;
	}
	else
	{
		ClearDescriptor();
	}
}

void ImGuiTexture::SetDescriptor(vkApi::VulkanComputeImageTarget* vVulkanComputeImageTarget)
{
	ZoneScoped;

	if (vVulkanComputeImageTarget &&
		vVulkanComputeImageTarget->height > 0)
	{
		ratio = (float)vVulkanComputeImageTarget->width / (float)vVulkanComputeImageTarget->height;

		if (firstLoad)
		{
			descriptor = VulkanImGuiRenderer::Instance()->CreateImGuiTexture(
				(VkSampler)vVulkanComputeImageTarget->targetDescriptorInfo.sampler,
				(VkImageView)vVulkanComputeImageTarget->targetDescriptorInfo.imageView,
				(VkImageLayout)vVulkanComputeImageTarget->targetDescriptorInfo.imageLayout);
			firstLoad = false;

			//LogVarDebug("Debug : New imGui Descriptor");
		}
		else
		{
			descriptor = VulkanImGuiRenderer::Instance()->CreateImGuiTexture(
				(VkSampler)vVulkanComputeImageTarget->targetDescriptorInfo.sampler,
				(VkImageView)vVulkanComputeImageTarget->targetDescriptorInfo.imageView,
				(VkImageLayout)vVulkanComputeImageTarget->targetDescriptorInfo.imageLayout,
				&descriptor);
		}

		canDisplayPreview = true;
	}
	else
	{
		ClearDescriptor();
	}
}

void ImGuiTexture::ClearDescriptor()
{
	ZoneScoped;

	canDisplayPreview = false;
	firstLoad = true;
	descriptor = vk::DescriptorSet{};
	
	//LogVarDebug("Debug : imGui Descriptor Cleared");
}

void ImGuiTexture::DestroyDescriptor()
{
	ZoneScoped;

	if (!destroyed)
	{
		if (VulkanImGuiRenderer::Instance()->DestroyImGuiTexture(&descriptor))
		{
			destroyed = true;

			//LogVarDebug("Debug : imGui Descriptor Destroyed");
		}
	}
	else
	{
#if _DEBUG
		CTOOL_DEBUG_BREAK;
#endif
	}
}