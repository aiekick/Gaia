#pragma once

#include <vulkan/vulkan.hpp>
#include <ctools/cTools.h>

struct GLFWwindow;
namespace vkApi
{
	class VulkanWindow
	{
	public:
		void Init(int width, int height, const std::string& name, bool fullscreen = false);
		void Unit();

		ct::ivec2 pixelrez() const;
		ct::ivec2 clentrez() const;

		bool IsMinimized();

		const std::string& name() const;
		vk::SurfaceKHR createSurface(vk::Instance vkInstance);
		const std::vector<const char*>& vkInstanceExtensions() const;

		GLFWwindow* WinPtr() const;

	private:
		std::string d_name;
		GLFWwindow* m_Window = nullptr;
		std::vector<const char*> d_vkInstanceExtension;

	public: // singleton
		static VulkanWindow *Instance()
		{
			static VulkanWindow _instance;
			return &_instance;
		}

	protected:
		VulkanWindow() {} // Prevent construction
		VulkanWindow(const VulkanWindow&) {}; // Prevent construction by copying
		VulkanWindow& operator =(const VulkanWindow&) { return *this; }; // Prevent assignment
		~VulkanWindow() {} // Prevent unwanted destruction
	};
}
