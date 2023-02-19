//
// Created by Kevin Yudi Utama on 4/1/20.
//

#pragma once

#include "gpu/type.h"
#include "gpu/render_graph.h"

namespace soul::gpu
{
	namespace impl
	{
		class RenderGraphExecution;
	}

	class RenderGraphRegistry {
	public:

		RenderGraphRegistry() = delete;
		RenderGraphRegistry(System& system, const impl::RenderGraphExecution& execution, PassNodeID pass_node_id) :
			system_(system), execution_(execution), pass_node_id_(pass_node_id) {}

		RenderGraphRegistry(const RenderGraphRegistry& other) = delete;
		RenderGraphRegistry& operator=(const RenderGraphRegistry& other) = delete;

		RenderGraphRegistry(RenderGraphRegistry&& other) = delete;
		RenderGraphRegistry operator=(RenderGraphRegistry&& other) = delete;

		~RenderGraphRegistry() = default;

		[[nodiscard]] BufferID get_buffer(BufferNodeID buffer_node_id) const;
		[[nodiscard]] TextureID get_texture(TextureNodeID texture_node_id) const;
		PipelineStateID get_pipeline_state(const RasterPipelineStateDesc& desc);
		PipelineStateID get_pipeline_state(const ComputePipelineStateDesc& desc);

	private:
		System& system_;
		const impl::RenderGraphExecution& execution_;
		const PassNodeID pass_node_id_;
	};
}
