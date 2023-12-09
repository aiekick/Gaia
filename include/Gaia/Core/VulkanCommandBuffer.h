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

#include <Gaia/gaia.h>
#include <mutex>
#include <thread>

namespace GaiApi {
class VulkanCore;
class GAIA_API VulkanCommandBuffer {
public:
    static vk::CommandBuffer beginSingleTimeCommands(GaiApi::VulkanCoreWeak vVulkanCore, bool begin, vk::CommandPool* vCommandPool = 0);
    static void flushSingleTimeCommands(GaiApi::VulkanCoreWeak vVulkanCore, vk::CommandBuffer& cmd, bool end, vk::CommandPool* vCommandPool = 0);
    static VulkanCommandBuffer CreateCommandBuffer(
        GaiApi::VulkanCoreWeak vVulkanCore, vk::QueueFlagBits vQueueType, vk::CommandPool* vCommandPool = 0);
    static std::mutex VulkanCommandBuffer_Mutex;

public:
    vk::CommandBuffer cmd;
    vk::Fence fence;
    vk::QueueFlagBits type;
    vk::Queue queue;
    uint32_t familyQueueIndex = 0;
    vk::Device device;
    vk::CommandPool commandpool;

private:
    VulkanCoreWeak m_VulkanCore;

public:
    void DestroyCommandBuffer();
    bool ResetFence();
    bool Begin();
    void End();
    bool SubmitCmd(vk::PipelineStageFlags vDstStage);
    bool SubmitCmd(vk::SubmitInfo vSubmitInfo);
};
}  // namespace GaiApi