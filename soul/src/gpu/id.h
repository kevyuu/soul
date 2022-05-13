#pragma once

#include "core/type.h"
// TODO: Figure out how to do it without single header library
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VK_NO_PROTOTYPES
#pragma warning(push, 0)
#include <vk_mem_alloc.h>
#pragma warning(pop)

#include "intern/shader_arg_set_allocator.h"
#include "object_pool.h"
#include "object_cache.h"
#include <variant>


namespace soul::gpu::impl
{
	struct Texture;
	struct Buffer;
	struct Sampler {};
	struct Program;
	struct Semaphore;
	struct Database;
	struct PipelineState;
	struct ShaderArgSet;
	struct Shader;
	struct Program;
	struct Semaphore;
	struct DescriptorSetLayoutKey;
}

namespace soul::gpu
{

	using TexturePool = ConcurrentObjectPool<impl::Texture>;
	using BufferPool = ConcurrentObjectPool<impl::Buffer>;

	struct GraphicPipelineStateDesc;
	struct ComputePipelineStateDesc;
	using PipelineStateDesc = std::variant<GraphicPipelineStateDesc, ComputePipelineStateDesc>;
	using PipelineStateCache = ConcurrentObjectCache<PipelineStateDesc, impl::PipelineState>;

	using DescriptorSetLayoutCache = ConcurrentObjectCache<impl::DescriptorSetLayoutKey, VkDescriptorSetLayout>;
	
	// ID
	using TextureID = ID<impl::Texture, TexturePool::ID, TexturePool::NULLVAL>;
	using BufferID = ID<impl::Buffer, BufferPool::ID, BufferPool::NULLVAL>;

	using SamplerID = ID<impl::Sampler, VkSampler, VK_NULL_HANDLE>;
	constexpr SamplerID SAMPLER_ID_NULL = SamplerID();

	using ShaderArgSetID = impl::ShaderArgSetAllocator::ID;

	using PipelineStateID = ID<impl::PipelineState, PipelineStateCache::ID, PipelineStateCache::NULLVAL>;
	constexpr PipelineStateID PIPELINE_STATE_ID_NULL = PipelineStateID();

	using ShaderID = ID<impl::Shader, ConcurrentObjectPool<impl::Shader>::ID, ConcurrentObjectPool<impl::Shader>::NULLVAL>;
	constexpr ShaderID SHADER_ID_NULL = ShaderID();

	using ProgramID = ID<impl::Program, uint16, 0>;
	constexpr ProgramID PROGRAM_ID_NULL = ProgramID();

	using SemaphoreID = ID<impl::Semaphore, uint32, 0>;
	constexpr SemaphoreID SEMAPHORE_ID_NULL = SemaphoreID();
}
