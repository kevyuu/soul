#pragma once

#include "core/type.h"

namespace soul::gpu
{

  constexpr u32 MAX_SET_PER_SHADER_PROGRAM   = 6;
  constexpr u32 SET_COUNT                    = 8;
  constexpr u32 MAX_BINDING_PER_SET          = 16;
  constexpr u32 MAX_INPUT_PER_SHADER         = 16;
  constexpr u32 MAX_INPUT_BINDING_PER_SHADER = 16;

  constexpr u32 MAX_DEPTH_ATTACHMENT_PER_SHADER   = 1;
  constexpr u32 MAX_COLOR_ATTACHMENT_PER_SHADER   = 8;
  constexpr u32 MAX_INPUT_ATTACHMENT_PER_SHADER   = 8;
  constexpr u32 MAX_RESOLVE_ATTACHMENT_PER_SHADER = MAX_COLOR_ATTACHMENT_PER_SHADER;
  constexpr u32 MAX_ATTACHMENT_PER_SHADER =
    MAX_COLOR_ATTACHMENT_PER_SHADER + MAX_RESOLVE_ATTACHMENT_PER_SHADER +
    MAX_INPUT_ATTACHMENT_PER_SHADER + MAX_DEPTH_ATTACHMENT_PER_SHADER;

  constexpr u32 MAX_SIGNAL_SEMAPHORE = 4;
  constexpr u32 MAX_VERTEX_BINDING   = 16;

  constexpr u32 PUSH_CONSTANT_SIZE                  = 256;
  constexpr u32 BINDLESS_SET_COUNT                  = 5;
  constexpr u32 STORAGE_BUFFER_DESCRIPTOR_SET_INDEX = 0;
  constexpr u32 SAMPLER_DESCRIPTOR_SET_INDEX        = 1;
  constexpr u32 SAMPLED_IMAGE_DESCRIPTOR_SET_INDEX  = 2;
  constexpr u32 STORAGE_IMAGE_DESCRIPTOR_SET_INDEX  = 3;
  constexpr u32 AS_DESCRIPTOR_SET_INDEX             = 4;

} // namespace soul::gpu
