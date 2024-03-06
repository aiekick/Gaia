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

#include <Gaia/Core/VulkanSwapChain.h>
#include <Gaia/Gui/VulkanWindow.h>
#include <Gaia/Core/VulkanCore.h>
#include <ctools/Logger.h>
#include <Gaia/Core/VulkanSubmitter.h>
#include <Gaia/Resources/VulkanFrameBuffer.h>
#include <Gaia/Buffer/FrameBuffer.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef PROFILER_INCLUDE
#include <vulkan/vulkan.hpp>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

namespace GaiApi {
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// STATIC ////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int g_SwapChainResizeWidth = 1280;
static int g_SwapChainResizeHeight = 720;
static bool g_SwapChainRebuild = false;

static void glfw_resize_callback(GLFWwindow*, int w, int h) {
    ZoneScoped;

    g_SwapChainRebuild = true;
    g_SwapChainResizeWidth = w;
    g_SwapChainResizeHeight = h;
}

VulkanSwapChainPtr VulkanSwapChain::Create(VulkanWindowWeak vVulkanWindow, VulkanCoreWeak vVulkanCore, std::function<void()> vResizeFunc) {
    auto res = std::make_shared<VulkanSwapChain>();
    if (!res->Init(vVulkanWindow, vVulkanCore, vResizeFunc)) {
        res.reset();
    }
    return res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// INIT //////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanSwapChain::Init(VulkanWindowWeak vVulkanWindow, VulkanCoreWeak vVulkanCore, std::function<void()> vResizeFunc) {
    ZoneScoped;

    if (!vResizeFunc)
        return false;

    m_ResizeFunction = vResizeFunc;

    m_VulkanWindow = vVulkanWindow;
    m_VulkanCore = vVulkanCore;

    auto winPtr = vVulkanWindow.lock();
    assert(winPtr != nullptr);

    glfwSetFramebufferSizeCallback(winPtr->getWindowPtr(), glfw_resize_callback);

    if (CreateSurface()) {
        if (Load()) {
            if (CreateRenderPass()) {
                if (CreateFrameBuffers()) {
                    return CreateSyncObjects();
                }
            }
        }
    }

    return false;
}

bool VulkanSwapChain::Reload() {
    ZoneScoped;

    DestroySyncObjects();
    DestroyFrameBuffers();
    DestroyRenderPass();

    bool res = true;

    res &= Load();

    res &= CreateRenderPass();
    res &= CreateFrameBuffers();
    res &= CreateSyncObjects();

    return res;
}

bool VulkanSwapChain::Load() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    if (corePtr != nullptr) {
        auto physDevice = corePtr->getPhysicalDevice();
        auto logDevice = corePtr->getDevice();
        auto graphicQueue = corePtr->getQueue(vk::QueueFlagBits::eGraphics);

        auto winPtr = m_VulkanWindow.lock();
        assert(winPtr != nullptr);

        auto size = winPtr->getFrameBufferResolution();
        m_DisplayRect = ct::frect(0, 0, (float)size.x, (float)size.y);

        // Setup viewports, Vsync
        vk::Extent2D swapchainSize = vk::Extent2D(size.x, size.y);

        // All framebuffers / attachments will be the same size as the surface
        vk::SurfaceCapabilitiesKHR surfaceCapabilities = physDevice.getSurfaceCapabilitiesKHR(m_Surface);
        if (!(surfaceCapabilities.currentExtent.width == 0U || surfaceCapabilities.currentExtent.height == 0U)) {
            swapchainSize = surfaceCapabilities.currentExtent;
            m_RenderArea = vk::Rect2D(vk::Offset2D(), swapchainSize);
            m_Viewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchainSize.width), static_cast<float>(swapchainSize.height), 0, 1.0f);
        }

        // VSync
        std::vector<vk::PresentModeKHR> surfacePresentModes = physDevice.getSurfacePresentModesKHR(m_Surface);
        vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;

        if constexpr (SWAPCHAIN_IMAGES_COUNT > 1) {
            for (vk::PresentModeKHR& pm : surfacePresentModes) {
                if (pm == vk::PresentModeKHR::eMailbox) {
                    presentMode = vk::PresentModeKHR::eMailbox;
                    break;
                }
            }
        }

        if (USE_VSYNC) {
            for (vk::PresentModeKHR& pm : surfacePresentModes) {
                if (pm == vk::PresentModeKHR::eFifo) {
                    presentMode = vk::PresentModeKHR::eFifo;
                    break;
                }
            }
        }

        // Create Swapchain, Images, Frame Buffers
        logDevice.waitIdle();
        vk::SwapchainKHR oldSwapchain = m_Swapchain;

        // Determine the number of VkImages to use in the swap chain.
        // Application desires to acquire 3 images at a time for triple
        // buffering
        uint32_t desiredNumOfSwapchainImages = 3;
        if (desiredNumOfSwapchainImages < surfaceCapabilities.minImageCount) {
            desiredNumOfSwapchainImages = surfaceCapabilities.minImageCount;
        }

        // If maxImageCount is 0, we can ask for as many images as we want,
        // otherwise
        // we're limited to maxImageCount
        if ((surfaceCapabilities.maxImageCount > 0) && (desiredNumOfSwapchainImages > surfaceCapabilities.maxImageCount)) {
            desiredNumOfSwapchainImages = SWAPCHAIN_IMAGES_COUNT;
        }

        // Some devices can support more than 2 buffers, but during my tests they would crash on fullscreen ~ ag
        // Tested on an NVIDIA 1050 TI and 60 Hz display (aiekick)
        // Tested on an NVIDIA 3060 and 60 Hz display (aiekick)
        // Tested on an NVIDIA 1080 and 165 Hz 2K display (original author)
        // uint32_t backbufferCount = ct::clamp<uint32_t>(SWAPCHAIN_IMAGES_COUNT, 2u, surfaceCapabilities.maxImageCount);
        if (desiredNumOfSwapchainImages != SWAPCHAIN_IMAGES_COUNT) {
            LogVarError("Cant Create swapchain. exit!");
            CTOOL_DEBUG_BREAK;
            return false;
        }

        CheckSurfaceFormat();

        m_Swapchain = logDevice.createSwapchainKHR(
            vk::SwapchainCreateInfoKHR(vk::SwapchainCreateFlagsKHR(), m_Surface, SWAPCHAIN_IMAGES_COUNT, m_SurfaceColorFormat, m_SurfaceColorSpace,
                swapchainSize, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 1, &graphicQueue.familyQueueIndex,
                vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode, VK_TRUE, oldSwapchain));

        m_OutputSize = vk::Extent2D(glm::clamp(swapchainSize.width, 1U, 8192U), glm::clamp(swapchainSize.height, 1U, 8192U));
        m_RenderArea = vk::Rect2D(vk::Offset2D(), m_OutputSize);
        m_Viewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(m_OutputSize.width), static_cast<float>(m_OutputSize.height), 0, 1.0f);

        // Destroy previous swapchain
        if (oldSwapchain != vk::SwapchainKHR(nullptr)) {
            logDevice.destroySwapchainKHR(oldSwapchain);
        }

        m_FrameIndex = 0;

        return true;  // todo : to add false cases
    }
    return false;
}

