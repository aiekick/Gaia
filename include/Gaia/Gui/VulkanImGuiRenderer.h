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

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <Gaia/gaia.h>

#define ENABLE_AIEKICK_CODE

#ifdef ENABLE_AIEKICK_CODE
#include <Gaia/Core/VulkanCore.h>
#endif
// Implemented features:
//  [X] Renderer: Support for large meshes (64k+ vertices) with 16-bit indices.
//  [x] Platform: Multi-viewport / platform windows. With issues (flickering when creating a new viewport).
//  [X] Renderer: User texture binding. Changes of ImTextureID aren't supported by this backend! See https://github.com/ocornut/imgui/pull/914

// You can copy and use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// The aim of imgui_impl_vulkan.h/.cpp is to be usable in your engine without any modification.
// IF YOU FEEL YOU NEED TO MAKE ANY CHANGE TO THIS CODE, please share them and your feedback at https://github.com/ocornut/imgui/

// Important note to the reader who wish to integrate imgui_impl_vulkan.cpp/.h in their own engine/app.
// - Common ImGui_ImplVulkan_XXX functions and structures are used to interface with imgui_impl_vulkan.cpp/.h.
//   You will use those if you want to use this rendering backend in your engine/app.
// - Helper ImGui_ImplVulkanH_XXX functions and structures are only used by this example (main.cpp) and by
//   the backend itself (imgui_impl_vulkan.cpp), but should PROBABLY NOT be used by your own engine/app code.
// Read comments in imgui_impl_vulkan.h.

#pragma once
#pragma warning(disable : 4251)

#include <ImGuiPack.h>      // GAIA_API

#include <cstdint>

// [Configuration] in order to use a custom Vulkan function loader:
// (1) You'll need to disable default Vulkan function prototypes.
//     We provide a '#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES' convenience configuration flag.
//     In order to make sure this is visible from the imgui_impl_vulkan.cpp compilation unit:
//     - Add '#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES' in your imconfig.h file
//     - Or as a compilation flag in your build system
//     - Or uncomment here (not recommended because you'd be modifying imgui sources!)
//     - Do not simply add it in a .cpp file!
// (2) Call ImGui_ImplVulkan_LoadFunctions() before ImGui_ImplVulkan_Init() with your custom function.
// If you have no idea what this is, leave it alone!
//#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES

// Vulkan includes
#if defined(IMGUI_IMPL_VULKAN_NO_PROTOTYPES) && !defined(VK_NO_PROTOTYPES)
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>
#include <Gaia/gaia.h>

// Initialization data, for ImGui_ImplVulkan_Init()
// [Please zero-clear before use!]
struct GAIA_API ImGui_ImplVulkan_InitInfo {
#ifdef ENABLE_AIEKICK_CODE
    GaiApi::VulkanCorePtr            vulkanCorePtr = nullptr;
#endif
    vk::Instance                      Instance;
    vk::PhysicalDevice PhysicalDevice;
    vk::Device Device;
    uint32_t                        QueueFamily;
    vk::Queue Queue;
    vk::PipelineCache PipelineCache;
    vk::DescriptorPool DescriptorPool;
    uint32_t                        Subpass;
    uint32_t                        MinImageCount;          // >= 2
    uint32_t                        ImageCount;             // >= MinImageCount
    VkSampleCountFlagBits MSAASamples;                    // >= VK_SAMPLE_COUNT_1_BIT
    const vk::AllocationCallbacks* Allocator;
    void                            (*CheckVkResultFn)(VkResult err);
};

// Called by user code
GAIA_API bool     ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass);
GAIA_API void     ImGui_ImplVulkan_Shutdown();
GAIA_API void     ImGui_ImplVulkan_NewFrame();
GAIA_API void     ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer command_buffer, VkPipeline pipeline = VK_NULL_HANDLE);
GAIA_API bool     ImGui_ImplVulkan_CreateFontsTexture(VkCommandBuffer command_buffer);
GAIA_API void     ImGui_ImplVulkan_DestroyFontUploadObjects();
GAIA_API void     ImGui_ImplVulkan_SetMinImageCount(uint32_t min_image_count); // To override MinImageCount after initialization (e.g. if swap chain is recreated)

// Optional: load Vulkan functions with a custom function loader
// This is only useful with IMGUI_IMPL_VULKAN_NO_PROTOTYPES / VK_NO_PROTOTYPES
GAIA_API bool     ImGui_ImplVulkan_LoadFunctions(PFN_vkVoidFunction(*loader_func)(const char* function_name, void* user_data), void* user_data = NULL);

//-------------------------------------------------------------------------
// Internal / Miscellaneous Vulkan Helpers
// (Used by example's main.cpp. Used by multi-viewport features. PROBABLY NOT used by your own engine/app.)
//-------------------------------------------------------------------------
// You probably do NOT need to use or care about those functions.
// Those functions only exist because:
//   1) they facilitate the readability and maintenance of the multiple main.cpp examples files.
//   2) the multi-viewport / platform window implementation needs them internally.
// Generally we avoid exposing any kind of superfluous high-level helpers in the bindings,
// but it is too much code to duplicate everywhere so we exceptionally expose them.
//
// Your engine/app will likely _already_ have code to setup all that stuff (swap chain, render pass, frame buffers, etc.).
// You may read this code to learn about Vulkan, but it is recommended you use you own custom tailored code to do equivalent work.
// (The ImGui_ImplVulkanH_XXX functions do not interact with any of the state used by the regular ImGui_ImplVulkan_XXX functions)
//-------------------------------------------------------------------------

