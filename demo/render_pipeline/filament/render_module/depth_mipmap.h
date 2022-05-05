#pragma once

#include "../data.h"
#include "gpu/type.h"
#include "gpu/render_graph.h"

namespace soul_fila
{
	class DepthMipmapPass
	{
	public:

		struct Input
		{
			soul::gpu::TextureNodeID depthMap;
		};

		struct Output
		{
			soul::gpu::TextureNodeID depthMap;
		};

		void init(soul::gpu::System* gpuSystemIn);
		Output computeRenderGraph(soul::gpu::RenderGraph& render_graph, const Input& input, const RenderData& render_data, const Scene& scene);
		
	private:
		soul::gpu::ProgramID programID;
		soul::gpu::System* gpuSystem = nullptr;
	};
}