void VulkanSwapChain::CheckSurfaceFormat() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    if (corePtr != nullptr) {
        auto physDevice = corePtr->getPhysicalDevice();
        //auto logDevice = corePtr->getDevice();

        std::vector<vk::SurfaceFormatKHR> surfaceFormats = physDevice.getSurfaceFormatsKHR(m_Surface);
        for (const auto& elem : surfaceFormats) {
            if (elem.format == m_SurfaceColorFormat && elem.colorSpace == m_SurfaceColorSpace) {
                return;
            }
        }

        if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined) {
            m_SurfaceColorFormat = vk::Format::eB8G8R8A8Unorm;
        } else {
            m_SurfaceColorFormat = surfaceFormats[0].format;
        }

        m_SurfaceColorSpace = surfaceFormats[0].colorSpace;
    }
}

bool VulkanSwapChain::CreateSurface() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    if (corePtr != nullptr) {
        auto physDevice = corePtr->getPhysicalDevice();
        //auto logDevice = corePtr->getDevice();
        auto queue = corePtr->getQueue(vk::QueueFlagBits::eGraphics);

        // Surface
        auto winPtr = m_VulkanWindow.lock();
        assert(winPtr != nullptr);

        m_Surface = winPtr->createSurface(corePtr->getInstance());

        if (!physDevice.getSurfaceSupportKHR(queue.familyQueueIndex, m_Surface)) {
            // Check if queueFamily supports this surface
            LogVarError("No surface found for this queueFamily");
            return false;
        }

        return true;  // todo : to add false cases
    }
    return false;
}

