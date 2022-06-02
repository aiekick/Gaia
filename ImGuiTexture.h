#pragma once

#include <ctools/cTools.h>
#include <vulkan/vulkan.hpp>
#include <vkFramework/VulkanComputeImageTarget.h>
#include <vkFramework/VulkanFrameBufferAttachment.h>

class ImGuiTexture;
typedef std::shared_ptr<ImGuiTexture> ImGuiTexturePtr;
typedef ct::cWeak<ImGuiTexture> ImGuiTextureWeak;

class ImGuiTexture
{
public:
	static ImGuiTexturePtr Create();

public:
	ImGuiTextureWeak m_This;
	vk::DescriptorSet descriptor = {};
	float ratio = 0.0f;
	bool canDisplayPreview = false;
	bool firstLoad = true;
	bool destroyed = false;

public:
	ImGuiTexture() = default;
	~ImGuiTexture();
	void SetDescriptor(vk::DescriptorImageInfo* vDescriptorImageInfo, float vRatio = 1.0f);
	void SetDescriptor(vkApi::VulkanFrameBufferAttachment* vVulkanFrameBufferAttachment);
	void SetDescriptor(vkApi::VulkanComputeImageTarget* vVulkanComputeImageTarget);
	void ClearDescriptor();
	void DestroyDescriptor();
};