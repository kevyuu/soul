#pragma once

#include "core/type.h"

#include "gpu/type.h"
#include "gpu/wsi.h"

struct GLFWwindow;

namespace soul::gpu
{

  class GLFWWsi : public WSI
  {
  public:
    GLFWWsi(const GLFWWsi&) = delete;

    GLFWWsi(GLFWWsi&&) = delete;

    auto operator=(const GLFWWsi&) -> GLFWWsi& = delete;

    auto operator=(GLFWWsi&&) -> GLFWWsi& = delete;

    ~GLFWWsi() override = default;

    explicit GLFWWsi(GLFWwindow* window);

    [[nodiscard]]
    auto create_vulkan_surface(VkInstance instance) -> VkSurfaceKHR override;

    [[nodiscard]]
    auto get_framebuffer_size() const -> vec2u32 override;

  private:
    GLFWwindow* window_;
  };

} // namespace soul::gpu