bool VulkanSwapChain::CreateSyncObjects() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    if (corePtr != nullptr) {
        auto logDevice = corePtr->getDevice();

        // Semaphore used to ensures that image presentation is complete before starting to submit again
        for (size_t i = 0; i < SWAPCHAIN_IMAGES_COUNT; ++i) {
            m_PresentCompleteSemaphores[i] = logDevice.createSemaphore(vk::SemaphoreCreateInfo());
            m_RenderCompleteSemaphores[i] = logDevice.createSemaphore(vk::SemaphoreCreateInfo());
            m_WaitFences[i] = logDevice.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
        }

        return true;  // todo : to add false cases
    }
    return false;
}

bool VulkanSwapChain::CreateFrameBuffers() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    if (corePtr != nullptr) {
        //auto physDevice = corePtr->getPhysicalDevice();
        auto logDevice = corePtr->getDevice();
        //auto queue = corePtr->getQueue(vk::QueueFlagBits::eGraphics);

        /*m_FrameBufferPtr = FrameBuffer::Create(m_VulkanCore);
        if (m_FrameBufferPtr && m_FrameBufferPtr->Init(//
            ct::uvec2(m_OutputSize.width, m_OutputSize.height), //
            SWAPCHAIN_IMAGES_COUNT, //
            true, true, //
            ct::fvec4(0, 0, 0, 1), //
            false, //
            m_SurfaceColorFormat, //
            vk::SampleCountFlagBits::e1)) {


        }*/

#ifdef SWAPCHAIN_USE_DEPTH
        vk::ImageCreateInfo image_ci(vk::ImageCreateFlags(), vk::ImageType::e2D, m_SurfaceDepthFormat,
            vk::Extent3D(m_OutputSize.width, m_OutputSize.height, 1), 1U, 1U,
            vk::SampleCountFlagBits::e1,  // la swpachain ne supporte que le e1
            vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive, 1, &queue.familyQueueIndex, vk::ImageLayout::eUndefined);

        vk::Image depth;
        VmaAllocation depth_alloction = {};
        VmaAllocationCreateInfo depth_alloc_info = {};
        depth_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        corePtr->check_error(vmaCreateImage(
            GaiApi::VulkanCore::sAllocator, (VkImageCreateInfo*)&image_ci, &depth_alloc_info, (VkImage*)&depth, &depth_alloction, nullptr));

        m_Depth.image = depth;
        m_Depth.meta = depth_alloction;

        m_Depth.view =
            logDevice.createImageView(vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(), m_Depth.image, vk::ImageViewType::e2D, m_SurfaceDepthFormat,
                vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1)));
#endif
        auto colorImagesInSwapchain = logDevice.getSwapchainImagesKHR(m_Swapchain);

        assert(colorImagesInSwapchain.size() == m_SwapchainFrameBuffers.size());

        for (size_t i = 0; i < colorImagesInSwapchain.size(); ++i) {
            // Color
            m_SwapchainFrameBuffers[i].views[COLOR] =
                logDevice.createImageView(vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(), colorImagesInSwapchain[i], vk::ImageViewType::e2D,
                    m_SurfaceColorFormat, vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));

