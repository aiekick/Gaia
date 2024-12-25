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

template <size_t size_of_array>
class Texture3DInputInterface {
protected:
    std::array<vk::DescriptorImageInfo, size_of_array> m_Image3DInfos;
    std::array<ez::fvec3, size_of_array> m_Image3DInfosSize;

public:
    virtual void SetTexture3D(const uint32_t& vBindingPoint, vk::DescriptorImageInfo* vImage3DInfo, ez::fvec3* vTextureSize) = 0;
};
