#pragma once

#include "gpu/impl/vulkan/vk_str.h"

#if defined(SOUL_ASSERT_ENABLE)
#  include "core/log.h"
#  include "core/panic.h"
#  define SOUL_VK_CHECK(expr, ...)                                                                 \
    do                                                                                             \
    {                                                                                              \
      VkResult _result = expr;                                                                     \
      if (_result != VK_SUCCESS)                                                                   \
      {                                                                                            \
        SOUL_LOG_ERROR("Vulkan error| expr = {}, result = {} ", #expr, to_string(_result));        \
        SOUL_ASSERT(0, expr != VK_SUCCESS, ##__VA_ARGS__);                                         \
      }                                                                                            \
    }                                                                                              \
    while (false)
#else
#  include "core/log.h"
#  define SOUL_VK_CHECK(expr, ...)                                                                 \
    do                                                                                             \
    {                                                                                              \
      VkResult _result = expr;                                                                     \
      if (_result != VK_SUCCESS)                                                                   \
      {                                                                                            \
        SOUL_LOG_ERROR("Vulkan error| expr = {}, result = {} ", #expr, usize(_result));            \
        SOUL_LOG_ERROR("Message = {}", ##__VA_ARGS__);                                             \
      }                                                                                            \
    }                                                                                              \
    while (false)
#endif
