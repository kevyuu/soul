//
// Created by Kevin Yudi Utama on 12/1/20.
//
#pragma once

#include <imgui/imgui.h>
#include "core/type.h"
#include "gpu/gpu.h"
#include "utils.h"
#include <stdio.h>
#include <map>

using namespace Soul;

struct SoulImTexture {
public:

	union Val{
	
		GPU::TextureNodeID renderGraphTex = GPU::TEXTURE_NODE_ID_NULL;
		ImTextureID imTextureID;

		Val() {};
	} val;

	static_assert(sizeof(ImTextureID) <= sizeof(val), "");
	
	SoulImTexture() = default;


	SoulImTexture(GPU::TextureNodeID texNodeID) {
		val.renderGraphTex = texNodeID;
	}

	SoulImTexture(ImTextureID imTextureID) {
		val.imTextureID = imTextureID;
	}

	ImTextureID getImTextureID() {
		return val.imTextureID;
	}

	GPU::TextureNodeID getTextureNodeID() {
		return val.renderGraphTex;
	}

};
static_assert(sizeof(SoulImTexture) <= sizeof(ImTextureID), "Size of SoulImTexture must be less that ImTextureID");

class ImGuiRenderModule {

public:
	struct Data {
		GPU::BufferNodeID vertexBuffer;
		GPU::BufferNodeID indexBuffer;
		GPU::BufferNodeID transformBuffer;
		GPU::TextureNodeID targetTex;
		std::map<GPU::TextureNodeID, GPU::TextureNodeID> imTextures;
	};

	void init(GPU::System* system) {

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
		ImGuiIO io = ImGui::GetIO();
		io.Fonts->GetTexDataAsRGBA32(&fontPixels, &width, &height);
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

	GPU::TextureID getFontTexture() {
		return _fontTex;
	}

	const Data& addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const ImDrawData& drawData, GPU::TextureID targetTextureID) {
		if (drawData.TotalVtxCount == 0) return Data();
		int fb_width = (int)(drawData.DisplaySize.x * drawData.FramebufferScale.x);
		int fb_height = (int)(drawData.DisplaySize.y * drawData.FramebufferScale.y);
		Vec2ui32 fbDim;
		fbDim.x = fb_width;
		fbDim.y = fb_height;

		GPU::TextureNodeID targetTex = renderGraph->importTexture("Color output", targetTextureID);

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
		GPU::BufferID _vertexBuffer = system->bufferCreate(vertexBufferDesc,
			[&vertIterator]
			(int i, byte* data){
			auto vertex = (ImDrawVert*) data;
			*vertex = vertIterator.get();
			vertIterator.next();
		});
		GPU::BufferNodeID vertexNodeID = renderGraph->importBuffer("Vertex buffers", _vertexBuffer);

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
		GPU::BufferID _indexBuffer = system->bufferCreate(indexBufferDesc,
			[&idxIterator]
			(int i, byte* data) {
			auto index = (ImDrawIdx*) data;
			*index = idxIterator.get();
			idxIterator.next();
		});
		GPU::BufferNodeID indexNodeID = renderGraph->importBuffer("Index buffer", _indexBuffer);

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
			[this, targetTex, vertexNodeID, indexNodeID, transformNodeID, drawData, fbDim]
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
				data->transformBuffer = builder->addInShaderBuffer(transformNodeID, 0, 0);

				for (int n = 0; n < drawData.CmdListsCount; n++) {
					const ImDrawList& cmdList = *drawData.CmdLists[n];
					for (int cmd_i = 0; cmd_i < cmdList.CmdBuffer.Size; cmd_i++) {
						const ImDrawCmd& cmd = cmdList.CmdBuffer[cmd_i];
						GPU::TextureNodeID imTexture = SoulImTexture(cmd.TextureId).getTextureNodeID();
						if (data->imTextures.find(imTexture) == data->imTextures.end()) {
							data->imTextures[imTexture] = builder->addInShaderTexture(imTexture, 1, 0);
						}
					}
				}

				GPU::GraphicPipelineConfig config;
				config.framebuffer.width = fbDim.x;
				config.framebuffer.height = fbDim.y;

				config.vertexShaderID = _vertShaderID;
				config.fragmentShaderID = _fragShaderID;

				config.viewport.width = fbDim.x;
				config.viewport.height = fbDim.y;

				config.scissor.dynamic = true;
				config.scissor.width = fbDim.x;
				config.scissor.height = fbDim.y;

				builder->setPipelineConfig(config);
			},
			[drawData, this, fbDim]
			(GPU::RenderGraphRegistry* registry, const Data& data, GPU::CommandBucket* commandBucket) {
				// Will project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_off = drawData.DisplayPos;         // (0,0) unless using multi-viewports
				ImVec2 clip_scale = drawData.FramebufferScale; // (1,1) unless using retina display which are often (2,2)

				int global_vtx_offset = 0;
				int global_idx_offset = 0;

				GPU::Descriptor transformDescriptor = {};
				transformDescriptor.type = GPU::DescriptorType::UNIFORM_BUFFER;
				transformDescriptor.uniformInfo.bufferID = registry->getBuffer(data.transformBuffer);
				transformDescriptor.uniformInfo.unitIndex = 0;

				GPU::ShaderArgSetDesc argSetDesc = {};
				argSetDesc.bindingCount = 1;
				argSetDesc.bindingDescriptions = &transformDescriptor;
				GPU::ShaderArgSetID argSet0 = registry->getShaderArgSet(0, argSetDesc);

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

							if (clip_rect.x < fbDim.x && clip_rect.y < fbDim.y && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
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

								command->shaderArgSets[0] = argSet0;

								GPU::Descriptor imageDescriptor = {};
								imageDescriptor.type = GPU::DescriptorType::SAMPLED_IMAGE;
								SoulImTexture soulImTexture(cmd.TextureId);
								imageDescriptor.sampledImageInfo.textureID = registry->getTexture(soulImTexture.getTextureNodeID());
								imageDescriptor.sampledImageInfo.samplerID = _fontSampler;

								GPU::ShaderArgSetDesc argSet1Desc = {};
								argSet1Desc.bindingCount = 1;
								argSet1Desc.bindingDescriptions = &imageDescriptor;

								GPU::ShaderArgSetID argSet1 = registry->getShaderArgSet(1, argSet1Desc);
								command->shaderArgSets[1] = argSet1;
							}
						}
					}
					global_idx_offset += cmdList.IdxBuffer.Size;
					global_vtx_offset += cmdList.VtxBuffer.Size;
				}
			}
		);

	}
	GPU::TextureID _fontTex;
private:
	GPU::ShaderID _vertShaderID;
	GPU::ShaderID _fragShaderID;

	GPU::SamplerID _fontSampler;

};

