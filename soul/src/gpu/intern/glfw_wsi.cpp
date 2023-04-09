#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "gpu/glfw_wsi.h"

namespace soul::gpu
{

  GLFWWsi::GLFWWsi(GLFWwindow* window) : window_(window) {}

  auto GLFWWsi::create_vulkan_surface(VkInstance instance) -> VkSurfaceKHR
  {
    VkSurfaceKHR surface;
    SOUL_LOG_INFO("Creating vulkan surface");
    glfwCreateWindowSurface(instance, window_, nullptr, &surface);
    SOUL_LOG_INFO("Vulkan surface creation sucessful.");
    return surface;
  }

  auto GLFWWsi::get_framebuffer_size() const -> vec2ui32
  {
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    return {width, height};
  }

} // namespace soul::gpu
