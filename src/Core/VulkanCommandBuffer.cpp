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

#include <Gaia/gaia.h>
#include <Gaia/Core/VulkanCommandBuffer.h>
#include <Gaia/Core/VulkanCore.h>
#include <Gaia/Core/VulkanSubmitter.h>
#include <ctools/Logger.h>

#ifdef PROFILER_INCLUDE
#include <vulkan/vulkan.hpp>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

namespace GaiApi
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//// STATIC //////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	std::mutex VulkanCommandBuffer::VulkanCommandBuffer_Mutex;

	vk::CommandBuffer VulkanCommandBuffer::beginSingleTimeCommands(GaiApi::VulkanCoreWeak vVulkanCore, bool begin, vk::CommandPool* vCommandPool)
	{
		ZoneScoped;

        auto corePtr = vVulkanCore.lock();
        assert(corePtr != nullptr);
        auto logDevice = corePtr->getDevice();
        auto queue = corePtr->getQueue(vk::QueueFlagBits::eGraphics);

		std::unique_lock<std::mutex> lck(VulkanCommandBuffer::VulkanCommandBuffer_Mutex, std::defer_lock);
		lck.lock();
		auto allocInfo = vk::CommandBufferAllocateInfo(queue.cmdPools, vk::CommandBufferLevel::ePrimary, 1);
		if (vCommandPool)
			allocInfo.commandPool = *vCommandPool;
		vk::CommandBuffer cmdBuffer = logDevice.allocateCommandBuffers(allocInfo)[0];
		lck.unlock();

		// If requested, also start the new command buffer
		if (begin)
		{
			cmdBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
		}

		return cmdBuffer;
	}

	void VulkanCommandBuffer::flushSingleTimeCommands(GaiApi::VulkanCoreWeak vVulkanCore, vk::CommandBuffer& commandBuffer, bool end, vk::CommandPool* vCommandPool)
	{
		ZoneScoped;

		if (end)
		{
			commandBuffer.end();
		}

        auto corePtr = vVulkanCore.lock();
        assert(corePtr != nullptr);
		VulkanCommandBuffer::VulkanCommandBuffer_Mutex.lock();
		auto logDevice = corePtr->getDevice();
		auto queue = corePtr->getQueue(vk::QueueFlagBits::eGraphics);
		vk::Fence fence = logDevice.createFence(vk::FenceCreateInfo());
		VulkanCommandBuffer::VulkanCommandBuffer_Mutex.unlock();

		auto submitInfos = vk::SubmitInfo(0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr);
		VulkanSubmitter::Submit(vVulkanCore, vk::QueueFlagBits::eGraphics, submitInfos, fence);

		// Wait for the fence to signal that command buffer has finished executing
		if (logDevice.waitForFences(1, &fence, VK_TRUE, UINT_MAX) == vk::Result::eSuccess)
		{
			logDevice.destroyFence(fence);
			if (vCommandPool)
				logDevice.freeCommandBuffers(*vCommandPool, 1, &commandBuffer);
			else
				logDevice.freeCommandBuffers(queue.cmdPools, 1, &commandBuffer);
		}
		
	}

	VulkanCommandBuffer VulkanCommandBuffer::CreateCommandBuffer(GaiApi::VulkanCoreWeak vVulkanCore, vk::QueueFlagBits vQueueType, vk::CommandPool* vCommandPool)
	{
		ZoneScoped;

        auto corePtr = vVulkanCore.lock();
        assert(corePtr != nullptr);

		const auto device = corePtr->getDevice();

		VulkanCommandBuffer commandBuffer = {};

		std::unique_lock<std::mutex> lck(VulkanCommandBuffer::VulkanCommandBuffer_Mutex, std::defer_lock);
		lck.lock();

		if (vQueueType == vk::QueueFlagBits::eGraphics)
		{
			const auto graphicQueue = corePtr->getQueue(vk::QueueFlagBits::eGraphics);
			commandBuffer.queue = graphicQueue.vkQueue;
			commandBuffer.familyQueueIndex = graphicQueue.familyQueueIndex;
			if (vCommandPool) commandBuffer.commandpool = *vCommandPool;
			else commandBuffer.commandpool = graphicQueue.cmdPools;
		}
		else if (vQueueType == vk::QueueFlagBits::eCompute)
		{
			const auto computeQueue = corePtr->getQueue(vk::QueueFlagBits::eCompute);
			commandBuffer.queue = computeQueue.vkQueue;
			commandBuffer.familyQueueIndex = computeQueue.familyQueueIndex;
			if (vCommandPool) commandBuffer.commandpool = *vCommandPool;
			else commandBuffer.commandpool = computeQueue.cmdPools;
		}

		commandBuffer.cmd = device.allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(commandBuffer.commandpool, vk::CommandBufferLevel::ePrimary, 1))[0];
		commandBuffer.fence = device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));

		commandBuffer.type = vQueueType;
		commandBuffer.device = device;

		lck.unlock();

		return commandBuffer;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//// PUBLIC //////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	void VulkanCommandBuffer::DestroyCommandBuffer()
	{
		ZoneScoped;

		device.freeCommandBuffers(commandpool, cmd);
		device.destroyFence(fence);
	}

	bool VulkanCommandBuffer::ResetFence()
	{
		ZoneScoped;

		return (device.resetFences(1, &fence) == vk::Result::eSuccess);
	}

	bool VulkanCommandBuffer::Begin()
	{
		ZoneScoped;

		ResetFence();

		//cmd.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
		cmd.begin(vk::CommandBufferBeginInfo());

		return true;
	}

	void VulkanCommandBuffer::End()
	{
		ZoneScoped;

		cmd.end();
	}

	bool VulkanCommandBuffer::SubmitCmd(vk::PipelineStageFlags vDstStage)
	{
		ZoneScoped;

		vk::SubmitInfo submitInfo;
		submitInfo.setPWaitDstStageMask(&vDstStage).setCommandBufferCount(1).setPCommandBuffers(&cmd);

		VulkanSubmitter::criticalSectionMutex.lock();
		auto result = queue.submit(1, &submitInfo, fence);
		if (result == vk::Result::eErrorDeviceLost)
		{
			LogVarError("Driver seem lost");
		}
		bool res = (device.waitForFences(1, &fence, VK_TRUE, UINT64_MAX) == vk::Result::eSuccess);
		VulkanSubmitter::criticalSectionMutex.unlock();

		return res;
	}

	bool VulkanCommandBuffer::SubmitCmd(vk::SubmitInfo vSubmitInfo)
	{
		ZoneScoped;

		vSubmitInfo.setCommandBufferCount(1).setPCommandBuffers(&cmd);
		VulkanSubmitter::criticalSectionMutex.lock();
		auto result = queue.submit(1, &vSubmitInfo, fence);
		if (result == vk::Result::eErrorDeviceLost)
		{
			LogVarDebugInfo("Debug : Driver seem lost");
		}
		bool res = (device.waitForFences(1, &fence, VK_TRUE, UINT64_MAX) == vk::Result::eSuccess);
		VulkanSubmitter::criticalSectionMutex.unlock();

		return res;
	}
}