// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "VulkanUniformBuffer.h"

#define TRACE_MEMORY
#include <vkProfiler/Profiler.h>

namespace vkApi
{
	VulkanUniformBuffer::VulkanUniformBuffer()
	{
		ZoneScoped;
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		ZoneScoped;
	}
}