#ifdef SWAPCHAIN_USE_DEPTH
            // Depth
            m_SwapchainFrameBuffers[i].views[DEPTH] = m_Depth.view;
#endif
            // frame buffer
            m_SwapchainFrameBuffers[i].frameBuffer = logDevice.createFramebuffer(
                vk::FramebufferCreateInfo(vk::FramebufferCreateFlags(), m_RenderPass, static_cast<uint32_t>(m_SwapchainFrameBuffers[i].views.size()),
                    m_SwapchainFrameBuffers[i].views.data(), m_OutputSize.width, m_OutputSize.height, 1));
        }

        return true;  // todo : to add false cases
    }
    return false;
}

bool VulkanSwapChain::CreateRenderPass() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    if (corePtr != nullptr) {
        //auto physDevice = corePtr->getPhysicalDevice();
        auto logDevice = corePtr->getDevice();
        //auto queue = corePtr->getQueue(vk::QueueFlagBits::eGraphics);

        std::vector<vk::AttachmentDescription> attachmentDescriptions = {
            vk::AttachmentDescription(vk::AttachmentDescriptionFlags(), m_SurfaceColorFormat, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR)
#ifdef SWAPCHAIN_USE_DEPTH
                ,
            vk::AttachmentDescription(vk::AttachmentDescriptionFlags(), m_SurfaceDepthFormat, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal)
#endif
        };

        std::vector<vk::AttachmentReference> colorReferences = {vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal)};

#ifdef SWAPCHAIN_USE_DEPTH
        std::vector<vk::AttachmentReference> depthReferences = {vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)};
#endif
        std::vector<vk::SubpassDescription> subpasses = {vk::SubpassDescription(vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0,
            nullptr, static_cast<uint32_t>(colorReferences.size()), colorReferences.data(), nullptr
#ifdef SWAPCHAIN_USE_DEPTH
            ,
            depthReferences.data()
#else
            ,
            nullptr
#endif
                )};

        std::vector<vk::SubpassDependency> dependencies = {
            vk::SubpassDependency(~0U, 0, vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
                vk::DependencyFlagBits::eByRegion),
            vk::SubpassDependency(0, ~0U, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
                vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead,
                vk::DependencyFlagBits::eByRegion),
#ifdef SWAPCHAIN_USE_DEPTH
            vk::SubpassDependency(~0U, 0, vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eEarlyFragmentTests,
                vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::DependencyFlagBits::eByRegion),
            vk::SubpassDependency(0, ~0U, vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::PipelineStageFlagBits::eLateFragmentTests,
                vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::DependencyFlagBits::eByRegion),
#endif
        };

        m_RenderPass = logDevice.createRenderPass(
            vk::RenderPassCreateInfo(vk::RenderPassCreateFlags(), static_cast<uint32_t>(attachmentDescriptions.size()), attachmentDescriptions.data(),
                static_cast<uint32_t>(subpasses.size()), subpasses.data(), static_cast<uint32_t>(dependencies.size()), dependencies.data()));

        m_ClearValues[0] = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
#ifdef SWAPCHAIN_USE_DEPTH
        m_ClearValues[1] = vk::ClearDepthStencilValue(1.0f, 0u);
#endif

        return true;  // todo : to add false cases
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// RESIZE ////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanSwapChain::Resize() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    if (corePtr != nullptr) {
        corePtr->getDevice().waitIdle();
        g_SwapChainRebuild = false;
        m_ResizeFunction();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// DESTROY ///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanSwapChain::Unit() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);
    auto logDevice = corePtr->getDevice();

    DestroyRenderPass();
    DestroyFrameBuffers();
    DestroySyncObjects();

    logDevice.destroySwapchainKHR(m_Swapchain);
    m_Swapchain = vk::SwapchainKHR{};

    DestroySurface();

    m_ResizeFunction = nullptr;
}

