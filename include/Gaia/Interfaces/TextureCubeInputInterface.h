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

#include <map>
#include <memory>
#include <Gaia/gaia.h>
#include <ImWidgets.h>
#include <Gaia/Core/VulkanCore.h>
#include <Gaia/Gui/ImGuiTexture.h>

template<size_t size_of_array>
class TextureCubeInputInterface 
{
protected:
	std::array<vk::DescriptorImageInfo, size_of_array> m_ImageCubeInfos;
	std::array<ez::fvec2, size_of_array> m_ImageCubeInfosSize;
	std::array<std::array<ImGuiTexture, 6U>, size_of_array> m_ImGuiTextureCubes;

public:
    virtual void SetTextureCube(const uint32_t& vBindingPoint, vk::DescriptorImageInfo* vImageInfo, ez::fvec2* vTextureCubeSize) = 0;

protected: // internal use
	void DrawInputTextureCube(GaiApi::VulkanCoreWeak vVKCore, const char* vLabel, const uint32_t& vIdx, const float& vRatio);
};

template<size_t size_of_array>
void TextureCubeInputInterface<size_of_array>::DrawInputTextureCube(
    GaiApi::VulkanCoreWeak vVKCore, const char* vLabel, const uint32_t& vIdx, const float& vRatio) {
    if (!vVKCore.expired() && vLabel && vIdx <= (uint32_t)size_of_array) {
        auto corePtr = vVKCore.lock();
        assert(corePtr != nullptr);
        auto imguiRendererPtr = corePtr->GetVulkanImGuiRenderer().lock();
		if (imguiRendererPtr) {
			if (ImGui::CollapsingHeader(vLabel)) {
				m_ImGuiTextureCubes[(size_t)vIdx].SetDescriptor(imguiRendererPtr,
					&m_ImageCubeInfos[(size_t)vIdx], vRatio);
				if (m_ImGuiTextureCubes[(size_t)vIdx].canDisplayPreview) {
					int w = (int)ImGui::GetContentRegionAvail().x;
					auto rect = ez::GetScreenRectWithRatio<int32_t>(m_ImGuiTextureCubes[(size_t)vIdx].ratio, ez::ivec2(w, w), false);
					ImGui::ImageRect((ImTextureID)&m_ImGuiTextureCubes[(size_t)vIdx].descriptor,
						ImVec2((float)rect.x, (float)rect.y),
						ImVec2((float)rect.w, (float)rect.h));
				}
			}
		}
	}
}