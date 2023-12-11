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

#include <Gaia/Resources/Texture3D.h>
#include <ctools/Logger.h>

#ifdef PROFILER_INCLUDE
#include <vulkan/vulkan.hpp>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

using namespace GaiApi;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// STATIC /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Texture3DPtr Texture3D::CreateEmptyTexture(GaiApi::VulkanCoreWeak vVulkanCore, ct::uvec3 vSize, vk::Format vFormat) {
    ZoneScoped;

    if (vVulkanCore.expired())
        return nullptr;
    auto res = std::make_shared<Texture3D>(vVulkanCore);

    if (!res->InitEmptyTexture(vSize, vFormat)) {
        res.reset();
    }

    return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Texture3D::Texture3D(GaiApi::VulkanCoreWeak vVulkanCore) : m_VulkanCore(vVulkanCore) {
    ZoneScoped;
}

Texture3D::~Texture3D() {
    ZoneScoped;

    Destroy();
}

bool Texture3D::InitEmptyTexture(const ct::uvec3& vSize, const vk::Format& vFormat) {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);
    m_Loaded = false;

    Destroy();

    std::vector<uint8_t> image_data;

    uint32_t channels = 0;
    uint32_t elem_size = 0;

    m_ImageFormat = vFormat;

    switch (vFormat) {
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
            elem_size = 32 / 8;  // sizeof(float)
            break;
        case vk::Format::eR32G32B32Sfloat:
            channels = 3;
            elem_size = 32 / 8;  // sizeof(float)
            break;
        case vk::Format::eR32Sfloat:
            channels = 1;
            elem_size = 32 / 8;  // sizeof(float)
            break;
        default:
            LogVarDebugInfo("Debug : unsupported type: %s", vk::to_string(vFormat).c_str());
            throw std::invalid_argument("unsupported fomat type!");
    }

    size_t dataSize = vSize.x * vSize.y;
    if (dataSize > 1) {
        image_data.resize(dataSize * channels * elem_size);
        memset(image_data.data(), 0, image_data.size());
    } else {
        image_data.resize(channels * elem_size);
        memset(image_data.data(), 0, image_data.size());
    }

    m_Texture3D = VulkanRessource::createTextureImage3D(m_VulkanCore, vSize.x, vSize.y, vSize.z, vFormat, image_data.data(), "Texture3D");

    vk::ImageViewCreateInfo imViewInfo = {};
    imViewInfo.flags = vk::ImageViewCreateFlags();
    imViewInfo.image = m_Texture3D->image;
    imViewInfo.viewType = vk::ImageViewType::e3D;
    imViewInfo.format = vFormat;
    imViewInfo.components = vk::ComponentMapping();
    imViewInfo.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
    m_TextureView = corePtr->getDevice().createImageView(imViewInfo);

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
    m_Sampler = corePtr->getDevice().createSampler(samplerInfo);

    m_DescriptorImageInfo.sampler = m_Sampler;
    m_DescriptorImageInfo.imageView = m_TextureView;
    m_DescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    m_Loaded = true;

    return m_Loaded;
}

void Texture3D::Destroy() {
    ZoneScoped;

    if (!m_Loaded)
        return;

    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);
    corePtr->getDevice().waitIdle();
    corePtr->getDevice().destroySampler(m_Sampler);
    corePtr->getDevice().destroyImageView(m_TextureView);
    m_Texture3D.reset();

    m_Loaded = false;
}
