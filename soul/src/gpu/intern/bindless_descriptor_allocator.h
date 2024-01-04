#pragma once

#include "core/mutex.h"
#include "core/type.h"
#include "gpu/constant.h"
#include "gpu/id.h"
#include "memory/allocator.h"
#include "runtime/runtime.h"

namespace soul::gpu
{
  class System;
  struct Descriptor;
} // namespace soul::gpu

namespace soul::gpu::impl
{

  struct BindlessDescriptorSets
  {
    VkDescriptorSet vk_handles[BINDLESS_SET_COUNT];
  };

  class BindlessDescriptorSet
  {
  public:
    explicit BindlessDescriptorSet(
      u32 capacity,
      VkDescriptorType descriptor_type,
      memory::Allocator* allocator = runtime::get_context_allocator());
    BindlessDescriptorSet(const BindlessDescriptorSet&)                    = delete;
    BindlessDescriptorSet(BindlessDescriptorSet&&)                         = delete;
    auto operator=(const BindlessDescriptorSet&) -> BindlessDescriptorSet& = delete;
    auto operator=(BindlessDescriptorSet&&) -> BindlessDescriptorSet&      = delete;
    auto init(VkDevice device, VkDescriptorPool descriptor_pool) -> void;
    auto create_descriptor(VkDevice device, const VkDescriptorBufferInfo& buffer_info)
      -> DescriptorID;
    auto create_descriptor(VkDevice device, const VkDescriptorImageInfo& image_info)
      -> DescriptorID;
    auto create_descriptor(VkDevice device, VkAccelerationStructureKHR as) -> DescriptorID;
    auto destroy_descriptor(VkDevice device, DescriptorID id) -> void;

    auto get_descriptor_set() const -> VkDescriptorSet
    {
      return descriptor_set_;
    }

    auto get_descriptor_set_layout() const -> VkDescriptorSetLayout
    {
      return descriptor_set_layout_;
    }

    ~BindlessDescriptorSet();

  private:
    memory::Allocator* allocator_;
    u32 free_head_;
    u32* list_;
    u32 capacity_;
    VkDescriptorType descriptor_type_;
    VkDescriptorSet descriptor_set_              = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    RWSpinMutex mutex_;
  };

  class BindlessDescriptorAllocator
  {
  public:
    explicit BindlessDescriptorAllocator(
      memory::Allocator* allocator = runtime::get_context_allocator());

    BindlessDescriptorAllocator(const BindlessDescriptorAllocator&) = delete;

    BindlessDescriptorAllocator(BindlessDescriptorAllocator&&) noexcept = delete;

    auto operator=(const BindlessDescriptorAllocator&) noexcept
      -> BindlessDescriptorAllocator = delete;

    auto operator=(BindlessDescriptorAllocator&&) noexcept -> BindlessDescriptorAllocator = delete;

    ~BindlessDescriptorAllocator();

    auto init(VkDevice device) -> void;

    auto create_storage_buffer_descriptor(VkBuffer buffer) -> DescriptorID;

    auto destroy_storage_buffer_descriptor(DescriptorID id) -> void;

    auto create_sampled_image_descriptor(VkImageView image_view) -> DescriptorID;

    auto destroy_sampled_image_descriptor(DescriptorID id) -> void;

    auto create_storage_image_descriptor(VkImageView image_view) -> DescriptorID;

    auto destroy_storage_image_descriptor(DescriptorID id) -> void;

    auto create_sampler_descriptor(VkSampler sampler) -> DescriptorID;

    auto destroy_sampler_descriptor(DescriptorID id) -> void;

    auto create_as_descriptor(VkAccelerationStructureKHR as) -> DescriptorID;

    auto destroy_as_descriptor(DescriptorID id) -> void;

    auto get_pipeline_layout() const -> VkPipelineLayout
    {
      return pipeline_layout_;
    }

    auto get_bindless_descriptor_sets() const -> BindlessDescriptorSets
    {
      return {
        {storage_buffer_descriptor_set_.get_descriptor_set(),
         sampler_descriptor_set_.get_descriptor_set(),
         sampled_image_descriptor_set_.get_descriptor_set(),
         storage_image_descriptor_set_.get_descriptor_set(),
         as_descriptor_set_.get_descriptor_set()}};
    }

  private:
    static constexpr u32 STORAGE_BUFFER_DESCRIPTOR_COUNT = 512u * 1024;
    static constexpr u32 SAMPLER_DESCRIPTOR_COUNT        = 4u * 1024;
    static constexpr u32 SAMPLED_IMAGE_DESCRIPTOR_COUNT  = 512u * 1024;
    static constexpr u32 STORAGE_IMAGE_DESCRIPTOR_COUNT  = 512u * 1024;
    static constexpr u32 AS_DESCRIPTOR_COUNT             = 512u;

    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    BindlessDescriptorSet storage_buffer_descriptor_set_;
    BindlessDescriptorSet sampler_descriptor_set_;
    BindlessDescriptorSet sampled_image_descriptor_set_;
    BindlessDescriptorSet storage_image_descriptor_set_;
    BindlessDescriptorSet as_descriptor_set_;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkDevice device_                  = VK_NULL_HANDLE;
  };

} // namespace soul::gpu::impl
