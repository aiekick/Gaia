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

#include <ezlibs/ezTools.hpp>
#include <vulkan/vulkan.hpp>
#include <Gaia/gaia.h>

class ImGuiTexture;
typedef std::shared_ptr<ImGuiTexture> ImGuiTexturePtr;
typedef std::weak_ptr<ImGuiTexture> ImGuiTextureWeak;

namespace GaiApi {
class VulkanFrameBufferAttachment;
class VulkanComputeImageTarget;
}  // namespace GaiApi

class VulkanImGuiRenderer;
class GAIA_API ImGuiTexture {
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
    ImGuiTexture();
    ~ImGuiTexture();
    void SetDescriptor(VulkanImGuiRendererWeak vVulkanImGuiRenderer, vk::DescriptorImageInfo* vDescriptorImageInfo, float vRatio = 1.0f);
    void SetDescriptor(VulkanImGuiRendererWeak vVulkanImGuiRenderer, GaiApi::VulkanFrameBufferAttachment* vVulkanFrameBufferAttachment);
    void SetDescriptor(VulkanImGuiRendererWeak vVulkanImGuiRenderer, GaiApi::VulkanComputeImageTarget* vVulkanComputeImageTarget);
    void ClearDescriptor();
    void DestroyDescriptor(VulkanImGuiRendererWeak vVulkanImGuiRenderer);
};