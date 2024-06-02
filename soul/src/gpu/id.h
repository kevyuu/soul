#pragma once

#pragma warning(push, 0)
#define VK_NO_PROTOTYPES
#include <vk_mem_alloc.h>
#pragma warning(pop)

#include "core/rid.h"
#include "core/type.h"
#include "core/type_traits.h"

namespace soul::gpu
{
  using GPUAddress   = ID<struct gpu_address_tag, u64, 0>;
  using DescriptorID = ID<struct descriptor_id_tag, u32>;

  // ID
  using TextureID   = RID<struct texture_id_tag>;
  using BufferID    = RID<struct buffer_id_tag>;
  using BlasID      = RID<struct blas_id_tag>;
  using BlasGroupID = RID<struct blas_group_id_tag>;
  using TlasID      = RID<struct tlas_id_tag>;

  using PipelineStateID = RID<struct pipeline_state_id_tag>;
  using ProgramID       = RID<struct program_id_tag>;
  using ShaderID        = RID<struct shader_id_tag>;
  using ShaderTableID   = RID<struct shader_id_tag>;

  struct SamplerID
  {
    VkSampler vkHandle = VK_NULL_HANDLE;
    DescriptorID descriptorID;

    [[nodiscard]]
    auto is_null() const -> b8
    {
      return vkHandle == VK_NULL_HANDLE;
    }

    [[nodiscard]]
    auto is_valid() const -> b8
    {
      return !this->is_null();
    }
  };

} // namespace soul::gpu