void VulkanSwapChain::DestroyFrameBuffers() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);
    auto logDevice = corePtr->getDevice();

    for (auto& elem : m_SwapchainFrameBuffers) {
        logDevice.destroyFramebuffer(elem.frameBuffer);
        elem.frameBuffer = vk::Framebuffer{};
        logDevice.destroyImageView(elem.views[COLOR]);
        elem.views[COLOR] = vk::ImageView{};
    }

#ifdef SWAPCHAIN_USE_DEPTH
    logDevice.destroyImageView(m_Depth.view);
    vmaDestroyImage(GaiApi::VulkanCore::sAllocator, (VkImage)m_Depth.image, m_Depth.meta);
#endif
}

void VulkanSwapChain::DestroyRenderPass() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);
    auto logDevice = corePtr->getDevice();

    logDevice.destroyRenderPass(m_RenderPass);
    m_RenderPass = vk::RenderPass{};
}

void VulkanSwapChain::DestroySyncObjects() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);
    auto logDevice = corePtr->getDevice();

    for (size_t i = 0; i < SWAPCHAIN_IMAGES_COUNT; ++i) {
        logDevice.destroySemaphore(m_PresentCompleteSemaphores[i]);
        m_PresentCompleteSemaphores[i] = vk::Semaphore{};
        logDevice.destroySemaphore(m_RenderCompleteSemaphores[i]);
        m_RenderCompleteSemaphores[i] = vk::Semaphore{};
        logDevice.destroyFence(m_WaitFences[i]);
        m_WaitFences[i] = vk::Fence{};
    }
}

void VulkanSwapChain::DestroySurface() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);
    auto instance = corePtr->getInstance();

    instance.destroySurfaceKHR(m_Surface);
    m_Surface = vk::SurfaceKHR{};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// GET ///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

vk::SurfaceKHR VulkanSwapChain::getSurface() const {
    ZoneScoped;

    return m_Surface;
}

vk::SwapchainKHR VulkanSwapChain::getSwapchain() const {
    ZoneScoped;

    return m_Swapchain;
}

vk::Viewport VulkanSwapChain::getViewport() const {
    ZoneScoped;

    return m_Viewport;
}

vk::Rect2D VulkanSwapChain::getRenderArea() const {
    ZoneScoped;

    return m_RenderArea;
}

uint32_t VulkanSwapChain::getSwapchainFrameBuffers() const {
    ZoneScoped;

    return static_cast<uint32_t>(m_SwapchainFrameBuffers.size());
}

vk::SampleCountFlagBits VulkanSwapChain::getSwapchainFrameBufferSampleCount() const {
    ZoneScoped;

    return m_SampleCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRESENTATION //////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanSwapChain::AcquireNextImage() {
    ZoneScoped;

    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);
    auto logDevice = corePtr->getDevice();
    vk::Result result = logDevice.acquireNextImageKHR(m_Swapchain, UINT64_MAX, m_PresentCompleteSemaphores[m_FrameIndex], nullptr, &m_FrameIndex);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || g_SwapChainRebuild) {
        // Swapchain lost, we'll try again next poll
        Resize();
        return false;
    }
    if (result == vk::Result::eErrorDeviceLost) {
        // driver lost, we'll crash in this case:
        corePtr->check_error(result);
        exit(1);
    }

    return true;
}

void VulkanSwapChain::Present() {
    ZoneScoped;

    // std::unique_lock<std::mutex> lck(VulkanSubmitter::criticalSectionMutex, std::defer_lock);
    // lck.lock();
    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);
    auto queue = corePtr->getQueue(vk::QueueFlagBits::eGraphics);
    auto result = queue.vkQueue.presentKHR(vk::PresentInfoKHR(1, &m_RenderCompleteSemaphores[m_FrameIndex], 1, &m_Swapchain, &m_FrameIndex, nullptr));
    // lck.unlock();
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        // Swapchain lost, we'll try again next poll
        Resize();
        return;
    }
    m_FrameIndex = (m_FrameIndex + 1) % SWAPCHAIN_IMAGES_COUNT;
}
}  // namespace GaiApi