#include "gpu/glfw_wsi.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace soul::gpu
{
    GLFWWsi::GLFWWsi(GLFWwindow* window) : window_(window) {}

    VkSurfaceKHR GLFWWsi::create_vulkan_surface(VkInstance instance)
    {
        VkSurfaceKHR surface;
        SOUL_LOG_INFO("Creating vulkan surface");
        glfwCreateWindowSurface(instance, (GLFWwindow*)window_, nullptr, &surface);
        SOUL_LOG_INFO("Vulkan surface creation sucessful.");
        return surface;
    }

    vec2ui32 GLFWWsi::get_framebuffer_size() const
    {
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        return { width, height };
    }

}
