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

#pragma once
#pragma warning(disable : 4251)

#include <string>
#include <ctools/cTools.h>
#include <vulkan/vulkan.hpp>
#include <Gaia/Core/VulkanCore.h>
#include <Gaia/Resources/VulkanRessource.h>
#include <Gaia/gaia.h>

class GAIA_API Texture3D {
public:
    static Texture3DPtr CreateEmptyTexture(GaiApi::VulkanCoreWeak vVulkanCore, ct::uvec3 vSize, vk::Format vFormat);

public:
    std::shared_ptr<VulkanImageObject> m_Texture3D = nullptr;
    vk::ImageView m_TextureView = {};
    vk::Sampler m_Sampler = {};
    vk::DescriptorImageInfo m_DescriptorImageInfo = {};
    vk::Format m_ImageFormat = vk::Format::eR8G8B8A8Unorm;
    uint32_t m_Width = 1U;
    uint32_t m_Height = 1U;
    uint32_t m_Depth = 1U;
    bool m_Loaded = false;

private:
    GaiApi::VulkanCoreWeak m_VulkanCore;

public:
    Texture3D(GaiApi::VulkanCoreWeak vVulkanCore);
    ~Texture3D();

    bool InitEmptyTexture(const ct::uvec3& vSize = 1, const vk::Format& vFormat = vk::Format::eR8G8B8A8Unorm);
    void Destroy();
};