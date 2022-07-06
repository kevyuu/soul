#pragma once

#include "gpu/type.h"

struct GLFWwindow;

namespace soul::gpu
{
	class System;
	class RenderGraph;
}

class ImGuiRenderGraphPass
{
public:
	ImGuiRenderGraphPass(GLFWwindow* window, soul::gpu::System* gpu_system);
	~ImGuiRenderGraphPass();
	void add_pass(soul::gpu::RenderGraph& render_graph);

private:

	soul::gpu::ProgramID program_id_ = soul::gpu::ProgramID();
	soul::gpu::TextureID font_texture_id_ = soul::gpu::TextureID();
	soul::gpu::SamplerID font_sampler_id_ = soul::gpu::SamplerID();
	soul::gpu::System* gpu_system_ = nullptr;
};