struct ImGui_ImplVulkanH_Frame;
struct ImGui_ImplVulkanH_Window;

// Helpers
GAIA_API void                 ImGui_ImplVulkanH_CreateOrResizeWindow(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, ImGui_ImplVulkanH_Window* wnd, uint32_t queue_family, const VkAllocationCallbacks* allocator, int w, int h, uint32_t min_image_count);
GAIA_API void                 ImGui_ImplVulkanH_DestroyWindow(VkInstance instance, VkDevice device, ImGui_ImplVulkanH_Window* wnd, const VkAllocationCallbacks* allocator);
GAIA_API VkSurfaceFormatKHR   ImGui_ImplVulkanH_SelectSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VkFormat* request_formats, int request_formats_count, VkColorSpaceKHR request_color_space);
GAIA_API VkPresentModeKHR     ImGui_ImplVulkanH_SelectPresentMode(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VkPresentModeKHR* request_modes, int request_modes_count);
GAIA_API int                  ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(VkPresentModeKHR present_mode);
GAIA_API VkDescriptorSet      ImGui_ImplVulkanH_Create_UserTexture_Descriptor(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout, VkDescriptorSet* vExistingDescriptorSet = nullptr);
GAIA_API bool                 ImGui_ImplVulkanH_Destroy_UserTexture_Descriptor(VkDescriptorSet* vVkDescriptorSet);
GAIA_API uint32_t             ImGui_ImplVulkanH_MemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits);

// Helper structure to hold the data needed by one rendering frame
// (Used by example's main.cpp. Used by multi-viewport features. Probably NOT used by your own engine/app.)
// [Please zero-clear before use!]
struct GAIA_API ImGui_ImplVulkanH_Frame {
    VkCommandPool       CommandPool;
    VkCommandBuffer     CommandBuffer;
    VkFence             Fence;
    VkImage             Backbuffer;
    VkImageView         BackbufferView;
    VkFramebuffer       Framebuffer;
};

struct GAIA_API ImGui_ImplVulkanH_FrameSemaphores {
    VkSemaphore         ImageAcquiredSemaphore;
    VkSemaphore         RenderCompleteSemaphore;
};

// Helper structure to hold the data needed by one rendering context into one OS window
// (Used by example's main.cpp. Used by multi-viewport features. Probably NOT used by your own engine/app.)
struct GAIA_API ImGui_ImplVulkanH_Window {
    int                 Width;
    int                 Height;
    VkSwapchainKHR      Swapchain;
    VkSurfaceKHR        Surface;
    VkSurfaceFormatKHR  SurfaceFormat;
    VkPresentModeKHR    PresentMode;
    VkRenderPass        RenderPass;
    VkPipeline          Pipeline;               // The window pipeline may uses a different VkRenderPass than the one passed in ImGui_ImplVulkan_InitInfo
    bool                ClearEnable;
    VkClearValue        ClearValue;
    uint32_t            FrameIndex;             // Current frame being rendered to (0 <= FrameIndex < FrameInFlightCount)
    uint32_t            ImageCount;             // Number of simultaneous in-flight frames (returned by vkGetSwapchainImagesKHR, usually derived from min_image_count)
    uint32_t            SemaphoreIndex;         // Current set of swapchain wait semaphores we're using (needs to be distinct from per frame data)
    ImGui_ImplVulkanH_Frame* Frames;
    ImGui_ImplVulkanH_FrameSemaphores* FrameSemaphores;

    ImGui_ImplVulkanH_Window()
    {
        memset(this, 0, sizeof(*this));
        PresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
        ClearEnable = true;
    }
};

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

class GAIA_API VulkanImGuiRenderer {
public:
    static VulkanImGuiRendererPtr Create(GaiApi::VulkanCoreWeak vVulkanCore, GaiApi::VulkanWindowWeak vVulkanWindow);

public:
    VulkanImGuiRendererWeak m_This;
    ImGui_ImplVulkan_InitInfo m_Info  = {};
    vk::PipelineCache m_PipelineCache = nullptr;
    GaiApi::VulkanCoreWeak m_VulkanCore;
    GaiApi::VulkanWindowWeak m_VulkanWindow;

private:
	VkRenderPass m_RenderPass = VK_NULL_HANDLE;

public:
    bool Init(GaiApi::VulkanCoreWeak vVulkanCore, GaiApi::VulkanWindowWeak vVulkanWindow);
    void Unit();

public:
	void NewFrame();
	void RenderDrawData(ImDrawData* draw_data, VkCommandBuffer command_buffer, VkPipeline pipeline = VK_NULL_HANDLE);
	bool CreateFontsTexture();
	vk::DescriptorSet CreateImGuiTexture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout, vk::DescriptorSet* vExistingDescriptorSet = nullptr);
	bool DestroyImGuiTexture(vk::DescriptorSet* vVkDescriptorSet);

public:
	VulkanImGuiRenderer() = default; // Prevent construction
	VulkanImGuiRenderer(const VulkanImGuiRenderer&) = default; // Prevent construction by copying
	VulkanImGuiRenderer& operator =(const VulkanImGuiRenderer&) { return *this; }; // Prevent assignment
	~VulkanImGuiRenderer() = default; // Prevent unwanted destruction
};
