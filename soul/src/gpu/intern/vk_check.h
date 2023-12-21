#pragma once

#if defined(SOUL_ASSERT_ENABLE)
#  include "core/log.h"
#  include "core/panic.h"
#  define SOUL_VK_CHECK(expr, message)                                                             \
    do {                                                                                           \
      VkResult _result = expr;                                                                     \
      if (_result != VK_SUCCESS) {                                                                 \
        SOUL_LOG_ERROR("Vulkan error| expr = {}, result = {} ", #expr, usize(_result));            \
        SOUL_ASSERT(0, expr != VK_SUCCESS, message);                                               \
      }                                                                                            \
    } while (false)
#else
#  include "core/log.h"
#  define SOUL_VK_CHECK(expr, message)                                                             \
    do {                                                                                           \
      VkResult _result = expr;                                                                     \
      if (_result != VK_SUCCESS) {                                                                 \
        SOUL_LOG_ERROR("Vulkan error| expr = {}, result = {} ", #expr, usize(_result));            \
        SOUL_LOG_ERROR("Message = {}", message);                                                   \
      }                                                                                            \
    } while (false)
#endif
