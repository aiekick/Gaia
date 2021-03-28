#pragma once

#include <vulkan/vulkan.hpp>
#include <vkFramework/VulkanCore.h>
#include <vkFramework/VulkanImage.h>
#include <string>

class Texture2D
{
public:
	static bool loadPNG(const std::string& file, std::vector<uint8_t>& buffer, uint32_t& width, uint32_t& height);
	static bool loadImage(const std::string& file, std::vector<uint8_t>& buffer, uint32_t& width, uint32_t& height, uint32_t& channels);

public:
	static std::shared_ptr<Texture2D> CreateFromFile(std::string vFilePathName);
	static std::shared_ptr<Texture2D> CreateEmptyTexture(ct::uvec2 vSize, vk::Format vFormat);
	static std::shared_ptr<Texture2D> CreateEmptyImage(ct::uvec2 vSize, vk::Format vFormat);

public:
	std::shared_ptr<VulkanImageObject> m_Texture2D = nullptr;
	vk::ImageView m_TextureView = {};
	vk::Sampler m_Sampler = {};
	vk::DescriptorImageInfo m_DescriptorImageInfo = {};
	uint32_t m_MipLevelCount = 1u;
	uint32_t m_Width = 0u;
	uint32_t m_Height = 0u;
	float m_Ratio = 0.0f;
	bool m_Loaded = false;

public:
	Texture2D() = default;
	~Texture2D();
	bool LoadFile(std::string vFilePathName, vk::Format vFormat = vk::Format::eR8G8B8A8Unorm, uint32_t vMipLevelCount = 1u);
	bool LoadEmptyTexture(ct::uvec2 vSize = 1, vk::Format vFormat = vk::Format::eR8G8B8A8Unorm);
	bool LoadEmptyImage(ct::uvec2 vSize = 1, vk::Format vFormat = vk::Format::eR8G8B8A8Unorm);
	void Destroy();

public:
	bool SaveToPng(const std::string& vFilePathName, bool vFlipY, int vSubSamplesCount, ct::ivec2 vNewSize);
	bool SaveToBmp(const std::string& vFilePathName, bool vFlipY, int vSubSamplesCount, ct::ivec2 vNewSize);
	bool SaveToJpg(const std::string& vFilePathName, bool vFlipY, int vSubSamplesCount, int vQualityFrom0To100, ct::ivec2 vNewSize);
	bool SaveToHdr(const std::string& vFilePathName, bool vFlipY, int vSubSamplesCount, ct::ivec2 vNewSize);
	bool SaveToTga(const std::string& vFilePathName, bool vFlipY, int vSubSamplesCount, ct::ivec2 vNewSize);
};