#include "core/dev_util.h"

#include "gpu/render_graph.h"

namespace Soul::GPU
{

	using namespace impl;

	TextureNodeID RenderGraph::importTexture(const char* name, TextureID textureID) {
		TextureNodeID nodeID = TextureNodeID(_textureNodes.add(TextureNode()));
		TextureNode& node = _textureNodes.back();

		uint32 resourceIndex = _externalTextures.add(RGExternalTexture());
		RGExternalTexture& externalTexture = _externalTextures.back();
		externalTexture.name = name;
		externalTexture.textureID = textureID;

		node.resourceID = RGResourceID::ExternalID(resourceIndex);

		return nodeID;
	}

	TextureNodeID RenderGraph::createTexture(const char* name, const RGTextureDesc& desc) {
		TextureNodeID nodeID = TextureNodeID(_textureNodes.add(TextureNode()));
		TextureNode& node = _textureNodes.back();

		uint32 resourceIndex = _internalTextures.add(RGInternalTexture());
		RGInternalTexture& internalTexture = _internalTextures.back();
		internalTexture.name = name;
		internalTexture.type = desc.type;
		internalTexture.width = desc.width;
		internalTexture.height = desc.height;
		internalTexture.depth = desc.depth;
		internalTexture.mipLevels = desc.mipLevels;
		internalTexture.format = desc.format;
		internalTexture.clear = desc.clear;
		internalTexture.clearValue = desc.clearValue;

		node.resourceID = RGResourceID::InternalID(resourceIndex);

		return nodeID;
	}

	BufferNodeID RenderGraph::importBuffer(const char* name, BufferID bufferID) {
		BufferNodeID nodeID = BufferNodeID(_bufferNodes.add(BufferNode()));
		BufferNode& node = _bufferNodes.back();

		uint32 resourceIndex = _externalBuffers.add(RGExternalBuffer());
		RGExternalBuffer& externalBuffer = _externalBuffers.back();
		externalBuffer.name = name;
		externalBuffer.bufferID = bufferID;

		node.resourceID = RGResourceID::ExternalID(resourceIndex);

		return nodeID;
	}

	BufferNodeID RenderGraph::createBuffer(const char* name, const RGBufferDesc& desc) {
		BufferNodeID nodeID = BufferNodeID(_bufferNodes.add(BufferNode()));
		BufferNode& node = _bufferNodes.back();

		uint32 resourceIndex = _internalBuffers.add(RGInternalBuffer());
		RGInternalBuffer& internalBuffer = _internalBuffers.back();
		internalBuffer.name = name;
		internalBuffer.count = desc.count;
		internalBuffer.typeSize = desc.typeSize;
		internalBuffer.typeAlignment = desc.typeAlignment;

		node.resourceID = RGResourceID::InternalID(resourceIndex);

		return nodeID;
	}

	void RenderGraph::exportTexture(TextureNodeID tex, char* pixels) {
		_textureExports.add({ tex, pixels });
	}

	void RenderGraph::cleanup() {
		SOUL_PROFILE_ZONE();
		for (PassNode* passNode : _passNodes) {
			passNode->cleanup();
			delete passNode;
		}
		_passNodes.cleanup();

		_bufferNodes.cleanup();

		_textureNodes.cleanup();

		_internalBuffers.cleanup();
		_internalTextures.cleanup();
		_externalBuffers.cleanup();
		_externalTextures.cleanup();
	}

	void RenderGraph::_bufferNodeRead(BufferNodeID bufferNodeID, PassNodeID passNodeID) {
		// _bufferNodePtr((bufferNodeID))->readers.add(passNodeID);
	}

	BufferNodeID RenderGraph::_bufferNodeWrite(BufferNodeID bufferNodeID, PassNodeID passNodeID) {
		_bufferNodeRead(bufferNodeID, passNodeID);
		BufferNodeID dstBufferNodeID = BufferNodeID(_bufferNodes.add(BufferNode()));
		BufferNode* dstBufferNode = _bufferNodePtr(dstBufferNodeID);
		dstBufferNode->resourceID = _bufferNodePtr(bufferNodeID)->resourceID;
		dstBufferNode->writer = passNodeID;
		return dstBufferNodeID;
	}

	void RenderGraph::_textureNodeRead(TextureNodeID textureNodeID, PassNodeID passNodeID) {
		// _textureNodePtr(textureNodeID)->readers.add(passNodeID);
	}

	TextureNodeID RenderGraph::_textureNodeWrite(TextureNodeID textureNodeID, PassNodeID passNodeID) {
		_textureNodeRead(textureNodeID, passNodeID);
		TextureNodeID dstTextureNodeID = TextureNodeID(_textureNodes.add(TextureNode()));
		TextureNode* dstTextureNode = _textureNodePtr(dstTextureNodeID);
		dstTextureNode->resourceID = _textureNodePtr(textureNodeID)->resourceID;
		dstTextureNode->writer = passNodeID;
		return dstTextureNodeID;
	}

	const BufferNode* RenderGraph::_bufferNodePtr(BufferNodeID nodeID) const {
		return &_bufferNodes[nodeID.id];
	}

