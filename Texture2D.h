#pragma once

#include <vulkan/vulkan.hpp>
#include <vkFramework/VulkanCore.h>
#include <vkFramework/VulkanImage.h>
#include <string>

class Texture2D
{
public:
	static bool loadPNG(const std::string& inFile, std::vector<uint8_t>& outBuffer, uint32_t& outWidth, uint32_t& outHeight);
	static bool loadImage(const std::string& inFile, std::vector<uint8_t>& outBuffer, uint32_t& outWidth, uint32_t& outHeight, uint32_t& outChannels);
	static vk::DescriptorImageInfo GetImageInfoFromMemory(uint8_t* buffer, const uint32_t& width, const uint32_t& height, const uint32_t& channels);

public:
	static std::shared_ptr<Texture2D> CreateFromFile(std::string vFilePathName);
	static std::shared_ptr<Texture2D> CreateFromMemory(uint8_t* buffer, const uint32_t& width, const uint32_t& height, const uint32_t& channels);
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
	bool LoadFile(const std::string& vFilePathName, const vk::Format& vFormat = vk::Format::eR8G8B8A8Unorm, const uint32_t& vMipLevelCount = 1u);
	bool LoadMemory(uint8_t* buffer, const uint32_t& width, const uint32_t& height, const uint32_t& channels, const vk::Format& vFormat = vk::Format::eR8G8B8A8Unorm, const uint32_t& vMipLevelCount = 1u);
	bool LoadEmptyTexture(const ct::uvec2& vSize = 1, const vk::Format& vFormat = vk::Format::eR8G8B8A8Unorm);
	bool LoadEmptyImage(const ct::uvec2& vSize = 1, const vk::Format& vFormat = vk::Format::eR8G8B8A8Unorm);
	void Destroy();

public:
	bool SaveToPng(const std::string& vFilePathName, const bool& vFlipY, const int& vSubSamplesCount, const ct::ivec2& vNewSize);
	bool SaveToBmp(const std::string& vFilePathName, const bool& vFlipY, const int& vSubSamplesCount, const ct::ivec2& vNewSize);
	bool SaveToJpg(const std::string& vFilePathName, const bool& vFlipY, const int& vSubSamplesCount, const int& vQualityFrom0To100, const ct::ivec2& vNewSize);
	bool SaveToHdr(const std::string& vFilePathName, const bool& vFlipY, const int& vSubSamplesCount, const ct::ivec2& vNewSize);
	bool SaveToTga(const std::string& vFilePathName, const bool& vFlipY, const int& vSubSamplesCount, const ct::ivec2& vNewSize);
};