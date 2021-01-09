//
// Created by Kevin Yudi Utama on 4/1/20.
//
#include "core/dev_util.h"

#include "runtime/system.h"

#include "gpu/render_graph_registry.h"
#include "gpu/system.h"
#include "gpu/intern/render_graph_execution.h"

namespace Soul {namespace GPU {
	BufferID RenderGraphRegistry::getBuffer(BufferNodeID bufferNodeID) const {
		return _execution->getBufferID(bufferNodeID);
	}

	TextureID RenderGraphRegistry::getTexture(TextureNodeID textureNodeId) const {
		return _execution->getTextureID(textureNodeId);
	}

	ShaderArgSetID RenderGraphRegistry::getShaderArgSet(uint32 set, const ShaderArgSetDesc &desc) {
		return _system->_shaderArgSetRequest(desc);
	}
}}
