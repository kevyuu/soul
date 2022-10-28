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
		RenderGraphRegistry(System& system, const impl::RenderGraphExecution& execution, const VkRenderPass render_pass = VK_NULL_HANDLE, const TextureSampleCount sample_count = TextureSampleCount::COUNT_1) :
			system_(system), execution_(execution), render_pass_(render_pass), sample_count_(sample_count) {}

		RenderGraphRegistry(const RenderGraphRegistry& other) = delete;
		RenderGraphRegistry& operator=(const RenderGraphRegistry& other) = delete;

		RenderGraphRegistry(RenderGraphRegistry&& other) = delete;
		RenderGraphRegistry operator=(RenderGraphRegistry&& other) = delete;

		~RenderGraphRegistry() = default;

		[[nodiscard]] BufferID get_buffer(BufferNodeID bufferNodeID) const;
		[[nodiscard]] TextureID get_texture(TextureNodeID textureNodeId) const;
		[[nodiscard]] TlasID get_tlas(TlasNodeID tlas_node_id) const;
		PipelineStateID get_pipeline_state(const GraphicPipelineStateDesc& desc);
		PipelineStateID get_pipeline_state(const ComputePipelineStateDesc& desc);

	private:
		System& system_;
		const impl::RenderGraphExecution& execution_;
		VkRenderPass render_pass_;
		const TextureSampleCount sample_count_;
	};
}
