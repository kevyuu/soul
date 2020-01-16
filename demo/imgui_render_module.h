//
// Created by Kevin Yudi Utama on 12/1/20.
//
#pragma once

#include <imgui/imgui.h>
#include "gpu/gpu.h"
#include "utils.h"
#include <stdio.h>

using namespace Soul;
class ImGuiRenderModule {
public:
	struct Data {
		GPU::BufferNodeID vertexBuffer;
		GPU::BufferNodeID indexBuffer;
		GPU::BufferNodeID transformBuffer;
		GPU::TextureNodeID fontTex;
		GPU::TextureNodeID targetTex;
	};

	void init(GPU::System* system, Vec2ui32 extent) {
		_extent = extent;

		const char* vertSrc = LoadFile("shaders/imgui_render.vert.glsl");
		GPU::ShaderDesc vertShaderDesc;
		vertShaderDesc.name = "Imgui vertex shader";
		vertShaderDesc.source = vertSrc;
		vertShaderDesc.sourceSize = strlen(vertSrc);
		_vertShaderID = system->shaderCreate(vertShaderDesc, GPU::ShaderStage::VERTEX);

		const char* fragSrc = LoadFile("shaders/imgui_render.frag.glsl");
		GPU::ShaderDesc fragShaderDesc;
		fragShaderDesc.name = "Imgui fragment shader";
		fragShaderDesc.source = fragSrc;
		fragShaderDesc.sourceSize = strlen(fragSrc);
		_fragShaderID = system->shaderCreate(fragShaderDesc, GPU::ShaderStage::FRAGMENT);

		unsigned char* fontPixels;
		int width, height;
		ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&fontPixels, &width, &height);
		GPU::TextureDesc fontTexDesc = {};
		fontTexDesc.type = GPU::TextureType::D2;
		fontTexDesc.format = GPU::TextureFormat::RGBA8;
		fontTexDesc.width = width;
		fontTexDesc.height = height;
		fontTexDesc.depth = 1;
		fontTexDesc.mipLevels = 1;
		fontTexDesc.usageFlags = GPU::TEXTURE_USAGE_SAMPLED_BIT;
		fontTexDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		_fontTex = system->textureCreate(fontTexDesc, (byte*) fontPixels, width * height * 4 * sizeof(char));

