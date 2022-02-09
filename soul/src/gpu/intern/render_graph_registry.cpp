//
// Created by Kevin Yudi Utama on 4/1/20.
//
#include "gpu/render_graph_registry.h"
#include "gpu/system.h"
#include "gpu/intern/render_graph_execution.h"

namespace soul::gpu
{

	using namespace impl;

	BufferID RenderGraphRegistry::get_buffer(BufferNodeID bufferNodeID) const {
		return execution_->get_buffer_id(bufferNodeID);
	}

	TextureID RenderGraphRegistry::get_texture(TextureNodeID textureNodeId) const {
		return execution_->get_texture_id(textureNodeId);
	}

	ShaderArgSetID RenderGraphRegistry::get_shader_arg_set(uint32 set, const ShaderArgSetDesc &desc) {
		return system_->request_shader_arg_set(desc);
	}

	PipelineStateID RenderGraphRegistry::get_pipeline_state(const GraphicPipelineStateDesc& desc) {
		return system_->request_pipeline_state(desc, render_pass_, sample_count_);
	}

	PipelineStateID RenderGraphRegistry::get_pipeline_state(const ComputePipelineStateDesc& desc)
	{
		return system_->request_pipeline_state(desc);
	}

}
