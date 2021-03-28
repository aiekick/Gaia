// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "VulkanImGuiOverlay.h"
#include <assert.h>

#include "VulkanCore.h"
#include "VulkanWindow.h"
#include "VulkanCommandBuffer.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include "VulkanImGuiRenderer.h"
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>

#include <FontIcons/CustomFont.cpp>
#include <FontIcons/Roboto_Medium.cpp>
#include <ctools/FileHelper.h>

#define TRACE_MEMORY
#include <vkProfiler/Profiler.h>

namespace vkApi
{
	VulkanImGuiOverlay::VulkanImGuiOverlay()
	{
		ZoneScoped;

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable ViewPort
		io.FontAllowUserScaling = true; // activate zoom feature with ctrl + mousewheel
		io.ConfigWindowsMoveFromTitleBarOnly = true; // can move windows only with titlebar

		// Setup Dear ImGui style
		//ImGui::StyleColorsClassic();
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		m_PipelineCache = VulkanCore::Instance()->getDevice().createPipelineCache(vk::PipelineCacheCreateInfo());

		ImGui_ImplGlfw_InitForVulkan(VulkanWindow::Instance()->WinPtr(), true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = (VkInstance)VulkanCore::Instance()->getInstance();
		init_info.PhysicalDevice = (VkPhysicalDevice)VulkanCore::Instance()->getPhysicalDevice();
		init_info.Device = (VkDevice)VulkanCore::Instance()->getDevice();
		init_info.QueueFamily = VulkanCore::Instance()->getQueue(vk::QueueFlagBits::eGraphics).familyQueueIndex;
		init_info.Queue = (VkQueue)VulkanCore::Instance()->getQueue(vk::QueueFlagBits::eGraphics).vkQueue;
		init_info.PipelineCache = (VkPipelineCache)m_PipelineCache;
		init_info.DescriptorPool = (VkDescriptorPool)VulkanCore::Instance()->getDescriptorPool();
		init_info.Allocator = nullptr;
		init_info.MinImageCount = VulkanCore::Instance()->getSwapchainFrameBuffers();
		init_info.ImageCount = VulkanCore::Instance()->getSwapchainFrameBuffers();
		init_info.MSAASamples = (VkSampleCountFlagBits)VulkanCore::Instance()->getSwapchainFrameBufferSampleCount();
		init_info.CheckVkResultFn = vkApi::VulkanCore::check_error;

		VulkanImGuiRenderer::Instance()->Init(&init_info, (VkRenderPass)VulkanCore::Instance()->getMainRenderPass());

		// load memory font file
		ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_RM, 15.0f);
		static ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
		static const ImWchar icons_ranges[] = { ICON_MIN_NDP, ICON_MAX_NDP, 0 };
		ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_NDP, 15.0f, &icons_config, icons_ranges);

		VulkanImGuiRenderer::Instance()->CreateFontsTexture();
	}

	void VulkanImGuiOverlay::Destroy()
	{
		ZoneScoped;

		VulkanCore::Instance()->getDevice().waitIdle();

		if (m_PipelineCache != vk::PipelineCache(nullptr))
		{
			VulkanCore::Instance()->getDevice().destroyPipelineCache(m_PipelineCache);
		}

		VulkanImGuiRenderer::Instance()->Unit();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void VulkanImGuiOverlay::begin()
	{
		ZoneScoped;

		// Start the Dear ImGui frame
		VulkanImGuiRenderer::Instance()->NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void VulkanImGuiOverlay::end()
	{
		ZoneScoped;

		ImGui::Render();
	}

	bool VulkanImGuiOverlay::render()
	{
		ZoneScoped;

		auto main_draw_datas = ImGui::GetDrawData();
		const bool main_is_minimized = (main_draw_datas->DisplaySize.x <= 0.0f || main_draw_datas->DisplaySize.y <= 0.0f);
		if (!main_is_minimized)
		{
			// Record Imgui Draw Data and draw funcs into command buffer
			VulkanImGuiRenderer::Instance()->RenderDrawData(
				ImGui::GetDrawData(),
				(VkCommandBuffer)VulkanCore::Instance()->getGraphicCommandBuffer());
			return true;
		}

		return false;
	}

	void VulkanImGuiOverlay::drawFPS()
	{
		ZoneScoped;

		const ImGuiWindowFlags fpsWindowFlags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoScrollbar;

		ImGui::Begin("fps", 0, fpsWindowFlags);
		ImGui::SetWindowPos(ImVec2(0, 0));
		ImGui::Text("GUI: Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	void VulkanImGuiOverlay::drawDemo()
	{
		ZoneScoped;

		ImGui::ShowDemoWindow();
	}

	ImGuiIO& VulkanImGuiOverlay::imgui_io()
	{
		ZoneScoped;

		return ImGui::GetIO();
	}
}