#include "ImGuiTexture.h"
#include <ctools/cTools.h>
#include <ctools/Logger.h>

#ifdef VULKAN
////#include <vkFramework/VulkanImGuiRenderer.h>
#endif

#include <vkProfiler/Profiler.h>

ImGuiTexture::~ImGuiTexture()
{
	ZoneScoped;
}

ImTextureID ImGuiTexture::GetImTextureID()
{
#ifdef VULKAN
	return (ImTextureID)&descriptor;
#else
	return (ImTextureID)(size_t)textureID;
#endif
}

#ifdef VULKAN
void ImGuiTexture::SetDescriptor(vk::DescriptorImageInfo *vDescriptorImageInfo, float vRatio)
{
	ZoneScoped;

	if (vDescriptorImageInfo)
	{
		if (IS_FLOAT_DIFFERENT(vRatio, 0.0f))
			ratio = vRatio;

		/*if (firstLoad)
		{
			descriptor = VulkanImGuiRenderer::Instance()->CreateImGuiTexture(
				vDescriptorImageInfo->sampler,
				vDescriptorImageInfo->imageView, 
				(VkImageLayout)vDescriptorImageInfo->imageLayout);
			firstLoad = false;

			LogVarDebug("New imGui Descriptor");
		}
		else
		{
			descriptor = VulkanImGuiRenderer::Instance()->CreateImGuiTexture(
				vDescriptorImageInfo->sampler,
				vDescriptorImageInfo->imageView,
				(VkImageLayout)vDescriptorImageInfo->imageLayout,
				(VkDescriptorSet*)&descriptor);
		}

		canDisplayPreview = true;*/
	}
	else
	{
		ClearDescriptor();
	}
}

void ImGuiTexture::SetDescriptor(vkApi::VulkanFrameBufferAttachment *vVulkanFrameBufferAttachment)
{
	ZoneScoped;

	if (vVulkanFrameBufferAttachment)
	{
		/*ratio = (float)vVulkanFrameBufferAttachment->width / (float)vVulkanFrameBufferAttachment->height;

		if (firstLoad)
		{
			descriptor = VulkanImGuiRenderer::Instance()->CreateImGuiTexture(
				vVulkanFrameBufferAttachment->attachmentDescriptorInfo.sampler,
				vVulkanFrameBufferAttachment->attachmentDescriptorInfo.imageView,
				(VkImageLayout)vVulkanFrameBufferAttachment->attachmentDescriptorInfo.imageLayout);
			firstLoad = false;

			LogVarDebug("New imGui Descriptor");
		}
		else
		{
			descriptor = VulkanImGuiRenderer::Instance()->CreateImGuiTexture(
				vVulkanFrameBufferAttachment->attachmentSampler,
				vVulkanFrameBufferAttachment->attachmentView,
				(VkImageLayout)vVulkanFrameBufferAttachment->attachmentDescriptorInfo.imageLayout,
				(VkDescriptorSet*)&descriptor);
		}

		canDisplayPreview = true;*/
	}
	else
	{
		ClearDescriptor();
	}
}

void ImGuiTexture::SetDescriptor(vkApi::VulkanComputeImageTarget *vVulkanComputeImageTarget)
{
	ZoneScoped;

	if (vVulkanComputeImageTarget && 
		vVulkanComputeImageTarget->height > 0)
	{
		/*ratio = (float)vVulkanComputeImageTarget->width / (float)vVulkanComputeImageTarget->height;

		if (firstLoad)
		{
			descriptor = VulkanImGuiRenderer::Instance()->CreateImGuiTexture(
				vVulkanComputeImageTarget->targetDescriptorInfo.sampler,
				vVulkanComputeImageTarget->targetDescriptorInfo.imageView,
				(VkImageLayout)vVulkanComputeImageTarget->targetDescriptorInfo.imageLayout);
			firstLoad = false;

			LogVarDebug("New imGui Descriptor");
		}
		else
		{
			descriptor = VulkanImGuiRenderer::Instance()->CreateImGuiTexture(
				vVulkanComputeImageTarget->targetDescriptorInfo.sampler,
				vVulkanComputeImageTarget->targetDescriptorInfo.imageView,
				(VkImageLayout)vVulkanComputeImageTarget->targetDescriptorInfo.imageLayout,
				(VkDescriptorSet*)&descriptor);
		}

		canDisplayPreview = true;*/
	}
	else
	{
		ClearDescriptor();
	}
}
#endif

void ImGuiTexture::ClearDescriptor()
{
	ZoneScoped;

	firstLoad = true;
#ifdef VULKAN
	descriptor = vk::DescriptorSet{};
#endif
	canDisplayPreview = false;

	LogVarDebug("imGui Descriptor Cleared");
}

void ImGuiTexture::DestroyDescriptor()
{
	ZoneScoped;

	if (!destroyed)
	{
#ifdef VULKAN
		/*if (VulkanImGuiRenderer::Instance()->DestroyImGuiTexture(descriptor))
		{
			destroyed = true;

			LogVarDebug("imGui Descriptor Destroyed");
		}*/
#endif
	}
	else
	{
#if _DEBUG
		assert(0);
#endif
	}
	
}