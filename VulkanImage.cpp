// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "VulkanImage.h"

#include "VulkanCore.h"
#include "VulkanCommandBuffer.h"
#include "VulkanBuffer.h"

#include <VulkanFramework/VulkanLogger.h>

#define TRACE_MEMORY
#include <Helper/Profiler.h>

namespace vkApi
{
	void VulkanImage::copy(vk::Image dst, vk::Buffer src, const vk::BufferImageCopy& region, vk::ImageLayout layout)
	{
		ZoneScoped;

		auto cmd = VulkanCommandBuffer::beginSingleTimeCommands(true);
		cmd.copyBufferToImage(src, dst, layout, region);
		VulkanCommandBuffer::flushSingleTimeCommands(cmd, true);
	}

	void VulkanImage::copy(vk::Buffer dst, vk::Image src, const vk::BufferImageCopy& region, vk::ImageLayout layout)
	{
		ZoneScoped;

		auto cmd = VulkanCommandBuffer::beginSingleTimeCommands(true);
		cmd.copyImageToBuffer(src, layout, dst, region);
		VulkanCommandBuffer::flushSingleTimeCommands(cmd, true);
	}

	void VulkanImage::copy(vk::Image dst, vk::Buffer src, const std::vector<vk::BufferImageCopy>& regions, vk::ImageLayout layout)
	{
		ZoneScoped;

		auto cmd = VulkanCommandBuffer::beginSingleTimeCommands(true);
		cmd.copyBufferToImage(src, dst, layout, regions);
		VulkanCommandBuffer::flushSingleTimeCommands(cmd, true);
	}

	void VulkanImage::copy(vk::Buffer dst, vk::Image src, const std::vector<vk::BufferImageCopy>& regions, vk::ImageLayout layout)
	{
		ZoneScoped;

		auto cmd = VulkanCommandBuffer::beginSingleTimeCommands(true);
		cmd.copyImageToBuffer(src, layout, dst, regions);
		VulkanCommandBuffer::flushSingleTimeCommands(cmd, true);
	}

	bool VulkanImage::hasStencilComponent(vk::Format format)
	{
		ZoneScoped;

		return
			format == vk::Format::eD16UnormS8Uint ||
			format == vk::Format::eD24UnormS8Uint ||
			format == vk::Format::eD32SfloatS8Uint;
	}

	std::shared_ptr<VulkanImageObject> VulkanImage::createSharedImageObject(const vk::ImageCreateInfo& image_info, const VmaAllocationCreateInfo& alloc_info)
	{
		ZoneScoped;

		auto ret = std::shared_ptr<VulkanImageObject>(new VulkanImageObject, [](VulkanImageObject* obj)
		{
			vmaDestroyImage(VulkanCore::Instance()->getMemAllocator(), (VkImage)obj->image, obj->alloc_meta);
		});

		VulkanCore::check_error(vmaCreateImage(VulkanCore::Instance()->getMemAllocator(), (VkImageCreateInfo*)& image_info, &alloc_info, (VkImage*)& ret->image, &ret->alloc_meta, nullptr));
		return ret;
	}

	std::shared_ptr<VulkanImageObject> VulkanImage::createTextureImage2D(uint32_t width, uint32_t height, uint32_t mipLevelCount, vk::Format format, void* hostdata_ptr)
	{
		ZoneScoped;

		uint32_t channels = 0;
		uint32_t elem_size = 0;

		mipLevelCount = ct::maxi(mipLevelCount, 1u);

		switch (format)
		{
		case vk::Format::eB8G8R8A8Unorm:
		case vk::Format::eR8G8B8A8Unorm:
			channels = 4;
			elem_size = 8 / 8;
			break;
		case vk::Format::eB8G8R8Unorm:
		case vk::Format::eR8G8B8Unorm:
			channels = 3;
			elem_size = 8 / 8;
			break;
		case vk::Format::eR8Unorm:
			channels = 1;
			elem_size = 8 / 8;
			break;
		case vk::Format::eD16Unorm:
			channels = 1;
			elem_size = 16 / 8;
			break;
		case vk::Format::eR32G32B32A32Sfloat:
			channels = 4;
			elem_size = 32 / 8;
			break;
		case vk::Format::eR32G32B32Sfloat:
			channels = 3;
			elem_size = 32 / 8;
			break;
		case vk::Format::eR32Sfloat:
			channels = 1;
			elem_size = 32 / 8;
			break;
		default:
			LogVar("unsupported type: %s", vk::to_string(format).c_str());
			throw std::invalid_argument("unsupported fomat type!");
		}

		vk::BufferCreateInfo stagingBufferInfo = {};
		VmaAllocationCreateInfo stagingAllocInfo = {};
		stagingBufferInfo.size = width * height * channels * elem_size;
		stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
		stagingAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
		auto stagebuffer = VulkanBuffer::createSharedBufferObject(stagingBufferInfo, stagingAllocInfo);
		VulkanBuffer::upload(*stagebuffer, hostdata_ptr, width * height * channels * elem_size);

		VmaAllocationCreateInfo image_alloc_info = {};
		image_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
		auto familyQueueIndex = VulkanCore::Instance()->getQueue(vk::QueueFlagBits::eGraphics).familyQueueIndex;
		auto texture = createSharedImageObject(vk::ImageCreateInfo(
			vk::ImageCreateFlags(),
			vk::ImageType::e2D,
			format,
			vk::Extent3D(vk::Extent2D(width, height), 1),
			mipLevelCount, 1u,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::SharingMode::eExclusive,
			1,
			&familyQueueIndex,
			vk::ImageLayout::eUndefined),
			image_alloc_info);

		vk::BufferImageCopy copyParams(
			0u, 0u, 0u,
			vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1),
			vk::Offset3D(0, 0, 0),
			vk::Extent3D(width, height, 1));

