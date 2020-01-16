//
// Created by Kevin Yudi Utama on 4/1/20.
//
#include "core/dev_util.h"
#include "core/math.h"

#include "job/system.h"

#include "gpu/render_graph_registry.h"
#include "gpu/system.h"
#include "gpu/intern/render_graph_execution.h"

#include <volk/volk.h>

namespace Soul {namespace GPU {
	BufferID RenderGraphRegistry::getBuffer(BufferNodeID bufferNodeID) const {
		SOUL_ASSERT_MAIN_THREAD();
		return _execution->getBufferID(bufferNodeID);
	}

	TextureID RenderGraphRegistry::getTexture(TextureNodeID textureNodeId) const {
		SOUL_ASSERT_MAIN_THREAD();
		return _execution->getTextureID(textureNodeId);
	}

	ShaderArgSetID RenderGraphRegistry::getShaderArgSet(uint32 set, const ShaderArgSetDesc &desc) {
		SOUL_ASSERT_MAIN_THREAD();
		return _system->_shaderArgSetRequest(desc, _programID, set);
	}
}}
