#include "VulkanSubmitter.h"
#include "VulkanCore.h"
#include <ctools/Logger.h>

#define TRACE_MEMORY
#include <vkProfiler/Profiler.h>

namespace vkApi
{
	// STATIC
	std::mutex VulkanSubmitter::criticalSectionMutex;

	bool VulkanSubmitter::Submit(
		vk::QueueFlagBits vQueueType,
		vk::SubmitInfo vSubmitInfo, 
		vk::Fence vWaitFence)
	{
		ZoneScoped;

		std::unique_lock<std::mutex> lck(VulkanSubmitter::criticalSectionMutex, std::defer_lock);
		lck.lock();
		auto result = VulkanCore::Instance()->getQueue(vQueueType).vkQueue.submit(1, &vSubmitInfo, vWaitFence);
		if (result == vk::Result::eErrorDeviceLost)
		{
			// driver lost, we'll crash in this case:
			LogVar("Driver Lost after submit");
			return false;
		}
		lck.unlock();

		return true;
	}
}