#pragma once

#include <vulkan/vulkan_core.h>

#include "core/builtins.h"

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
    virtual auto get_framebuffer_size() const -> vec2u32 = 0;

    virtual ~WSI() = default;
  };
} // namespace soul::gpu
