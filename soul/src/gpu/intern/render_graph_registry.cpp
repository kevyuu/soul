#include "gpu/render_graph_registry.h"
#include "gpu/intern/render_graph_execution.h"
#include "gpu/system.h"

namespace soul::gpu
{

  using namespace impl;

  auto RenderGraphRegistry::get_buffer(BufferNodeID buffer_node_id) const -> BufferID
  {
    return execution_->get_buffer_id(buffer_node_id);
  }

  auto RenderGraphRegistry::get_texture(TextureNodeID texture_node_id) const -> TextureID
  {
    return execution_->get_texture_id(texture_node_id);
  }

  auto RenderGraphRegistry::get_tlas(TlasNodeID tlas_node_id) const -> TlasID
  {
    return execution_->get_tlas_id(tlas_node_id);
  }

  auto RenderGraphRegistry::get_pipeline_state(const GraphicPipelineStateDesc& desc)
    -> PipelineStateID
  {
    return system_->request_pipeline_state(desc, render_pass_, sample_count_);
  }

  auto RenderGraphRegistry::get_pipeline_state(const ComputePipelineStateDesc& desc)
    -> PipelineStateID
  {
    return system_->request_pipeline_state(desc);
  }

} // namespace soul::gpu
