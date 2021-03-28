#include "VulkanDevice.h"
#include "VulkanWindow.h"
#include "VulkanLogger.h"

#define TRACE_MEMORY
#include <Profiler/Profiler.h>

namespace vkApi
{
	static inline const char* GetStringFromObjetType(VkDebugReportObjectTypeEXT vObjectType)
	{
		ZoneScoped;

		switch (vObjectType)
		{
		case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT:
			return "Unknow";
		case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:
			return "Instance";
		case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:
			return "PhysDevice";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:
			return "LogicalDevice";
		case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:
			return "Queue";
		case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
			return "Semaphore";
		case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:
			return "CommandBuffer";
		case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:
			return "Fence";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
			return "DeviceMemory";
		case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
			return "Buffer";
		case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
			return "Image";
		case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
			return "Event";
		case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
			return "QueryPool";
		case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
			return "BufferView";
		case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
			return "ImageView";
		case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
			return "ShaderModule";
		case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:
			return "PipelineCache";
		case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:
			return "PipelineLayout";
		case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
			return "RenderPass";
		case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
			return "Pipeline";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:
			return "DescriptorSetLayout";
		case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
			return "Sampler";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
			return "DescriptorPool";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
			return "DescriptorSet";
		case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
			return "Framebuffer";
		case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
			return "CommandPool";
		case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:
			return "SurfaceKHR";
		case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
			return "SwapchainKHR";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT:
			return "DebugReportCallcack";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT:
			return "DisplayKHR";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT:
			return "DsiplayModeKHR";
		case VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT_EXT:
			return "ValidationCache";
		case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT:
			return "YCBCRConversion";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT:
			return "DescriptorUpdateTemplate";
		case VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR_EXT:
			return "AccelerationStructure";
		case VK_DEBUG_REPORT_OBJECT_TYPE_MAX_ENUM_EXT:
			return "Unknow";
		}
		return "";
	}

