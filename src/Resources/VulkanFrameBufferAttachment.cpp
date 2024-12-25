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

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <Gaia/Resources/VulkanFrameBufferAttachment.h>
#include <Gaia/Core/VulkanCore.h>

#ifdef PROFILER_INCLUDE
#include <vulkan/vulkan.hpp>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

namespace GaiApi {
VulkanFrameBufferAttachment::~VulkanFrameBufferAttachment() {
    ZoneScoped;

    Unit();
}

bool VulkanFrameBufferAttachment::InitColor2D(GaiApi::VulkanCoreWeak vVulkanCore,
    ez::uvec2 vSize,
    vk::Format vFormat,
    uint32_t vMipLevelCount,
    bool vNeedToClear,
    vk::SampleCountFlagBits vSampleCount) {
    ZoneScoped;

    m_VulkanCore = vVulkanCore;

    bool res = false;

    ez::uvec2 size = ez::clamp(vSize, 1u, 8192u);
    if (!size.emptyOR()) {
        mipLevelCount = vMipLevelCount;
        width = size.x;
        height = size.y;
        format = vFormat;
        ratio = (float)height / (float)width;
        sampleCount = vSampleCount;

        attachmentPtr =
            VulkanRessource::createColorAttachment2D(m_VulkanCore, width, height, mipLevelCount, format, sampleCount, "VulkanFrameBufferAttachment");

        auto corePtr = m_VulkanCore.lock();
        assert(corePtr != nullptr);

        vk::ImageViewCreateInfo imViewInfo = {};
        imViewInfo.flags = vk::ImageViewCreateFlags();
        imViewInfo.image = attachmentPtr->image;
        imViewInfo.viewType = vk::ImageViewType::e2D;
        imViewInfo.format = format;
        imViewInfo.components = vk::ComponentMapping();
        imViewInfo.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipLevelCount, 0, 1);
        attachmentView = corePtr->getDevice().createImageView(imViewInfo);

        vk::SamplerCreateInfo samplerInfo = {};
        samplerInfo.flags = vk::SamplerCreateFlags();
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;  // U
        samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;  // V
        samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;  // W
        // samplerInfo.mipLodBias = 0.0f;
        // samplerInfo.anisotropyEnable = false;
        // samplerInfo.maxAnisotropy = 0.0f;
        // samplerInfo.compareEnable = false;
        // samplerInfo.compareOp = vk::CompareOp::eAlways;
        // samplerInfo.minLod = 0.0f;
        // samplerInfo.maxLod = static_cast<float>(m_MipLevelCount);
        // samplerInfo.unnormalizedCoordinates = false;
        attachmentSampler = corePtr->getDevice().createSampler(samplerInfo);

        attachmentDescriptorInfo.sampler = attachmentSampler;
        attachmentDescriptorInfo.imageView = attachmentView;
        attachmentDescriptorInfo.imageLayout = vk::ImageLayout::eAttachmentOptimal;

        attachmentDescription.flags = vk::AttachmentDescriptionFlags();
        attachmentDescription.format = format;
        attachmentDescription.samples = vSampleCount;

        if (vNeedToClear) {
            attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
        } else {
            if (sampleCount != vk::SampleCountFlagBits::e1) {
                attachmentDescription.loadOp = vk::AttachmentLoadOp::eDontCare;
            } else {
                attachmentDescription.loadOp = vk::AttachmentLoadOp::eLoad;
            }
        }

        if (sampleCount != vk::SampleCountFlagBits::e1) {
            attachmentDescription.storeOp = vk::AttachmentStoreOp::eDontCare;
        } else {
            attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
        }

        attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
        attachmentDescription.finalLayout = vk::ImageLayout::eAttachmentOptimal;

        res = true;
    }

    return res;
}

bool VulkanFrameBufferAttachment::InitDepth(
    GaiApi::VulkanCoreWeak vVulkanCore, ez::uvec2 vSize, vk::Format vFormat, vk::SampleCountFlagBits vSampleCount) {
    ZoneScoped;

    m_VulkanCore = vVulkanCore;

    bool res = false;

    ez::uvec2 size = ez::clamp(vSize, 1u, 8192u);
    if (!size.emptyOR()) {
        mipLevelCount = 1U;
        width = size.x;
        height = size.y;
        format = vFormat;
        ratio = (float)height / (float)width;
        sampleCount = vSampleCount;

        attachmentPtr = VulkanRessource::createDepthAttachment(m_VulkanCore, width, height, format, sampleCount, "VulkanFrameBufferAttachment");

        auto corePtr = m_VulkanCore.lock();
        assert(corePtr != nullptr);

        vk::ImageViewCreateInfo imViewInfo = {};
        imViewInfo.flags = vk::ImageViewCreateFlags();
        imViewInfo.image = attachmentPtr->image;
        imViewInfo.viewType = vk::ImageViewType::e2D;
        imViewInfo.format = format;
        imViewInfo.components = vk::ComponentMapping();
        imViewInfo.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1);
        attachmentView = corePtr->getDevice().createImageView(imViewInfo);

        vk::SamplerCreateInfo samplerInfo = {};
        samplerInfo.flags = vk::SamplerCreateFlags();
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;  // U
        samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;  // V
        samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;  // W
        // samplerInfo.mipLodBias = 0.0f;
        // samplerInfo.anisotropyEnable = false;
        // samplerInfo.maxAnisotropy = 0.0f;
        // samplerInfo.compareEnable = false;
        // samplerInfo.compareOp = vk::CompareOp::eAlways;
        // samplerInfo.minLod = 0.0f;
        // samplerInfo.maxLod = static_cast<float>(m_MipLevelCount);
        // samplerInfo.unnormalizedCoordinates = false;
        attachmentSampler = corePtr->getDevice().createSampler(samplerInfo);

        attachmentDescriptorInfo.sampler = attachmentSampler;
        attachmentDescriptorInfo.imageView = attachmentView;
        attachmentDescriptorInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        attachmentDescription.flags = vk::AttachmentDescriptionFlags();
        attachmentDescription.format = format;
        attachmentDescription.samples = sampleCount;

        attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;

        if (sampleCount != vk::SampleCountFlagBits::e1) {
            attachmentDescription.storeOp = vk::AttachmentStoreOp::eDontCare;
        } else {
            attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
        }

        attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eClear;
        attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
        attachmentDescription.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        res = true;
    }

    return res;
}

void VulkanFrameBufferAttachment::Unit() {
    ZoneScoped;

    attachmentPtr.reset();

    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);

    corePtr->getDevice().destroyImageView(attachmentView);
    corePtr->getDevice().destroySampler(attachmentSampler);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// MIP MAPPING /////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanFrameBufferAttachment::UpdateMipMapping() {
    if (!m_VulkanCore.expired() && attachmentPtr != nullptr) {
        VulkanRessource::GenerateMipmaps(m_VulkanCore, attachmentPtr->image, format, width, height, mipLevelCount);
        return true;
    }
    return false;
}

}  // namespace GaiApi