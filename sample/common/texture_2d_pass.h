#pragma once

#include "gpu/type.h"
#include "gpu/render_graph.h"

class Texture2DRGPass
{
public:
	struct Parameter
	{
		soul::gpu::TextureNodeID sampled_texture;
		soul::gpu::TextureNodeID render_target;
	};

	Texture2DRGPass(soul::gpu::System* gpu_system);
	~Texture2DRGPass();
	soul::gpu::TextureNodeID add_pass(const Parameter& parameter, soul::gpu::RenderGraph& render_graph);

private:

	soul::gpu::ProgramID program_id_ = soul::gpu::ProgramID();
    soul::gpu::BufferID vertex_buffer_id_ = soul::gpu::BufferID();
	soul::gpu::BufferID index_buffer_id_ = soul::gpu::BufferID();
	soul::gpu::SamplerID sampler_id_ = soul::gpu::SamplerID();
	soul::gpu::System* gpu_system_ = nullptr;
};