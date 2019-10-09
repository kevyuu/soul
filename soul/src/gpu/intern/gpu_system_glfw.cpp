#include "gpu/system.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Soul {
	namespace GPU {

		void System::_surfaceCreate(void* windowHandle, VkSurfaceKHR* surface) {
			SOUL_LOG_INFO("Creating vulkan surface");
			SOUL_VK_CHECK(glfwCreateWindowSurface(_db.instance, (GLFWwindow*) windowHandle, nullptr, surface), "Vulkan surface creation fail!");
			SOUL_LOG_INFO("Vulkan surface creation sucessful.");
		}
		
	}
}