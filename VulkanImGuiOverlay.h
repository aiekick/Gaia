#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>
#include <imgui/imgui.h>

namespace vkApi
{
	class VulkanImGuiOverlay
	{
	public:
		VulkanImGuiOverlay();
		~VulkanImGuiOverlay();

		void Destroy();

		void begin();
		void end();
		virtual bool render();

		void drawFPS();
		void drawDemo();

		ImGuiIO& imgui_io();

	private:
		bool m_IsRecording = false;
		vk::PipelineCache m_PipelineCache = nullptr;
	};
}
