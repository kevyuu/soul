#pragma once

#include "../data.h"
#include "gpu/type.h"
#include "gpu/render_graph.h"
#include "draw_item.h"

namespace soul_fila
{
	struct LightingPass
	{
		struct Input
		{
			soul::gpu::BufferNodeID frameUb;
			soul::gpu::BufferNodeID lightsUb;
			soul::gpu::BufferNodeID shadowUb;
			soul::gpu::BufferNodeID froxelRecrodUb;
			soul::gpu::BufferNodeID objectsUb;
			soul::gpu::BufferNodeID bonesUb;
			soul::gpu::BufferNodeID materialsUb;
			soul::gpu::TextureNodeID structureTex;
			soul::gpu::TextureNodeID shadowMap;
		};

		struct Output
		{
			soul::gpu::TextureNodeID renderTarget;
			soul::gpu::TextureNodeID depthTarget;
		};

		void init(soul::gpu::System* gpuSystemIn, GPUProgramRegistry* programRegistryIn)
		{
			this->gpuSystem = gpuSystemIn;
			this->programRegistry = programRegistryIn;
		}
		soul::gpu::System* gpuSystem = nullptr;
		GPUProgramRegistry* programRegistry = nullptr;
		Array<DrawItem> drawItems;

		Output computeRenderGraph(soul::gpu::RenderGraph& renderGraph, const Input& input, const RenderData& renderData, const Scene& scene);

	};
}