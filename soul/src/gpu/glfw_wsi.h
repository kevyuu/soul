#pragma once

#include "core/type.h"
#include "gpu/type.h"

struct GLFWwindow;

namespace soul::gpu
{

  class GLFWWsi : public WSI
  {
  public:
    explicit GLFWWsi(GLFWwindow* window);
    [[nodiscard]] auto create_vulkan_surface(VkInstance instance) -> VkSurfaceKHR override;
    [[nodiscard]] auto get_framebuffer_size() const -> vec2ui32 override;
    ~GLFWWsi() override = default;

  private:
    GLFWwindow* window_;
  };

} // namespace soul::gpu
