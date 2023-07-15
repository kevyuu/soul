#pragma once

#include <variant>

// TODO: Figure out how to do it without single header library
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VK_NO_PROTOTYPES
#pragma warning(push, 0)
#include <vk_mem_alloc.h>
#pragma warning(pop)

#include "core/type.h"

#include "object_cache.h"
#include "object_pool.h"

namespace soul::gpu::impl
{
  struct Texture;
  struct Buffer;
  struct Blas;
  struct BlasGroup;
  struct Tlas;

  struct Sampler {
  };

  struct Program;
  struct BinarySemaphore;
  struct Database;
  struct GraphicPipelineStateKey;
  struct ComputePipelineStateKey;
  using PipelineStateKey = std::variant<GraphicPipelineStateKey, ComputePipelineStateKey>;
  struct PipelineState;
  struct ShaderArgSet;
  struct Shader;
  struct Program;
  struct ShaderTable;
  struct BinarySemaphore;
  struct DescriptorSetLayoutKey;
} // namespace soul::gpu::impl

namespace soul::gpu
{

  struct GPUAddressStub {
  };

  using GPUAddress = ID<GPUAddressStub, VkDeviceAddress, 0>;
  static_assert(sizeof(GPUAddress) == sizeof(u64), "GPUAddress size is not the same as u64");

  using TexturePool = ConcurrentObjectPool<impl::Texture>;
  using BufferPool = ConcurrentObjectPool<impl::Buffer>;
  using BlasPool = ConcurrentObjectPool<impl::Blas>;
  using BlasGroupPool = ConcurrentObjectPool<impl::BlasGroup>;
  using TlasPool = ConcurrentObjectPool<impl::Tlas>;
  using ShaderPool = ConcurrentObjectPool<impl::Shader>;
  using ShaderTablePool = ConcurrentObjectPool<impl::ShaderTable>;

  using PipelineStateCache = ConcurrentObjectCache<impl::PipelineStateKey, impl::PipelineState>;

  using DescriptorSetLayoutCache =
    ConcurrentObjectCache<impl::DescriptorSetLayoutKey, VkDescriptorSetLayout>;

  // ID
  using TextureID = ID<impl::Texture, TexturePool::ID, TexturePool::NULLVAL>;
  using BufferID = ID<impl::Buffer, BufferPool::ID, BufferPool::NULLVAL>;
  using BlasID = ID<impl::Blas, BlasPool::ID, BlasPool::NULLVAL>;
  using BlasGroupID = ID<impl::BlasGroup, BlasGroupPool::ID, BlasGroupPool::NULLVAL>;
  using TlasID = ID<impl::Tlas, TlasPool::ID, TlasPool::NULLVAL>;

  struct Descriptor;
  using DescriptorID = ID<Descriptor, u32>;

  struct SamplerID {
    VkSampler vkHandle = VK_NULL_HANDLE;
    DescriptorID descriptorID;

    [[nodiscard]] auto is_null() const -> b8 { return vkHandle == VK_NULL_HANDLE; }

    [[nodiscard]] auto is_valid() const -> b8 { return !this->is_null(); }
  };

  using PipelineStateID =
    ID<impl::PipelineState, PipelineStateCache::ID, PipelineStateCache::NULLVAL>;
  using ProgramID = ID<impl::Program, u16>;
  using ShaderTableID = ID<impl::ShaderTable, ShaderTablePool::ID, ShaderTablePool::NULLVAL>;

} // namespace soul::gpu
