// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include <Gaia/Rendering/Passes/QuadShaderPass.h>

#include <Gaia/gaia.h>
#include <Gaia/Resources/VulkanRessource.h>
#include <Gaia/Utils/Mesh/VertexStruct.h>

#ifdef PROFILER_INCLUDE
#include <Gaia/gaia.h>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

QuadShaderPass::QuadShaderPass(GaiApi::VulkanCoreWeak vVulkanCore, const MeshShaderPassType& vMeshShaderPassType)
    : MeshShaderPass<VertexStruct::P2_T2>(vVulkanCore, vMeshShaderPassType) {
    ZoneScoped;
}

QuadShaderPass::QuadShaderPass(GaiApi::VulkanCoreWeak vVulkanCore,
    const MeshShaderPassType& vMeshShaderPassType,
    vk::CommandPool* vCommandPool,
    vk::DescriptorPool* vDescriptorPool)
    : MeshShaderPass<VertexStruct::P2_T2>(vVulkanCore, vMeshShaderPassType, vCommandPool, vDescriptorPool) {
    ZoneScoped;
}

bool QuadShaderPass::BuildModel() {
    ZoneScoped;

    m_Vertices.m_Array = {
        VertexStruct::P2_T2(ez::fvec2(-1.0f, -1.0f), ez::fvec2(0.0f, 0.0f)),
        VertexStruct::P2_T2(ez::fvec2(1.0f, -1.0f), ez::fvec2(1.0f, 0.0f)),
        VertexStruct::P2_T2(ez::fvec2(1.0f, 1.0f), ez::fvec2(1.0f, 1.0f)),
        VertexStruct::P2_T2(ez::fvec2(-1.0f, 1.0f), ez::fvec2(0.0f, 1.0f)),
    };
    m_Indices.m_Array = {0U, 1U, 2U, 0U, 2U, 3U};

    m_Vertices.m_Buffer = GaiApi::VulkanRessource::createVertexBufferObject(m_VulkanCore, m_Vertices.m_Array, false, false, false, "QuadShaderPass");
    m_Vertices.m_Count = (uint32_t)m_Vertices.m_Array.size();
    m_Vertices.m_BufferInfo.buffer = m_Vertices.m_Buffer->buffer;
    m_Vertices.m_BufferInfo.range = m_Vertices.m_Count * sizeof(VertexStruct::P3_N3_TA3_BTA3_T2_C4);
    m_Vertices.m_BufferInfo.offset = 0;

    m_Indices.m_Buffer = GaiApi::VulkanRessource::createIndexBufferObject(m_VulkanCore, m_Indices.m_Array, false, false, false, "QuadShaderPass");
    m_Indices.m_Count = (uint32_t)m_Indices.m_Array.size();
    m_Indices.m_BufferInfo.buffer = m_Indices.m_Buffer->buffer;
    m_Indices.m_BufferInfo.range = m_Indices.m_Count * sizeof(uint32_t);
    m_Indices.m_BufferInfo.offset = 0;

    return true;
}

void QuadShaderPass::DestroyModel(const bool& /*vReleaseDatas*/) {
    ZoneScoped;
    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);

    corePtr->getDevice().waitIdle();

    m_Vertices.m_Buffer.reset();
    m_Indices.m_BufferInfo = vk::DescriptorBufferInfo{};

    m_Indices.m_Buffer.reset();
    m_Indices.m_BufferInfo = vk::DescriptorBufferInfo{};
}

std::string QuadShaderPass::GetVertexShaderCode(std::string& vOutShaderName) {
    ZoneScoped;
    vOutShaderName = "QuadShaderPass_Vertex";

    return u8R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vertPosition;
layout(location = 1) in vec2 vertUv;

layout(location = 0) out vec2 uv_map;

void main() 
{
	uv_map = vertUv;
	gl_Position = vec4(vertPosition, 0.0, 1.0);
}
)";
}

std::string QuadShaderPass::GetFragmentShaderCode(std::string& vOutShaderName) {
    ZoneScoped;
    vOutShaderName = "QuadShaderPass_Fragment";

    return u8R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 uv_map;

void main() 
{
	fragColor = vec4(uv_map, 0.0, 1.0);
}
)";
}