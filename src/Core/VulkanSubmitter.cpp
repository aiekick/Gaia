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

#include <Gaia/Core/VulkanSubmitter.h>
#include <Gaia/Core/VulkanCore.h>
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
	// STATIC
	GAIA_API std::mutex VulkanSubmitter::criticalSectionMutex;

	bool VulkanSubmitter::Submit(
		GaiApi::VulkanCoreWeak vVulkanCore,
		vk::QueueFlagBits vQueueType,
		vk::SubmitInfo vSubmitInfo,
		vk::Fence vWaitFence)
	{
		ZoneScoped;

        auto corePtr = vVulkanCore.lock();
        if (corePtr != nullptr)
		{
			std::unique_lock<std::mutex> lck(VulkanSubmitter::criticalSectionMutex, std::defer_lock);
			lck.lock();
			auto result = corePtr->getQueue(vQueueType).vkQueue.submit(1, &vSubmitInfo, vWaitFence);
			if (result == vk::Result::eErrorDeviceLost)
			{
				// driver lost, we'll crash in this case:
				LogVarError("Driver Lost after submit");
				return false;
			}
			lck.unlock();

			return true;
		}

		return false;
	}
}