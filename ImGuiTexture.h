#pragma once

#ifdef VULKAN
#include <vulkan/vulkan.hpp>
#include <vkFramework/VulkanFrameBufferAttachment.h>
#include <vkFramework/VulkanComputeImageTarget.h>
#else
#include <glad/glad.h>
#endif

#include <imgui/imgui.h>

class ImGuiTexture
{
public:
#ifdef VULKAN
	vk::DescriptorSet descriptor = {};
#else
	GLuint textureID = 0U;
#endif

	float ratio = 0.0f;
	bool canDisplayPreview = false;
	bool firstLoad = true;
	bool destroyed = false;

public:
	ImGuiTexture() = default;
	~ImGuiTexture();

	ImTextureID GetImTextureID();

#ifdef VULKAN
	void SetDescriptor(vk::DescriptorImageInfo *vDescriptorImageInfo, float vRatio = 1.0f);
	void SetDescriptor(vkApi::VulkanFrameBufferAttachment *vVulkanFrameBufferAttachment);
	void SetDescriptor(vkApi::VulkanComputeImageTarget *vVulkanComputeImageTarget);
#endif
	void ClearDescriptor();
	void DestroyDescriptor();
};