#pragma once

#include "core/builtins.h"

#include <vulkan/vulkan_core.h>

namespace soul::gpu
{
  class WSI
  {
  public:
    [[nodiscard]]
    virtual auto create_vulkan_surface(VkInstance instance) -> VkSurfaceKHR = 0;

    WSI() = default;

    WSI(const WSI& other) = delete;

    WSI(WSI&& other) = delete;

    auto operator=(const WSI& other) -> WSI& = delete;

    auto operator=(WSI&& other) -> WSI& = delete;

    [[nodiscard]]
    virtual auto get_framebuffer_size() const -> vec2ui32 = 0;

    virtual ~WSI() = default;
  };
} // namespace soul::gpu