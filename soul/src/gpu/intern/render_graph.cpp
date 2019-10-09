#include "gpu/render_graph.h"

namespace Soul { namespace GPU {

	TextureNodeID RenderGraph::importTexture(const char* name, TextureID textureID) {
		return 0;
	}

	TextureNodeID RenderGraph::createTexture(const char* name, const RenderGraphTextureDesc& desc, TextureWriteUsage usage) {
		RenderGraphTextureID texID = textures.add({
			.name = name,
			.width = desc.width,
			.height = desc.height,
			.format = desc.format,
			.clear = desc.clear,
			.clearValue = desc.clearValue
		});
		TextureNodeID texNodeID = textureNodes.add({
			.resourceID = texID,
			.writer = (uint16)passNodes.size()
		});
		passNodes.back()->textureWrites.add({
			.nodeID = texNodeID,
			.usage = usage
		});
		return texNodeID;
	}

	TextureNodeID RenderGraph::writeTexture(TextureNodeID textureNodeID, TextureWriteUsage usage) {
		TextureNode srcTexture = textureNodes[textureNodeID];
		srcTexture.readers.add(passNodes.size());
		TextureNodeID dstTextureNodeID = textureNodes.add({
			.resourceID = srcTexture.resourceID,
			.writer = (uint16)passNodes.size()
		});

		return dstTextureNodeID;
	}
	
	TextureNodeID RenderGraph::readTexture(TextureNodeID textureNodeID, TextureReadUsage usage) {
		textureNodes[textureNodeID].readers.add(passNodes.size());
		passNodes.back()->textureReads.add({
			.nodeID = textureNodeID,
			.usage = usage
		});
		return textureNodeID;
	}

	BufferNodeID RenderGraph::writeBuffer(BufferNodeID bufferNodeID, BufferWriteUsage usage) {
		return 0;
	}

	BufferNodeID RenderGraph::readBuffer(BufferNodeID bufferNodeID, BufferReadUsage usage) {
		return 0;
	}

}}