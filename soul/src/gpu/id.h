#pragma once

#include "core/type.h"
// TODO: Figure out how to do it without single header library
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VK_NO_PROTOTYPES
#pragma warning(push, 0)
#include <vk_mem_alloc.h>
#pragma warning(pop)

#include "object_pool.h"

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
}

namespace soul::gpu
{
	// ID
	using TextureID = ID<impl::Texture, ObjectPool<impl::Texture>::ID, ObjectPool<impl::Texture>::NULLVAL>;
	using BufferID = ID<impl::Buffer, ObjectPool<impl::Buffer>::ID, ObjectPool<impl::Buffer>::NULLVAL>;

	using SamplerID = ID<impl::Sampler, VkSampler, VK_NULL_HANDLE>;
	constexpr SamplerID SAMPLER_ID_NULL = SamplerID();

	using PipelineStateID = ID<impl::PipelineState, uint32, 0>;
	constexpr PipelineStateID PIPELINE_STATE_ID_NULL = PipelineStateID();

	using ShaderArgSetID = ID<impl::ShaderArgSet, uint32, 0>;

	using ShaderID = ID<impl::Shader, ObjectPool<impl::Shader>::ID, ObjectPool<impl::Shader>::NULLVAL>;
	constexpr ShaderID SHADER_ID_NULL = ShaderID();

	using ProgramID = ID<impl::Program, uint16, 0>;
	constexpr ProgramID PROGRAM_ID_NULL = ProgramID();

	using SemaphoreID = ID<impl::Semaphore, uint32, 0>;
	constexpr SemaphoreID SEMAPHORE_ID_NULL = SemaphoreID();
}
