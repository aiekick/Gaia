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
    GaiApi::VulkanCoreWeak vVulkanCore,       //
    ez::uvec2 vSize,                          //
    vk::Format vFormat,                       //
    uint32_t vMipLevelCount,                  //
    vk::SampleCountFlagBits vSampleCount) {
    ZoneScoped;

    bool res = false;

    m_VulkanCore = vVulkanCore;

    ez::uvec2 size = ez::clamp(vSize, 1u, 8192u);
    if (!size.emptyOR()) {
        mipLevelCount = vMipLevelCount;
        width = size.x;
        height = size.y;
        format = vFormat;
        ratio = (float)height / (float)width;
        sampleCount = vSampleCount;

        target = VulkanRessource::createComputeTarget2D(m_VulkanCore, width, height, mipLevelCount, format, sampleCount, "VulkanComputeImageTarget");

        auto corePtr = m_VulkanCore.lock();
        assert(corePtr != nullptr);

        vk::ImageViewCreateInfo imViewInfo = {};
        imViewInfo.flags = vk::ImageViewCreateFlags();
        imViewInfo.image = target->image;
        imViewInfo.viewType = vk::ImageViewType::e2D;
        imViewInfo.format = format;
        imViewInfo.components = vk::ComponentMapping();
        imViewInfo.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipLevelCount, 0, 1);
        targetView = corePtr->getDevice().createImageView(imViewInfo);

        vk::SamplerCreateInfo samplerInfo = {};
        samplerInfo.flags = vk::SamplerCreateFlags();
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;  // U
        samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;  // V
        samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;  // W
        targetSampler = corePtr->getDevice().createSampler(samplerInfo);

        targetDescriptorInfo.sampler = targetSampler;
        targetDescriptorInfo.imageView = targetView;
        targetDescriptorInfo.imageLayout = vk::ImageLayout::eGeneral;

        res = true;
    }

    return res;
}

void VulkanComputeImageTarget::Unit() {
    ZoneScoped;

    target.reset();

    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);

    corePtr->getDevice().destroyImageView(targetView);
    corePtr->getDevice().destroySampler(targetSampler);
}

}  // namespace GaiApi
