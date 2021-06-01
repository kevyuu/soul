//
// Created by Kevin Yudi Utama on 4/1/20.
//

#pragma once

#include "gpu/data.h"
#include "gpu/render_graph.h"

namespace Soul { namespace GPU {
	class _RenderGraphExecution;

	class RenderGraphRegistry {
	public:

		RenderGraphRegistry() = delete;
		RenderGraphRegistry(System* system, const _RenderGraphExecution* execution, VkRenderPass renderPass) :
			_system(system), _execution(execution), _renderPass(renderPass) {}

		RenderGraphRegistry(const RenderGraphRegistry& other) = delete;
		RenderGraphRegistry& operator=(const RenderGraphRegistry& other) = delete;

		RenderGraphRegistry(RenderGraphRegistry&& other) = delete;
		RenderGraphRegistry operator=(RenderGraphRegistry&& other) = delete;

		~RenderGraphRegistry() = default;

		BufferID getBuffer(BufferNodeID bufferNodeID) const;
		TextureID getTexture(TextureNodeID textureNodeId) const;
		ShaderArgSetID  getShaderArgSet(uint32 set, const ShaderArgSetDesc& desc);
		PipelineStateID getPipelineState(const PipelineStateDesc& desc);

	private:
		System* _system;
		const _RenderGraphExecution* _execution;
		VkRenderPass _renderPass;
	};
}}