		GPU::SamplerDesc samplerDesc = {};
		samplerDesc.minFilter = GPU::TextureFilter::LINEAR;
		samplerDesc.magFilter = GPU::TextureFilter::LINEAR;
		samplerDesc.mipmapFilter = GPU::TextureFilter::LINEAR;
		samplerDesc.wrapU = GPU::TextureWrap::CLAMP_TO_EDGE;
		samplerDesc.wrapV = GPU::TextureWrap::CLAMP_TO_EDGE;
		samplerDesc.wrapW = GPU::TextureWrap::CLAMP_TO_EDGE;
		samplerDesc.anisotropyEnable = false;
		samplerDesc.maxAnisotropy = 0.0f;
		_fontSampler = system->samplerRequest(samplerDesc);

	}

	Data addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const ImDrawData& drawData) {
		if (drawData.TotalVtxCount == 0) return Data();
		int fb_width = (int)(drawData.DisplaySize.x * drawData.FramebufferScale.x);
		int fb_height = (int)(drawData.DisplaySize.y * drawData.FramebufferScale.y);
		_extent.x = fb_width;
		_extent.y = fb_height;

		GPU::RGTextureDesc desc;
		desc.width = _extent.x;
		desc.height = _extent.y;
		desc.clear = true;
		desc.clearValue = {};
		GPU::TextureNodeID targetTex = renderGraph->createTexture("Imgui layer", desc);

		GPU::BufferDesc vertexBufferDesc;
		vertexBufferDesc.typeSize = sizeof(ImDrawVert);
		vertexBufferDesc.typeAlignment = alignof(ImDrawVert);
		vertexBufferDesc.count = drawData.TotalVtxCount;
		vertexBufferDesc.usageFlags = GPU::BUFFER_USAGE_VERTEX_BIT;
		vertexBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		struct VertexIterator {
			const ImDrawData& _drawData;
			uint32 _cmdListIndex;
			uint32 _vertexIndex;

			VertexIterator(const ImDrawData& drawData) : _drawData(drawData), _cmdListIndex(0), _vertexIndex(0) {}
			const ImDrawVert& get() {
				while (_vertexIndex >= _drawData.CmdLists[_cmdListIndex]->VtxBuffer.size()) {
					_cmdListIndex++;
					_vertexIndex = 0;
				}
				return _drawData.CmdLists[_cmdListIndex]->VtxBuffer[_vertexIndex];
			}

			void next() {
				_vertexIndex++;
			}
		};
		VertexIterator vertIterator(drawData);
		_vertexBuffer = system->bufferCreate(vertexBufferDesc,
			[&vertIterator]
			(int i, byte* data){
			auto vertex = (ImDrawVert*) data;
			*vertex = vertIterator.get();
			vertIterator.next();
		});
		GPU::BufferNodeID vertexNodeID = renderGraph->importBuffer("Vertex buffer", _vertexBuffer);

		GPU::BufferDesc indexBufferDesc;
		indexBufferDesc.typeSize = sizeof(ImDrawIdx);
		indexBufferDesc.typeAlignment = alignof(ImDrawIdx);
		indexBufferDesc.count = drawData.TotalIdxCount;
		indexBufferDesc.usageFlags = GPU::BUFFER_USAGE_INDEX_BIT;
		indexBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		struct IndexIterator {
			const ImDrawData& _drawData;
			uint32 _cmdListIndex;
			uint32 _indexIndex;

			IndexIterator(const ImDrawData& drawData) : _drawData(drawData), _cmdListIndex(0), _indexIndex(0) {}
			const ImDrawIdx& get() {

				while (_indexIndex >= _drawData.CmdLists[_cmdListIndex]->IdxBuffer.size()) {
					_cmdListIndex++;
					_indexIndex = 0;
				}
				return _drawData.CmdLists[_cmdListIndex]->IdxBuffer[_indexIndex];

			}
			void next() {
				_indexIndex++;
			}
		};
		IndexIterator idxIterator(drawData);
		_indexBuffer = system->bufferCreate(indexBufferDesc,
			[&idxIterator]
			(int i, byte* data) {
			auto index = (ImDrawIdx*) data;
			*index = idxIterator.get();
			idxIterator.next();
		});
		GPU::BufferNodeID indexNodeID = renderGraph->importBuffer("Index buffer", _indexBuffer);

		GPU::TextureNodeID fontNodeID = renderGraph->importTexture("Font texture", _fontTex);

		struct TransformUBO {
			float scale[2];
			float translate[2];
		} transformUBO;
		transformUBO.scale[0] = 2.0f / drawData.DisplaySize.x;
		transformUBO.scale[1] = 2.0f / drawData.DisplaySize.y;
		transformUBO.translate[0] = -1.0f - drawData.DisplayPos.x * transformUBO.scale[0];
		transformUBO.translate[1] = -1.0f - drawData.DisplayPos.y * transformUBO.scale[1];
		GPU::BufferDesc transformBufferDesc;
		transformBufferDesc.typeSize = sizeof(TransformUBO);
		transformBufferDesc.typeAlignment = alignof(TransformUBO);
		transformBufferDesc.count = 1;
		transformBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		transformBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		GPU::BufferID transformBufferID = system->bufferCreate(transformBufferDesc,
			[&transformUBO]
			(int i, byte* data) {
			auto transform = (TransformUBO*) data;
			*transform = transformUBO;
		});
		GPU::BufferNodeID transformNodeID = renderGraph->importBuffer("Transform UBO", transformBufferID);

		system->bufferDestroy(_vertexBuffer);
		system->bufferDestroy(_indexBuffer);
		system->bufferDestroy(transformBufferID);

		return renderGraph->addGraphicPass<Data>(
			"Imgui Pass",
			[this, targetTex, vertexNodeID, indexNodeID, fontNodeID, transformNodeID, drawData]
			(GPU::GraphicNodeBuilder* builder, Data* data) {
				GPU::ColorAttachmentDesc targetAttchDesc;
				targetAttchDesc.clear = true;
				targetAttchDesc.clearValue = {};
				targetAttchDesc.blendEnable = true;
				targetAttchDesc.srcColorBlendFactor = GPU::BlendFactor::SRC_ALPHA;
				targetAttchDesc.dstColorBlendFactor = GPU::BlendFactor::ONE_MINUS_SRC_ALPHA;
				targetAttchDesc.colorBlendOp = GPU::BlendOp::ADD;
				targetAttchDesc.srcAlphaBlendFactor = GPU::BlendFactor::ONE;
				targetAttchDesc.dstAlphaBlendFactor = GPU::BlendFactor::ZERO;
				targetAttchDesc.alphaBlendOp = GPU::BlendOp::ADD;
				data->targetTex = builder->addColorAttachment(targetTex, targetAttchDesc);
				data->vertexBuffer = builder->addVertexBuffer(vertexNodeID);
				data->indexBuffer = builder->addIndexBuffer(indexNodeID);
				data->fontTex = builder->addInShaderTexture(fontNodeID, 0, 1);
				data->transformBuffer = builder->addInShaderBuffer(transformNodeID, 0, 0);

				GPU::GraphicPipelineConfig config;
				config.framebuffer.width = _extent.x;
				config.framebuffer.height = _extent.y;

				config.vertexShaderID = _vertShaderID;
				config.fragmentShaderID = _fragShaderID;

				config.viewport.width = _extent.x;
				config.viewport.height = _extent.y;

				config.scissor.dynamic = true;
				config.scissor.width = _extent.x;
				config.scissor.height = _extent.y;

				builder->setPipelineConfig(config);
			},
			[drawData, this]
			(GPU::RenderGraphRegistry* registry, const Data& data, GPU::CommandBucket* commandBucket) {
				// Will project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_off = drawData.DisplayPos;         // (0,0) unless using multi-viewports
				ImVec2 clip_scale = drawData.FramebufferScale; // (1,1) unless using retina display which are often (2,2)

				int global_vtx_offset = 0;
				int global_idx_offset = 0;

				GPU::Descriptor descriptors[2];

				GPU::Descriptor& transformDescriptor = descriptors[0];
				transformDescriptor.type = GPU::DescriptorType::UNIFORM_BUFFER;
				transformDescriptor.uniformInfo.bufferID = registry->getBuffer(data.transformBuffer);
				transformDescriptor.uniformInfo.unitIndex = 0;

				GPU::Descriptor& fontDescriptor = descriptors[1];
				fontDescriptor.type = GPU::DescriptorType::SAMPLED_IMAGE;
				fontDescriptor.sampledImageInfo.textureID = registry->getTexture(data.fontTex);
				fontDescriptor.sampledImageInfo.samplerID = _fontSampler;

				GPU::ShaderArgSetDesc argSetDesc;
				argSetDesc.bindingCount = 2;
				argSetDesc.bindingDescriptions = descriptors;
				GPU::ShaderArgSetID argSetID = registry->getShaderArgSet(0, argSetDesc);

				uint32 scissorChange = 0;
				for (int n = 0; n < drawData.CmdListsCount; n++) {
					const ImDrawList& cmdList = *drawData.CmdLists[n];
					for (int cmd_i = 0; cmd_i < cmdList.CmdBuffer.Size; cmd_i++) {
						const ImDrawCmd& cmd = cmdList.CmdBuffer[cmd_i];
						if (cmd.UserCallback != NULL) {
							SOUL_NOT_IMPLEMENTED();
						}
						else {
							// Project scissor/clipping rectangles into framebuffer space
							ImVec4 clip_rect;
							clip_rect.x = (cmd.ClipRect.x - clip_off.x) * clip_scale.x;
							clip_rect.y = (cmd.ClipRect.y - clip_off.y) * clip_scale.y;
							clip_rect.z = (cmd.ClipRect.z - clip_off.x) * clip_scale.x;
							clip_rect.w = (cmd.ClipRect.w - clip_off.y) * clip_scale.y;

							if (clip_rect.x < _extent.x && clip_rect.y < _extent.y && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
							{
								if (clip_rect.x < 0.0f)
									clip_rect.x = 0.0f;
								if (clip_rect.y < 0.0f)
									clip_rect.y = 0.0f;

								using DrawCommand = GPU::Command::DrawIndexRegion;
								DrawCommand* command = commandBucket->add<DrawCommand>(n);
								command->scissor.offsetX = (int32_t)(clip_rect.x);
								command->scissor.offsetY = (int32_t)(clip_rect.y);
								command->scissor.width = (uint32_t)(clip_rect.z - clip_rect.x);
								command->scissor.height = (uint32_t)(clip_rect.w - clip_rect.y);

								command->vertexBufferID = registry->getBuffer(data.vertexBuffer);
								command->vertexOffset = cmd.VtxOffset + global_vtx_offset;
								command->indexBufferID = registry->getBuffer(data.indexBuffer);
								command->indexCount = cmd.ElemCount;
								command->indexOffset = cmd.IdxOffset + global_idx_offset;

								command->shaderArgSets[0] = argSetID;
								++scissorChange;
							}
						}
					}
					global_idx_offset += cmdList.IdxBuffer.Size;
					global_vtx_offset += cmdList.VtxBuffer.Size;
				}
				SOUL_LOG_INFO("Scissor change = %d", scissorChange);
			}
		);

	}
private:
	Vec2ui32 _extent;
	GPU::ShaderID _vertShaderID;
	GPU::ShaderID _fragShaderID;

	GPU::BufferID _vertexBuffer;
	GPU::BufferID _indexBuffer;

	GPU::TextureID _fontTex;
	GPU::SamplerID _fontSampler;
};