	//HELPERS
	VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objectType,
		uint64_t object,
		size_t location,
		int32_t messageCode,
		const char* pLayerPrefix,
		const char* pMessage,
		void* pUserData)
	{
		UNUSED(flags); 
		UNUSED(object);
		UNUSED(location);
		UNUSED(messageCode);
		UNUSED(pUserData);
		UNUSED(pLayerPrefix);

		LogVarLight("[VULKAN][%s] => %s", GetStringFromObjetType(objectType), pMessage);
		return VK_FALSE;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//// STATIC //////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	void VulkanDevice::findBestExtensions(const std::vector<vk::ExtensionProperties>& installed, const std::vector<const char*>& wanted, std::vector<const char*>& out)
	{
		ZoneScoped;

		for (const char* const& w : wanted) 
		{
			for (const auto & i : installed) 
			{
				if (std::string((const char*)i.extensionName).compare(w) == 0) 
				{
					out.emplace_back(w);
					break;
				}
			}
		}
	}

	void VulkanDevice::findBestLayers(const std::vector<vk::LayerProperties>& installed, const std::vector<const char*>& wanted, std::vector<const char*>& out)
	{
		ZoneScoped;

		for (const char* const& w : wanted) 
		{
			for (const auto & i : installed) 
			{
				if (std::string((const char*)i.layerName).compare(w) == 0) 
				{
					out.emplace_back(w);
					break;
				}
			}
		}
	}

	uint32_t VulkanDevice::getQueueIndex(vk::PhysicalDevice& physicalDevice, vk::QueueFlags flags, bool standalone)
	{
		ZoneScoped;

		std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();

		if (!standalone)
		{
			for (size_t i = 0; i < queueProps.size(); ++i)
			{
				if (queueProps[i].queueFlags & flags) {
					return static_cast<uint32_t>(i);
				}
			}
		}
		else
		{
			for (size_t i = 0; i < queueProps.size(); ++i)
			{
				if (queueProps[i].queueFlags == flags) {
					return static_cast<uint32_t>(i);
				}
			}
		}

		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//// CONSTRUCTOR /////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	VulkanDevice::VulkanDevice() = default;
	VulkanDevice::~VulkanDevice() = default;

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//// INIT / UNIT /////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	bool VulkanDevice::Init()
	{
		ZoneScoped;

		bool res = false;

		CreateVulkanInstance();
		CreatePhysicalDevice();
		CreateLogicalDevice();

		res = true;

		return res;
	}

	void VulkanDevice::Unit()
	{
		ZoneScoped;

		DestroyLogicalDevice();
		DestroyVulkanInstance();
	}

	VulkanQueue VulkanDevice::getQueue(vk::QueueFlagBits vQueueType)
	{
		ZoneScoped;

		return m_Queues[vQueueType];
	}

	void VulkanDevice::WaitIdle()
	{
		ZoneScoped;

		m_LogDevice.waitIdle();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//// PRIVATE // LAYERS ///////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_GOOGLE_threading",
		"VK_LAYER_LUNARG_parameter_validation",
		"VK_LAYER_LUNARG_device_limits",
		"VK_LAYER_LUNARG_object_tracker",
		"VK_LAYER_LUNARG_image",
		"VK_LAYER_LUNARG_core_validation",
		"VK_LAYER_LUNARG_swapchain",
		"VK_LAYER_GOOGLE_unique_objects",
	};

	void PrintLayerStatus(VkLayerProperties layer_info, string layer_name, bool layer_found)
	{
		UNUSED(layer_found);

		ZoneScoped;

		string major = to_string(VK_VERSION_MAJOR(layer_info.specVersion));
		string minor = to_string(VK_VERSION_MINOR(layer_info.specVersion));
		string patch = to_string(VK_VERSION_PATCH(layer_info.specVersion));
		string version = major + "." + minor + "." + patch;

		LogVarDebug("%s Vulkan vers %s layer vers %u \tDescription:", layer_name.c_str(), version.c_str(), layer_info.implementationVersion);
	}

	// Find available validation layers
	bool CheckValidationLayerSupport()
	{
		ZoneScoped;

		// Query validation layers currently isntalled
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		LogVarDebug("Requesting Vulkan validation layers\t [%u]", layerCount);
		
		// Check needed validation layers against found layers`
		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;
			VkLayerProperties layer_info = {};
			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					layer_info = layerProperties;
					break;
				}
			}

			PrintLayerStatus(layer_info, layerName, layerFound);
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//// PRIVATE /////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	void VulkanDevice::CreateVulkanInstance()
	{
		ZoneScoped;

		//CheckValidationLayerSupport();

		auto wantedExtensions = VulkanWindow::Instance()->vkInstanceExtensions();
		auto wantedLayers = std::vector<const char*>();

#if VULKAN_DEBUG
		wantedLayers.emplace_back("VK_LAYER_KHRONOS_validation");
		//wantedLayers.emplace_back("VK_LAYER_LUNARG_monitor");
		//wantedLayers.emplace_back("VK_LAYER_LUNARG_api_dump");
		//wantedLayers.emplace_back("VK_LAYER_LUNARG_device_simulation");
		//wantedLayers.emplace_back("VK_LAYER_LUNARG_screenshot");
		
		wantedExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		wantedExtensions.emplace_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
#endif

#if ENABLE_CALIBRATED_CONTEXT
		wantedExtensions.emplace_back(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME);
#endif

		// Find the best Instance Extensions
		auto installedExtensions = vk::enumerateInstanceExtensionProperties();
		std::vector<const char*> extensions = {};
		findBestExtensions(installedExtensions, wantedExtensions, extensions);

		// find best instance extensions
		auto installedLayers = vk::enumerateInstanceLayerProperties();
		std::vector<const char*> layers = {};
		findBestLayers(installedLayers, wantedLayers, layers);

		vk::ApplicationInfo appInfo("vkSdfMesher", 0, "vkSDm", 0, VK_API_VERSION_1_0);

		vk::InstanceCreateInfo instanceCreateInfo(
			vk::InstanceCreateFlags(),
			&appInfo,
			static_cast<uint32_t>(layers.size()),
			layers.data(),
			static_cast<uint32_t>(extensions.size()),
			extensions.data()
		);

		std::vector<vk::ValidationFeatureEnableEXT> enabledFeatures;
#if VULKAN_DEBUG_FEATURES
		enabledFeatures.emplace_back(vk::ValidationFeatureEnableEXT::eBestPractices);
		enabledFeatures.emplace_back(vk::ValidationFeatureEnableEXT::eGpuAssisted);
		enabledFeatures.emplace_back(vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot);
#endif
		vk::ValidationFeaturesEXT validationFeatures{ uint32_t(enabledFeatures.size()), enabledFeatures.data() };

		vk::StructureChain<vk::InstanceCreateInfo, vk::ValidationFeaturesEXT> chain = { instanceCreateInfo, validationFeatures };

		m_Instance = vk::createInstance(chain.get<vk::InstanceCreateInfo>());

#if VULKAN_DEBUG
		m_Dldy.init(m_Instance, vkGetInstanceProcAddr);
		VkDebugReportCallbackEXT handle;

		// Setup the debug report callback
		VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
		debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		debug_report_ci.flags = 
			  VK_DEBUG_REPORT_ERROR_BIT_EXT
			| VK_DEBUG_REPORT_WARNING_BIT_EXT 
			| VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT 
			//| VK_DEBUG_REPORT_DEBUG_BIT_EXT  // affiche les extentions
			//| VK_DEBUG_REPORT_INFORMATION_BIT_EXT
		;
		debug_report_ci.pfnCallback = debug_report;
		debug_report_ci.pUserData = NULL;

		auto creat_func = m_Dldy.vkCreateDebugReportCallbackEXT;

		if (creat_func)
		{
			creat_func((VkInstance)m_Instance, &debug_report_ci, nullptr, &handle);
			m_DebugReport = vk::DebugReportCallbackEXT(handle);
		}
		else
		{
			LogVar("%s library is not there. VkDebug is not enabled",
				VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
#endif
	}

	void VulkanDevice::DestroyVulkanInstance()
	{
		ZoneScoped;

#if VULKAN_DEBUG
		if (m_DebugReport) 
			m_Dldy.vkDestroyDebugReportCallbackEXT(m_Instance, m_DebugReport, nullptr);
#endif
		m_Instance.destroy();
	}

	void VulkanDevice::CreatePhysicalDevice()
	{
		ZoneScoped;

		std::vector<vk::PhysicalDevice> physicalDevices =
			m_Instance.enumeratePhysicalDevices();

		auto gpuid = VULKAN_GPU_ID;

		if (gpuid < 0 || gpuid >= physicalDevices.size())
		{
			LogVar("GPU ID error.");
			exit(EXIT_FAILURE);
		}

		m_PhysDevice = physicalDevices[gpuid];
		m_Queues[vk::QueueFlagBits::eGraphics].familyQueueIndex = getQueueIndex(m_PhysDevice, vk::QueueFlagBits::eGraphics, false);
		m_Queues[vk::QueueFlagBits::eCompute].familyQueueIndex = getQueueIndex(m_PhysDevice, vk::QueueFlagBits::eCompute, false);
		m_Queues[vk::QueueFlagBits::eTransfer].familyQueueIndex = getQueueIndex(m_PhysDevice, vk::QueueFlagBits::eTransfer, false);

		//auto limits = m_PhysDevice.getProperties().limits;
		//limits.maxDescriptorSetUniformBuffers;
	}

	void VulkanDevice::DestroyPhysicalDevice()
	{
		ZoneScoped;

		// nothing here
	}

	void VulkanDevice::CreateLogicalDevice()
	{
		ZoneScoped;

		std::map<uint32_t, uint32_t> counts;
		counts[m_Queues[vk::QueueFlagBits::eGraphics].familyQueueIndex]++;
		counts[m_Queues[vk::QueueFlagBits::eCompute].familyQueueIndex]++;
		counts[m_Queues[vk::QueueFlagBits::eTransfer].familyQueueIndex]++;

		float mQueuePriority = 0.5f;
		std::vector<vk::DeviceQueueCreateInfo> qcinfo;
		for (const auto& elem : counts)
		{
			if (elem.second > 0)
			{
				qcinfo.push_back({});
				qcinfo.back().setQueueFamilyIndex(elem.first);
				qcinfo.back().setQueueCount(elem.second);
				qcinfo.back().setPQueuePriorities(&mQueuePriority);
			}
		}

		// Logical VulkanCore
		std::vector<vk::ExtensionProperties> installedDeviceExtensions = m_PhysDevice.enumerateDeviceExtensionProperties();
		std::vector<const char*> wantedDeviceExtensions = {	VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		std::vector<const char*> deviceExtensions = {};
		findBestExtensions(installedDeviceExtensions, wantedDeviceExtensions, deviceExtensions);

		// enabled features
		m_PhysDeviceFeatures.wideLines = true;			// pour changer la taille des lignes
		m_PhysDeviceFeatures.sampleRateShading = true;	// pour anti aliaser les textures
		m_PhysDeviceFeatures.geometryShader = true;		// pour utiliser les shader de geometrie
		m_PhysDeviceFeatures.tessellationShader = true;	// pour utiliser les shaders de tesselation

		vk::DeviceCreateInfo dinfo;
		dinfo.setPEnabledFeatures(&m_PhysDeviceFeatures);
		dinfo.setPQueueCreateInfos(qcinfo.data());
		dinfo.setQueueCreateInfoCount(static_cast<uint32_t>(qcinfo.size()));
		dinfo.setPpEnabledExtensionNames(deviceExtensions.data());
		dinfo.setEnabledExtensionCount(static_cast<uint32_t>(deviceExtensions.size()));
		m_LogDevice = m_PhysDevice.createDevice(dinfo);

		uint32_t familyQueueIndex = m_Queues[vk::QueueFlagBits::eGraphics].familyQueueIndex;
		m_Queues[vk::QueueFlagBits::eGraphics].vkQueue = m_LogDevice.getQueue(familyQueueIndex, 0);
		m_Queues[vk::QueueFlagBits::eGraphics].cmdPools = m_LogDevice.createCommandPool(
			vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer), familyQueueIndex));
	
		familyQueueIndex = m_Queues[vk::QueueFlagBits::eCompute].familyQueueIndex;
		m_Queues[vk::QueueFlagBits::eCompute].vkQueue = m_LogDevice.getQueue(familyQueueIndex, 0);
		m_Queues[vk::QueueFlagBits::eCompute].cmdPools = m_LogDevice.createCommandPool(
			vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer), familyQueueIndex));
		
		familyQueueIndex = m_Queues[vk::QueueFlagBits::eTransfer].familyQueueIndex;
		m_Queues[vk::QueueFlagBits::eTransfer].vkQueue = m_LogDevice.getQueue(familyQueueIndex, 0);
		m_Queues[vk::QueueFlagBits::eTransfer].cmdPools = m_LogDevice.createCommandPool(
			vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer), familyQueueIndex));
	}

	void VulkanDevice::DestroyLogicalDevice()
	{
		ZoneScoped;

		for (auto& elem : m_Queues) { m_LogDevice.destroyCommandPool(elem.second.cmdPools);	}
		m_LogDevice.destroy();
	}
}