		// on va copier que le mip level 0, on fera les autre dans GenerateMipmaps juste apres ce block
		transitionImageLayout(texture->image, format, mipLevelCount, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		copy(texture->image, stagebuffer->buffer, copyParams);
		if (mipLevelCount > 1)
		{
			GenerateMipmaps(texture->image, format, width, height, mipLevelCount);
		}
		else
		{
			transitionImageLayout(texture->image, format, mipLevelCount, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
		}

		return texture;
	}

	void VulkanImage::getDatasFromTextureImage2D(uint32_t width, uint32_t height, vk::Format format, std::shared_ptr<VulkanImageObject> vImage, void *vDatas, uint32_t *vSize)
	{
		UNUSED(width);
		UNUSED(height);
		UNUSED(format);
		UNUSED(vImage);
		UNUSED(vDatas);
		UNUSED(vSize);

		ZoneScoped;	
	}

	std::shared_ptr<VulkanImageObject> VulkanImage::createColorAttachment2D(uint32_t width, uint32_t height, uint32_t mipLevelCount, vk::Format format,	vk::SampleCountFlagBits vSampleCount)
	{
		ZoneScoped;

		mipLevelCount = ct::maxi(mipLevelCount, 1u);

		auto graphicQueue = VulkanCore::Instance()->getQueue(vk::QueueFlagBits::eGraphics);
		auto computeQueue = VulkanCore::Instance()->getQueue(vk::QueueFlagBits::eCompute);
		std::vector<uint32_t> familyIndices = { graphicQueue.familyQueueIndex };
		if (computeQueue.familyQueueIndex != graphicQueue.familyQueueIndex)
			familyIndices.push_back(computeQueue.familyQueueIndex);

		vk::ImageCreateInfo imageInfo = {};
		imageInfo.flags = vk::ImageCreateFlags();
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.format = format;
		imageInfo.extent = vk::Extent3D(width, height, 1);
		imageInfo.mipLevels = mipLevelCount;
		imageInfo.arrayLayers = 1U;
		imageInfo.samples = vSampleCount;
		imageInfo.tiling = vk::ImageTiling::eOptimal;
		imageInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;// | vk::ImageUsageFlagBits::eStorage;
		if (familyIndices.size() > 1)
			imageInfo.sharingMode = vk::SharingMode::eConcurrent;
		else
			imageInfo.sharingMode = vk::SharingMode::eExclusive;
		imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(familyIndices.size());
		imageInfo.pQueueFamilyIndices = familyIndices.data();
		imageInfo.initialLayout = vk::ImageLayout::eUndefined;
		
		VmaAllocationCreateInfo image_alloc_info = {};
		image_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
		
		return createSharedImageObject(imageInfo, image_alloc_info);
	}

	std::shared_ptr<VulkanImageObject> VulkanImage::createComputeTarget2D(uint32_t width, uint32_t height, uint32_t mipLevelCount, vk::Format format, vk::SampleCountFlagBits vSampleCount)
	{
		ZoneScoped;

		mipLevelCount = ct::maxi(mipLevelCount, 1u);

		auto graphicQueue = VulkanCore::Instance()->getQueue(vk::QueueFlagBits::eGraphics);
		auto computeQueue = VulkanCore::Instance()->getQueue(vk::QueueFlagBits::eCompute);
		std::vector<uint32_t> familyIndices = { graphicQueue.familyQueueIndex };
		if (computeQueue.familyQueueIndex != graphicQueue.familyQueueIndex)
			familyIndices.push_back(computeQueue.familyQueueIndex);

		vk::ImageCreateInfo imageInfo = {};
		imageInfo.flags = vk::ImageCreateFlags();
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.format = format;
		imageInfo.extent = vk::Extent3D(width, height, 1);
		imageInfo.mipLevels = mipLevelCount;
		imageInfo.arrayLayers = 1U;
		imageInfo.samples = vSampleCount;
		imageInfo.tiling = vk::ImageTiling::eOptimal;
		imageInfo.usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;
		if (familyIndices.size() > 1)
			imageInfo.sharingMode = vk::SharingMode::eConcurrent;
		else
			imageInfo.sharingMode = vk::SharingMode::eExclusive;
		imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(familyIndices.size());
		imageInfo.pQueueFamilyIndices = familyIndices.data();
		imageInfo.initialLayout = vk::ImageLayout::eGeneral;

		VmaAllocationCreateInfo image_alloc_info = {};
		image_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

		return createSharedImageObject(imageInfo, image_alloc_info);
	}

	std::shared_ptr<VulkanImageObject> VulkanImage::createDepthAttachment(uint32_t width, uint32_t height, vk::Format format, vk::SampleCountFlagBits vSampleCount)
	{
		ZoneScoped;

		auto graphicQueue = VulkanCore::Instance()->getQueue(vk::QueueFlagBits::eGraphics);
		auto computeQueue = VulkanCore::Instance()->getQueue(vk::QueueFlagBits::eCompute);
		std::vector<uint32_t> familyIndices = { graphicQueue.familyQueueIndex };
		if (computeQueue.familyQueueIndex != graphicQueue.familyQueueIndex)
			familyIndices.push_back(computeQueue.familyQueueIndex);

		vk::ImageCreateInfo imageInfo = {};
		imageInfo.flags = vk::ImageCreateFlags();
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.format = format;
		imageInfo.extent = vk::Extent3D(width, height, 1);
		imageInfo.mipLevels = 1U;
		imageInfo.arrayLayers = 1U;
		imageInfo.samples = vSampleCount;
		imageInfo.tiling = vk::ImageTiling::eOptimal;
		imageInfo.usage = 
			vk::ImageUsageFlagBits::eDepthStencilAttachment | 
			vk::ImageUsageFlagBits::eTransferSrc;
		imageInfo.sharingMode = vk::SharingMode::eExclusive;
		imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(familyIndices.size());
		imageInfo.pQueueFamilyIndices = familyIndices.data();
		imageInfo.initialLayout = vk::ImageLayout::eUndefined;

		VmaAllocationCreateInfo image_alloc_info = {};
		image_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

		return createSharedImageObject(imageInfo, image_alloc_info);
	}

	void VulkanImage::GenerateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
	{
		ZoneScoped;

		if (mipLevels > 1)
		{
			auto physDevice = VulkanCore::Instance()->getPhysicalDevice();
			
			// Check if image format supports linear blitting
			vk::FormatProperties formatProperties = physDevice.getFormatProperties(imageFormat);

			if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
			{
				throw std::runtime_error("texture image format does not support linear blitting!");
			}

			vk::CommandBuffer commandBuffer = VulkanCommandBuffer::beginSingleTimeCommands(true);

			vk::ImageMemoryBarrier barrier;
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;

			int32_t mipWidth = texWidth;
			int32_t mipHeight = texHeight;

			for (uint32_t i = 1; i < mipLevels; i++)
			{
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
				barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
				barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
				barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

				commandBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eTransfer,
					vk::DependencyFlags(),
					{},
					{},
					barrier);

				VkImageBlit blit{};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(commandBuffer,
					image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit,
					VK_FILTER_LINEAR);

				barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
				barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
				barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

				commandBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eFragmentShader,
					vk::DependencyFlags(),
					{},
					{},
					barrier);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eFragmentShader,
				vk::DependencyFlags(),
				{},
				{},
				barrier);

			VulkanCommandBuffer::flushSingleTimeCommands(commandBuffer, true);
		}
	}

	void VulkanImage::transitionImageLayout(vk::Image image, vk::Format format, uint32_t mipLevel, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
	{
		ZoneScoped;

		vk::ImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevel;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = 1;

		transitionImageLayout(image, format, oldLayout, newLayout, subresourceRange);
	}

	void VulkanImage::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageSubresourceRange subresourceRange)
	{
		ZoneScoped;

		auto cmd = VulkanCommandBuffer::beginSingleTimeCommands(true);
		vk::ImageMemoryBarrier barrier = {};
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		barrier.subresourceRange = subresourceRange;
		barrier.srcAccessMask = vk::AccessFlags();  //
		barrier.dstAccessMask = vk::AccessFlags();  //

		vk::PipelineStageFlags sourceStage;
		vk::PipelineStageFlags destinationStage;

		// TODO:
		if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
		{
			barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
			if (hasStencilComponent(format))
			{
				barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
			}
		}
		else
		{
			barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		}

		if (oldLayout == vk::ImageLayout::eUndefined &&
			(newLayout == vk::ImageLayout::eTransferDstOptimal ||
				newLayout == vk::ImageLayout::eTransferSrcOptimal))
		{
			barrier.srcAccessMask = vk::AccessFlags();
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
			newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else if (oldLayout == vk::ImageLayout::eUndefined &&
			newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
		{
			barrier.srcAccessMask = vk::AccessFlags();
			barrier.dstAccessMask =
				vk::AccessFlagBits::eDepthStencilAttachmentRead |
				vk::AccessFlagBits::eDepthStencilAttachmentWrite;

			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		}
		else
		{
			LogVarDebug("unsupported layouts: %s, %s",
				vk::to_string(oldLayout).c_str(),
				vk::to_string(newLayout).c_str());
			throw std::invalid_argument("unsupported layout transition!");
		}

		cmd.pipelineBarrier(sourceStage, destinationStage,
			vk::DependencyFlags(),
			{},
			{},
			barrier);

		VulkanCommandBuffer::flushSingleTimeCommands(cmd, true);
	}
}