//
// Created by Kevin Yudi Utama on 4/1/20.
//
#include "gpu/render_graph_registry.h"
#include "gpu/system.h"
#include "gpu/intern/render_graph_execution.h"

namespace soul::gpu
{

	using namespace impl;

	BufferID RenderGraphRegistry::get_buffer(BufferNodeID buffer_node_id) const {
		return execution_.get_buffer_id(buffer_node_id);
	}

	TextureID RenderGraphRegistry::get_texture(TextureNodeID texture_node_id) const {
		return execution_.get_texture_id(texture_node_id);
	}

	PipelineStateID RenderGraphRegistry::get_pipeline_state(const RasterPipelineStateDesc& desc) {
		const auto& raster_target = execution_.get_raster_target(pass_node_id_);
		auto rg_desc = desc;
		
		rg_desc.sample_count = raster_target.sample_count;
	    return system_.request_pipeline_state(rg_desc);
	}

	PipelineStateID RenderGraphRegistry::get_pipeline_state(const ComputePipelineStateDesc& desc)
	{
		return system_.request_pipeline_state(desc);
	}

}
