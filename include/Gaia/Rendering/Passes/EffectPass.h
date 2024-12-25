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
#pragma warning(disable : 4275)

#include <Gaia/gaia.h>
#include <Gaia/Rendering/Base/ShaderPass.h>
#include <Gaia/Interfaces/EffectInterface.h>
#include <Gaia/Interfaces/Texture2DInputInterface.h>
#include <Gaia/Interfaces/Texture2DOutputInterface.h>

/*
An effect have at least one texture as input and one texture as output
*/

template <size_t size_of_array>
class EffectPass : public ShaderPass, public EffectInterface, public Texture2DInputInterface<size_of_array>, public Texture2DOutputInterface {
public:
    EffectPass(GaiApi::VulkanCoreWeak vVulkanCore) : ShaderPass(vVulkanCore) {
    }
    virtual ~EffectPass() = default;

protected:
    void UpdateRessourceDescriptor() override {
        if (m_EffectEnabled != m_LastEffectEnabled) {
            NeedNewUBOUpload();
            m_LastEffectEnabled = m_EffectEnabled;
        }
        ShaderPass::UpdateRessourceDescriptor();
    }
};
