/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <ctools/cTools.h>
#include <ctools/Logger.h>
#include <Gaia/Gui/ImGuiTexture.h>
#include <Gaia/Gui/VulkanImGuiRenderer.h>
#include <Gaia/Resources/VulkanComputeImageTarget.h>
#include <Gaia/Resources/VulkanFrameBufferAttachment.h>

#ifdef PROFILER_INCLUDE
#include <vulkan/vulkan.hpp>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

ImGuiTexturePtr ImGuiTexture::Create()
{
	ImGuiTexturePtr res = std::make_shared<ImGuiTexture>();
	res->m_This = res;
	return res;
}

ImGuiTexture::ImGuiTexture() 
{
	ZoneScoped;
}

ImGuiTexture::~ImGuiTexture()
{
	ZoneScoped;
}

void ImGuiTexture::SetDescriptor(VulkanImGuiRendererWeak vVulkanImGuiRenderer, vk::DescriptorImageInfo* vDescriptorImageInfo, float vRatio) {
	ZoneScoped;

	auto rendPtr = vVulkanImGuiRenderer.lock();
	if (rendPtr)
	{
		if (vDescriptorImageInfo)
		{
			if (IS_FLOAT_DIFFERENT(vRatio, 0.0f))
				ratio = vRatio;

			if (firstLoad)
			{
				descriptor = rendPtr->CreateImGuiTexture(
					(VkSampler)vDescriptorImageInfo->sampler,
					(VkImageView)vDescriptorImageInfo->imageView,
					(VkImageLayout)vDescriptorImageInfo->imageLayout);
				firstLoad = false;
			}
			else
			{
				descriptor = rendPtr->CreateImGuiTexture(
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
}

void ImGuiTexture::SetDescriptor(VulkanImGuiRendererWeak vVulkanImGuiRenderer, GaiApi::VulkanFrameBufferAttachment* vVulkanFrameBufferAttachment) {
	ZoneScoped;

	auto rendPtr = vVulkanImGuiRenderer.lock();
	if (rendPtr)
	{
		if (vVulkanFrameBufferAttachment)
		{
			ratio = (float)vVulkanFrameBufferAttachment->width / (float)vVulkanFrameBufferAttachment->height;

			if (firstLoad)
			{
				descriptor = rendPtr->CreateImGuiTexture(
					(VkSampler)vVulkanFrameBufferAttachment->attachmentDescriptorInfo.sampler,
					(VkImageView)vVulkanFrameBufferAttachment->attachmentDescriptorInfo.imageView,
					(VkImageLayout)vVulkanFrameBufferAttachment->attachmentDescriptorInfo.imageLayout);
				firstLoad = false;
			}
			else
			{
				descriptor = rendPtr->CreateImGuiTexture(
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
}

void ImGuiTexture::SetDescriptor(VulkanImGuiRendererWeak vVulkanImGuiRenderer, GaiApi::VulkanComputeImageTarget* vVulkanComputeImageTarget) {
    ZoneScoped;

    auto rendPtr = vVulkanImGuiRenderer.lock();
    if (rendPtr) {
        if (vVulkanComputeImageTarget) {
            ratio = (float)vVulkanComputeImageTarget->width / (float)vVulkanComputeImageTarget->height;

            if (firstLoad) {
                descriptor = rendPtr->CreateImGuiTexture(                                    //
                    (VkSampler)vVulkanComputeImageTarget->targetDescriptorInfo.sampler,      //
                    (VkImageView)vVulkanComputeImageTarget->targetDescriptorInfo.imageView,  //
                    (VkImageLayout)vVulkanComputeImageTarget->targetDescriptorInfo.imageLayout);
                firstLoad  = false;
            } else {
                descriptor = rendPtr->CreateImGuiTexture(                                        //
                    (VkSampler)vVulkanComputeImageTarget->targetDescriptorInfo.sampler,          //
                    (VkImageView)vVulkanComputeImageTarget->targetDescriptorInfo.imageView,      //
                    (VkImageLayout)vVulkanComputeImageTarget->targetDescriptorInfo.imageLayout,  //
                    &descriptor);
            }

            canDisplayPreview = true;
        } else {
            ClearDescriptor();
        }
    }
}

void ImGuiTexture::ClearDescriptor()
{
	ZoneScoped;

	canDisplayPreview = false;
	firstLoad = true;
	descriptor = vk::DescriptorSet{};
}

void ImGuiTexture::DestroyDescriptor(VulkanImGuiRendererWeak vVulkanImGuiRendererWeak)
{
	ZoneScoped;

	if (!destroyed)
	{
		auto rendPtr = vVulkanImGuiRendererWeak.lock();
		if (rendPtr && rendPtr->DestroyImGuiTexture(&descriptor))
		{
			destroyed = true;
		}
	}
	else
	{
#if _DEBUG
		CTOOL_DEBUG_BREAK;
#endif
	}
}