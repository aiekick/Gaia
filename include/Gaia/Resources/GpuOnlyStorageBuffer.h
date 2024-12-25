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

#include <vector>

#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezLog.hpp>

#include <Gaia/gaia.h>
#include <Gaia/Core/VulkanCore.h>
#include <Gaia/Resources/VulkanRessource.h>

class GAIA_API GpuOnlyStorageBuffer {
public:
    static GpuOnlyStorageBufferPtr Create(GaiApi::VulkanCoreWeak vVulkanCore);

private:
    GaiApi::VulkanCoreWeak m_VulkanCore;
    VulkanBufferObjectPtr m_BufferObjectPtr = nullptr;
    vk::DescriptorBufferInfo m_DescriptorBufferInfo = {VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};
    uint32_t m_BufferSize = 0U;

public:
    GpuOnlyStorageBuffer(GaiApi::VulkanCoreWeak vVulkanCore);
    ~GpuOnlyStorageBuffer();
    bool CreateBuffer(const uint32_t& vDatasSizeInBytes,
        const uint32_t& vDatasCount,
        const VmaMemoryUsage& vVmaMemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
        const vk::BufferUsageFlags& vBufferUsageFlags = (vk::BufferUsageFlags)0);

    VulkanBufferObjectPtr GetBufferObjectPtr();
    vk::DescriptorBufferInfo* GetBufferInfo();
    const uint32_t& GetBufferSize();
    vk::Buffer* GetVulkanBuffer();
    void DestroyBuffer();

    bool MapMemory(void* vMappedMemory);
    void UnmapMemory();
};