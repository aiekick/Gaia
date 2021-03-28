#pragma once

#include <vulkan/vulkan.hpp>
#include <vkFramework/VulkanFrameBufferAttachment.h>
#include <vkFramework/VulkanComputeImageTarget.h>

class ImGuiTexture
{
public:
	vk::DescriptorSet descriptor = {};
	float ratio = 0.0f;
	bool canDisplayPreview = false;
	bool firstLoad = true;
	bool destroyed = false;

public:
	ImGuiTexture() = default;
	~ImGuiTexture();
	void SetDescriptor(vk::DescriptorImageInfo *vDescriptorImageInfo, float vRatio = 1.0f);
	void SetDescriptor(vkApi::VulkanFrameBufferAttachment *vVulkanFrameBufferAttachment);
	void SetDescriptor(vkApi::VulkanComputeImageTarget *vVulkanComputeImageTarget);
	void ClearDescriptor();
	void DestroyDescriptor();
};