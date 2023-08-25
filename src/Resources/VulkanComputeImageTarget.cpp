// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <Gaia/Resources/VulkanComputeImageTarget.h>
#include <Gaia/Core/VulkanCore.h>

#ifdef PROFILER_INCLUDE
#include <vulkan/vulkan.hpp>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

namespace GaiApi {
VulkanComputeImageTarget::~VulkanComputeImageTarget() {
    ZoneScoped;

    Unit();
}

bool VulkanComputeImageTarget::InitTarget2D(  //
    GaiApi::VulkanCorePtr vVulkanCorePtr,     //
    ct::uvec2 vSize,                          //
    vk::Format vFormat,                       //
    uint32_t vMipLevelCount,                  //
    vk::SampleCountFlagBits vSampleCount) {
    ZoneScoped;

    bool res = false;

    m_VulkanCorePtr = vVulkanCorePtr;

    ct::uvec2 size = ct::clamp(vSize, 1u, 8192u);
    if (!size.emptyOR()) {
        mipLevelCount = vMipLevelCount;
        width         = size.x;
        height        = size.y;
        format        = vFormat;
        ratio         = (float)height / (float)width;
        sampleCount   = vSampleCount;

        target = VulkanRessource::createComputeTarget2D(m_VulkanCorePtr, width, height, mipLevelCount, format, sampleCount);

        vk::ImageViewCreateInfo imViewInfo = {};
        imViewInfo.flags                   = vk::ImageViewCreateFlags();
        imViewInfo.image                   = target->image;
        imViewInfo.viewType                = vk::ImageViewType::e2D;
        imViewInfo.format                  = format;
        imViewInfo.components              = vk::ComponentMapping();
        imViewInfo.subresourceRange        = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipLevelCount, 0, 1);
        targetView                         = m_VulkanCorePtr->getDevice().createImageView(imViewInfo);

        vk::SamplerCreateInfo samplerInfo = {};
        samplerInfo.flags                 = vk::SamplerCreateFlags();
        samplerInfo.magFilter             = vk::Filter::eLinear;
        samplerInfo.minFilter             = vk::Filter::eLinear;
        samplerInfo.mipmapMode            = vk::SamplerMipmapMode::eLinear;
        samplerInfo.addressModeU          = vk::SamplerAddressMode::eClampToEdge;  // U
        samplerInfo.addressModeV          = vk::SamplerAddressMode::eClampToEdge;  // V
        samplerInfo.addressModeW          = vk::SamplerAddressMode::eClampToEdge;  // W
        targetSampler                     = m_VulkanCorePtr->getDevice().createSampler(samplerInfo);

        targetDescriptorInfo.sampler     = targetSampler;
        targetDescriptorInfo.imageView   = targetView;
        targetDescriptorInfo.imageLayout = vk::ImageLayout::eGeneral;

        res = true;
    }

    return res;
}

void VulkanComputeImageTarget::Unit() {
    ZoneScoped;

    target.reset();

    if (m_VulkanCorePtr) {
        m_VulkanCorePtr->getDevice().destroyImageView(targetView);
        m_VulkanCorePtr->getDevice().destroySampler(targetSampler);
    } else {
        CTOOL_DEBUG_BREAK;
    }
}

}  // namespace GaiApi
