#pragma once

#include "../data.h"
#include "gpu/type.h"
#include "gpu/render_graph.h"

namespace SoulFila
{
	class GPUProgramRegistry;

	struct StructurePass
	{
		struct Input
		{
			soul::gpu::BufferNodeID frameUb;
			soul::gpu::BufferNodeID objectsUb;
			soul::gpu::BufferNodeID bonesUb;
			soul::gpu::BufferNodeID materialsUb;
		};

		struct Output
		{
			soul::gpu::TextureNodeID depthTarget;
		};

		void init(soul::gpu::System* gpuSystemIn, GPUProgramRegistry* programRegistryIn)
		{
			this->gpuSystem = gpuSystemIn;
			this->programRegistry = programRegistryIn;
		}

		soul::gpu::System* gpuSystem = nullptr;
		GPUProgramRegistry* programRegistry = nullptr;
		
		Output computeRenderGraph(soul::gpu::RenderGraph& renderGraph, const Input& innput, const RenderData& renderData, const Scene& scene);

	};
}