#include "core/dev_util.h"

#include "gpu/render_graph.h"

namespace Soul { namespace GPU {

	TextureNodeID RenderGraph::importTexture(const char* name, TextureID textureID) {
		TextureNodeID nodeID = TextureNodeID(_textureNodes.add(_TextureNode()));
		_TextureNode& node = _textureNodes.back();

		uint32 resourceIndex = _externalTextures.add(_RGExternalTexture());
		_RGExternalTexture& externalTexture = _externalTextures.back();
		externalTexture.name = name;
		externalTexture.textureID = textureID;

		node.resourceID = _RGResourceID::ExternalID(resourceIndex);

		return nodeID;
	}

	TextureNodeID RenderGraph::createTexture(const char* name, const RGTextureDesc& desc) {
		TextureNodeID nodeID = TextureNodeID(_textureNodes.add(_TextureNode()));
		_TextureNode& node = _textureNodes.back();

		uint32 resourceIndex = _internalTextures.add(_RGInternalTexture());
		_RGInternalTexture& internalTexture = _internalTextures.back();
		internalTexture.name = name;
		internalTexture.type = desc.type;
		internalTexture.width = desc.width;
		internalTexture.height = desc.height;
		internalTexture.depth = desc.depth;
		internalTexture.mipLevels = desc.mipLevels;
		internalTexture.format = desc.format;

		node.resourceID = _RGResourceID::InternalID(resourceIndex);

		return nodeID;
	}

	BufferNodeID RenderGraph::importBuffer(const char* name, BufferID bufferID) {
		BufferNodeID nodeID = BufferNodeID(_bufferNodes.add(_BufferNode()));
		_BufferNode& node = _bufferNodes.back();

		uint32 resourceIndex = _externalBuffers.add(_RGExternalBuffer());
		_RGExternalBuffer& externalBuffer = _externalBuffers.back();
		externalBuffer.name = name;
		externalBuffer.bufferID = bufferID;

		node.resourceID = _RGResourceID::ExternalID(resourceIndex);

		return nodeID;
	}

	BufferNodeID RenderGraph::createBuffer(const char* name, const RGBufferDesc& desc) {
		BufferNodeID nodeID = BufferNodeID(_bufferNodes.add(_BufferNode()));
		_BufferNode& node = _bufferNodes.back();

		uint32 resourceIndex = _internalBuffers.add(_RGInternalBuffer());
		_RGInternalBuffer& internalBuffer = _internalBuffers.back();
		internalBuffer.name = name;
		internalBuffer.count = desc.count;
		internalBuffer.typeSize = desc.typeSize;
		internalBuffer.typeAlignment = desc.typeAlignment;

		node.resourceID = _RGResourceID::InternalID(resourceIndex);

		return nodeID;
	}

	void RenderGraph::cleanup() {
		SOUL_PROFILE_ZONE();
		for (PassNode* passNode : _passNodes) {
			passNode->cleanup();
		}
		_passNodes.cleanup();

		for (_BufferNode& node : _bufferNodes) {
			node.readers.cleanup();
		}
		_bufferNodes.cleanup();

		for (_TextureNode& node : _textureNodes) {
			node.readers.cleanup();
		}
		_textureNodes.cleanup();

		_internalBuffers.cleanup();
		_internalTextures.cleanup();
		_externalBuffers.cleanup();
		_externalTextures.cleanup();
	}

	void RenderGraph::_bufferNodeRead(BufferNodeID bufferNodeID, PassNodeID passNodeID) {
		_bufferNodePtr((bufferNodeID))->readers.add(passNodeID);
	}

	BufferNodeID RenderGraph::_bufferNodeWrite(BufferNodeID bufferNodeID, PassNodeID passNodeID) {
		_bufferNodeRead(bufferNodeID, passNodeID);
		BufferNodeID dstBufferNodeID = BufferNodeID(_bufferNodes.add(_BufferNode()));
		_BufferNode* dstBufferNode = _bufferNodePtr(dstBufferNodeID);
		dstBufferNode->resourceID = _bufferNodePtr(bufferNodeID)->resourceID;
		dstBufferNode->writer = passNodeID;
		return dstBufferNodeID;
	}

	void RenderGraph::_textureNodeRead(TextureNodeID textureNodeID, PassNodeID passNodeID) {
		_textureNodePtr(textureNodeID)->readers.add(passNodeID);
	}

