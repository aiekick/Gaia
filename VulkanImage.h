#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

#include <vector>

struct VulkanImageObject
{
	vk::Image image;
	VmaAllocation alloc_meta;
};

namespace vkApi
{
	class VulkanImage
	{
	public:
		static void copy(vk::Image  dst, vk::Buffer src, const vk::BufferImageCopy& region, vk::ImageLayout layout = vk::ImageLayout::eTransferDstOptimal);
		static void copy(vk::Image  dst, vk::Buffer src, const std::vector<vk::BufferImageCopy>& regions, vk::ImageLayout layout = vk::ImageLayout::eTransferDstOptimal);
		static void copy(vk::Buffer dst, vk::Image  src, const vk::BufferImageCopy& region, vk::ImageLayout layout = vk::ImageLayout::eTransferSrcOptimal);
		static void copy(vk::Buffer dst, vk::Image  src, const std::vector<vk::BufferImageCopy>& regions, vk::ImageLayout layout = vk::ImageLayout::eTransferSrcOptimal);

		static std::shared_ptr<VulkanImageObject> createSharedImageObject(const vk::ImageCreateInfo& image_info, const VmaAllocationCreateInfo& alloc_info);
		static std::shared_ptr<VulkanImageObject> createTextureImage2D(uint32_t width, uint32_t height, uint32_t mipLevelCount, vk::Format format, void* hostdata_ptr);
		static std::shared_ptr<VulkanImageObject> createColorAttachment2D(uint32_t width, uint32_t height, uint32_t mipLevelCount, vk::Format format, vk::SampleCountFlagBits vSampleCount);
		static std::shared_ptr<VulkanImageObject> createComputeTarget2D(uint32_t width, uint32_t height, uint32_t mipLevelCount, vk::Format format, vk::SampleCountFlagBits vSampleCount);

		static std::shared_ptr<VulkanImageObject> createDepthAttachment(uint32_t width, uint32_t height, vk::Format format, vk::SampleCountFlagBits vSampleCount);

		static void GenerateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

		static void transitionImageLayout(vk::Image image, vk::Format format, uint32_t mipLevel, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
		static void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageSubresourceRange subresourceRange);

		static bool hasStencilComponent(vk::Format format);

		static void getDatasFromTextureImage2D(uint32_t width, uint32_t height, vk::Format format, std::shared_ptr<VulkanImageObject> vImage, void* vDatas, uint32_t* vSize);
	};
}