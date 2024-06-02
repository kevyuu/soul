#pragma once

#include "gpu/type.h"

struct GLFWwindow;

namespace soul::gpu
{
  class System;
}

class ImGuiRenderGraphPass
{
public:
  ImGuiRenderGraphPass(soul::gpu::System* gpu_system);
  ~ImGuiRenderGraphPass();
  void add_pass(soul::gpu::TextureNodeID render_target, soul::gpu::RenderGraph& render_graph);

private:
  soul::gpu::ProgramID program_id_      = soul::gpu::ProgramID::Null();
  soul::gpu::TextureID font_texture_id_ = soul::gpu::TextureID::Null();
  soul::gpu::SamplerID font_sampler_id_ = soul::gpu::SamplerID();
  soul::gpu::System* gpu_system_        = nullptr;
};
