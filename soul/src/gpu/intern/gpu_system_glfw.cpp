#include "gpu/system.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace soul {
	namespace gpu {

		void System::create_surface(void* window_handle, VkSurfaceKHR* surface) {
			SOUL_LOG_INFO("Creating vulkan surface");
			SOUL_VK_CHECK(glfwCreateWindowSurface(_db.instance, (GLFWwindow*) window_handle, nullptr, surface), "Vulkan surface creation fail!");
			SOUL_LOG_INFO("Vulkan surface creation sucessful.");
		}
		
	}
}