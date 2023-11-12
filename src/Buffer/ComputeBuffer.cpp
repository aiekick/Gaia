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

#include <Gaia/Buffer/ComputeBuffer.h>

#include <utility>
#include <functional>

#include <ctools/Logger.h>
#include <ctools/FileHelper.h>
#include <ImWidgets.h>
#include <Gaia/Core/VulkanSubmitter.h>

#ifdef PROFILER_INCLUDE
#include <vulkan/vulkan.hpp>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

using namespace GaiApi;

// #define VERBOSE_DEBUG
// #define BLEND_ENABLED

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / STATIC ///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

ComputeBufferPtr ComputeBuffer::Create(GaiApi::VulkanCorePtr vVulkanCorePtr) {
    ZoneScoped;
    if (!vVulkanCorePtr)
        return nullptr;
    auto res = std::make_shared<ComputeBuffer>(vVulkanCorePtr);

    return res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / CTOR/DTOR ////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

ComputeBuffer::ComputeBuffer(GaiApi::VulkanCorePtr vVulkanCorePtr) {
    ZoneScoped;
    m_VulkanCorePtr = vVulkanCorePtr;
}

ComputeBuffer::~ComputeBuffer() {
    ZoneScoped;
    Unit();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / INIT/UNIT ////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ComputeBuffer::Init(const ct::uvec2& vSize, const uint32_t& vCountColorBuffers, const bool& vPingPongBufferMode, const vk::Format& vFormat) {
    ZoneScoped;

    m_Loaded = false;

    m_Device = m_VulkanCorePtr->getDevice();
    ct::uvec2 size = ct::clamp(vSize, 1u, 8192u);
    if (!size.emptyOR()) {
        m_PingPongBufferMode = vPingPongBufferMode;

        m_TemporarySize = ct::ivec2(size.x, size.y);
        m_TemporaryCountBuffer = vCountColorBuffers;

        m_Queue = m_VulkanCorePtr->getQueue(vk::QueueFlagBits::eGraphics);

        m_OutputSize = ct::uvec3(size.x, size.y, 0);
        m_OutputRatio = ct::fvec2((float)m_OutputSize.x, (float)m_OutputSize.y).ratioXY<float>();

        m_Format = vFormat;

        if (CreateComputeBuffers(vSize, vCountColorBuffers, m_Format)) {
            m_Loaded = true;
        }
    }

    return m_Loaded;
}

void ComputeBuffer::Unit() {
    ZoneScoped;

    m_Device.waitIdle();

    DestroyComputeBuffers();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / RESIZE ///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ComputeBuffer::NeedResize(ct::ivec2* vNewSize, const uint32_t* vCountColorBuffers) {
    ZoneScoped;
    if (vNewSize) {
        m_TemporarySize = *vNewSize;
        m_NeedResize = true;
    }

    if (vCountColorBuffers) {
        m_TemporaryCountBuffer = *vCountColorBuffers;
        m_NeedResize = true;
    }
}

bool ComputeBuffer::ResizeIfNeeded() {
    ZoneScoped;
    if (m_NeedResize && m_Loaded) {
        DestroyComputeBuffers();
        CreateComputeBuffers(ct::uvec2(m_TemporarySize.x, m_TemporarySize.y), m_TemporaryCountBuffer, m_Format);

        m_TemporaryCountBuffer = m_CountBuffers;
        m_TemporarySize = ct::ivec2(m_OutputSize.x, m_OutputSize.y);
        m_OutputRatio = ct::fvec2((float)m_OutputSize.x, (float)m_OutputSize.y).ratioXY<float>();

        m_NeedResize = false;

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / RENDER ///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ComputeBuffer::Begin(vk::CommandBuffer* /*vCmdBufferPtr*/) {
    ZoneScoped;
    if (m_Loaded) {
        return true;
    }

    return false;
}

void ComputeBuffer::End(vk::CommandBuffer* /*vCmdBufferPtr*/) {
    ZoneScoped;
    if (m_Loaded) {
        Swap();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / RENDER ///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ComputeBuffer::Swap() {
    ZoneScoped;
    if (m_PingPongBufferMode) {
        m_CurrentFrame = 1U - m_CurrentFrame;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / GET //////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

float ComputeBuffer::GetOutputRatio() const {
    ZoneScoped;

    return m_OutputRatio;
}

ct::fvec2 ComputeBuffer::GetOutputSize() const {
    ZoneScoped;
    return ct::fvec2((float)m_OutputSize.x, (float)m_OutputSize.y);
}

uint32_t ComputeBuffer::GetBuffersCount() const {
    ZoneScoped;
    return m_CountBuffers;
}

bool ComputeBuffer::IsPingPongBufferMode() const {
    ZoneScoped;
    return m_PingPongBufferMode;
}

vk::DescriptorImageInfo* ComputeBuffer::GetFrontDescriptorImageInfo(const uint32_t& vBindingPoint) {
    ZoneScoped;
    if (vBindingPoint < m_CountBuffers) {
        auto& buffers = m_ComputeBuffers[(size_t)m_CurrentFrame];
        if (vBindingPoint < buffers.size()) {
            return &buffers[(size_t)vBindingPoint]->m_DescriptorImageInfo;
        }

        LogVarError("return nullptr for frame %u", (uint32_t)m_CurrentFrame);
    }

    LogVarError("return nullptr for binding point %u", (uint32_t)vBindingPoint);

    return nullptr;
}

vk::DescriptorImageInfo* ComputeBuffer::GetBackDescriptorImageInfo(const uint32_t& vBindingPoint) {
    ZoneScoped;
    if (vBindingPoint < m_CountBuffers) {
        uint32_t frame = m_CurrentFrame;
        if (m_PingPongBufferMode)
            frame = 1U - frame;

        auto& buffers = m_ComputeBuffers[(size_t)frame];
        if (vBindingPoint < buffers.size()) {
            return &buffers[(size_t)vBindingPoint]->m_DescriptorImageInfo;
        }

        LogVarError("return nullptr for frame %u", (uint32_t)frame);
    }

    LogVarError("return nullptr for binding point %u", (uint32_t)vBindingPoint);

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRIVATE / FRAMEBUFFER /////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ComputeBuffer::CreateComputeBuffers(const ct::uvec2& vSize, const uint32_t& vCountColorBuffers, const vk::Format& vFormat) {
    ZoneScoped;

    bool res = false;

    auto countColorBuffers = vCountColorBuffers;
    if (countColorBuffers == 0)
        countColorBuffers = m_CountBuffers;

    if (countColorBuffers > 0 && countColorBuffers <= 8) {
        ct::uvec2 size = ct::clamp(vSize, 1u, 8192u);
        if (!size.emptyOR()) {
            m_CountBuffers = countColorBuffers;
            m_OutputSize = ct::uvec3(size, 0);
            m_OutputRatio = ct::fvec2((float)m_OutputSize.x, (float)m_OutputSize.y).ratioXY<float>();

            res = true;

            m_ComputeBuffers.clear();
            m_ComputeBuffers.emplace_back(std::vector<Texture2DPtr>{});
            m_ComputeBuffers[0U].resize(m_CountBuffers);
            for (auto& bufferPtr : m_ComputeBuffers[0U]) {
                bufferPtr = Texture2D::CreateEmptyImage(m_VulkanCorePtr, size, vFormat);
                res &= (bufferPtr != nullptr);
            }

            if (m_PingPongBufferMode) {
                m_ComputeBuffers.emplace_back(std::vector<Texture2DPtr>{});
                m_ComputeBuffers[1U].resize(m_CountBuffers);
                for (auto& bufferPtr : m_ComputeBuffers[1U]) {
                    bufferPtr = Texture2D::CreateEmptyImage(m_VulkanCorePtr, size, vFormat);
                    res &= (bufferPtr != nullptr);
                }
            }
        } else {
            LogVarDebugInfo("Debug : Size is empty on one channel at least : x:%u,y:%u", size.x, size.y);
        }
    } else {
        LogVarDebugInfo("Debug : CountColorBuffer must be between 0 and 8. here => %u", vCountColorBuffers);
    }

    return res;
}

void ComputeBuffer::DestroyComputeBuffers() {
    ZoneScoped;

    m_ComputeBuffers.clear();
}