	BufferNode* RenderGraph::_bufferNodePtr(BufferNodeID nodeID) {
		return &_bufferNodes[nodeID.id];
	}

	const TextureNode* RenderGraph::_textureNodePtr(TextureNodeID nodeID) const {
		return &_textureNodes[nodeID.id];
	}

	TextureNode* RenderGraph::_textureNodePtr(TextureNodeID nodeID) {
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

	BufferNodeID GraphicNodeBuilder::addShaderBuffer(BufferNodeID nodeID, ShaderStageFlags stageFlags, ShaderBufferReadUsage usage) {
		_renderGraph->_bufferNodeRead(nodeID, _passID);
		_graphicNode->shaderBufferReadAccesses.add({ nodeID, stageFlags, usage });
		return nodeID;
	}

	BufferNodeID GraphicNodeBuilder::addShaderBuffer(BufferNodeID nodeID, ShaderStageFlags stageFlags, ShaderBufferWriteUsage usage) {
		BufferNodeID outNodeID = _renderGraph->_bufferNodeWrite(nodeID, _passID);
		_graphicNode->shaderBufferWriteAccesses.add({ nodeID, stageFlags, usage });
		return nodeID;
	}

	TextureNodeID GraphicNodeBuilder::addShaderTexture(TextureNodeID nodeID, ShaderStageFlags stageFlags, ShaderTextureReadUsage usage) {
		_renderGraph->_textureNodeRead(nodeID, _passID);
		_graphicNode->shaderTextureReadAccesses.add({ nodeID, stageFlags, usage });
		return nodeID;
	}

	TextureNodeID GraphicNodeBuilder::addShaderTexture(TextureNodeID nodeID, ShaderStageFlags stageFlags, ShaderTextureWriteUsage usage) {
		TextureNodeID outNodeID = _renderGraph->_textureNodeWrite(nodeID, _passID);
		_graphicNode->shaderTextureWriteAccesses.add({ outNodeID, stageFlags, usage });
		return outNodeID;
	}

	TextureNodeID GraphicNodeBuilder::addInputAttachment(TextureNodeID nodeID, ShaderStageFlags stageFlags) {
		_renderGraph->_textureNodeRead(nodeID, _passID);
		_graphicNode->inputAttachments.add(InputAttachment(nodeID, stageFlags));
		return nodeID;
	}

	TextureNodeID GraphicNodeBuilder::addColorAttachment(TextureNodeID nodeID, const ColorAttachmentDesc& desc) {
		TextureNodeID outNodeID = _renderGraph->_textureNodeWrite(nodeID, _passID);
		_graphicNode->colorAttachments.add(ColorAttachment(nodeID, desc));
		return outNodeID;
	}

	TextureNodeID GraphicNodeBuilder::setDepthStencilAttachment(TextureNodeID nodeID, const DepthStencilAttachmentDesc& desc) {
		if (desc.depthWriteEnable || desc.clear) {
			TextureNodeID dstTextureNodeID = _renderGraph->_textureNodeWrite(nodeID, _passID);
			_graphicNode->depthStencilAttachment = DepthStencilAttachment(dstTextureNodeID, desc);
			return dstTextureNodeID;
		}
		_renderGraph->_textureNodeRead(nodeID, _passID);
		_graphicNode->depthStencilAttachment = DepthStencilAttachment(nodeID, desc);
		return nodeID;
	}

	void GraphicNodeBuilder::setRenderTargetDimension(Vec2ui32 dimension) {
		_graphicNode->renderTargetDimension = dimension;
	}

	BufferNodeID ComputeNodeBuilder::addShaderBuffer(BufferNodeID nodeID, ShaderStageFlags stageFlags, ShaderBufferReadUsage usage) {
		_renderGraph->_bufferNodeRead(nodeID, _passID);
		_computeNode->shaderBufferReadAccesses.add({ nodeID, stageFlags, usage });
		return nodeID;
	}

	BufferNodeID ComputeNodeBuilder::addShaderBuffer(BufferNodeID nodeID, ShaderStageFlags stageFlags, ShaderBufferWriteUsage usage) {
		BufferNodeID outNodeID = _renderGraph->_bufferNodeWrite(nodeID, _passID);
		_computeNode->shaderBufferWriteAccesses.add({ nodeID, stageFlags, usage });
		return nodeID;
	}

	TextureNodeID ComputeNodeBuilder::addShaderTexture(TextureNodeID nodeID, ShaderStageFlags stageFlags, ShaderTextureReadUsage usage) {
		_renderGraph->_textureNodeRead(nodeID, _passID);
		_computeNode->shaderTextureReadAccesses.add({ nodeID, stageFlags, usage });
		return nodeID;
	}

	TextureNodeID ComputeNodeBuilder::addShaderTexture(TextureNodeID nodeID, ShaderStageFlags stageFlags, ShaderTextureWriteUsage usage) {
		TextureNodeID outNodeID = _renderGraph->_textureNodeWrite(nodeID, _passID);
		_computeNode->shaderTextureWriteAccesses.add({ outNodeID, stageFlags, usage });
		return outNodeID;
	}

	void ComputeNodeBuilder::setPipelineConfig(const ComputePipelineConfig &config) {
		_computeNode->pipelineConfig = config;
	}
}