	TextureNodeID RenderGraph::_textureNodeWrite(TextureNodeID textureNodeID, PassNodeID passNodeID) {
		_textureNodeRead(textureNodeID, passNodeID);
		TextureNodeID dstTextureNodeID = TextureNodeID(_textureNodes.add(_TextureNode()));
		_TextureNode* dstTextureNode = _textureNodePtr(dstTextureNodeID);
		dstTextureNode->resourceID = _textureNodePtr(textureNodeID)->resourceID;
		dstTextureNode->writer = passNodeID;
		return dstTextureNodeID;
	}

	const _BufferNode* RenderGraph::_bufferNodePtr(BufferNodeID nodeID) const {
		return &_bufferNodes[nodeID.id];
	}

	_BufferNode* RenderGraph::_bufferNodePtr(BufferNodeID nodeID) {
		return &_bufferNodes[nodeID.id];
	}

	const _TextureNode* RenderGraph::_textureNodePtr(TextureNodeID nodeID) const {
		return &_textureNodes[nodeID.id];
	}

	_TextureNode* RenderGraph::_textureNodePtr(TextureNodeID nodeID) {
		return &_textureNodes[nodeID.id];
	}

	BufferNodeID GraphicNodeBuilder::addVertexBuffer(BufferNodeID nodeID) {
		_renderGraph->_bufferNodeRead(nodeID, _passID);
		_graphicNode->vertexBuffers.add(nodeID);
		return nodeID;
	}

	BufferNodeID GraphicNodeBuilder::addIndexBuffer(BufferNodeID nodeID) {
		_renderGraph->_bufferNodeRead(nodeID, _passID);
		_graphicNode->indexBuffers.add(nodeID);
		return nodeID;
	}

	BufferNodeID GraphicNodeBuilder::addInShaderBuffer(BufferNodeID nodeID, uint8 set, uint8 binding) {
		_renderGraph->_bufferNodeRead(nodeID, _passID);
		_graphicNode->inShaderBuffers.add(ShaderBuffer(nodeID, set, binding));
		return nodeID;
	}

	BufferNodeID GraphicNodeBuilder::addOutShaderBuffer(BufferNodeID nodeID, uint8 set, uint8 binding) {
		BufferNodeID dstBufferNodeID = _renderGraph->_bufferNodeWrite(nodeID, _passID);
		_graphicNode->outShaderBuffers.add(ShaderBuffer(dstBufferNodeID, set, binding));
		return dstBufferNodeID;
	}

	TextureNodeID GraphicNodeBuilder::addInShaderTexture(TextureNodeID nodeID, uint8 set, uint8 binding) {
		_renderGraph->_textureNodeRead(nodeID, _passID);
		_graphicNode->inShaderTextures.add(ShaderTexture(nodeID, set, binding));
		return nodeID;
	}

	TextureNodeID GraphicNodeBuilder::addOutShaderTexture(TextureNodeID nodeID, uint8 set, uint8 binding) {
		TextureNodeID outNodeID = _renderGraph->_textureNodeWrite(nodeID, _passID);
		_graphicNode->outShaderTextures.add(ShaderTexture(outNodeID, set, binding));
		return outNodeID;
	}

	TextureNodeID GraphicNodeBuilder::addInputAttachment(TextureNodeID nodeID) {
		_renderGraph->_textureNodeRead(nodeID, _passID);
		_graphicNode->inputAttachments.add(nodeID);
		return nodeID;
	}

	TextureNodeID GraphicNodeBuilder::addColorAttachment(TextureNodeID nodeID, const ColorAttachmentDesc& desc) {
		TextureNodeID outNodeID = _renderGraph->_textureNodeWrite(nodeID, _passID);
		_graphicNode->colorAttachments.add(ColorAttachment(nodeID, desc));
		return outNodeID;
	}

	TextureNodeID GraphicNodeBuilder::setDepthStencilAttachment(TextureNodeID nodeID, const DepthStencilAttachmentDesc& desc) {
		if (desc.depthWriteEnable) {
			TextureNodeID dstTextureNodeID = _renderGraph->_textureNodeWrite(nodeID, _passID);
			_graphicNode->depthStencilAttachment = DepthStencilAttachment(dstTextureNodeID, desc);
			return dstTextureNodeID;
		}
		_renderGraph->_textureNodeRead(nodeID, _passID);
		_graphicNode->depthStencilAttachment = DepthStencilAttachment(nodeID, desc);
		return nodeID;
	}

	void GraphicNodeBuilder::setPipelineConfig(const GraphicPipelineConfig &config) {
		_graphicNode->pipelineConfig = config;
	}
}}