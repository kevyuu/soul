//
// Created by Kevin Yudi Utama on 12/1/20.
//
#pragma once

#include <imgui/imgui.h>
#include "core/type.h"
#include "gpu/gpu.h"
#include "utils.h"
#include <map>

#include "ui/ui.h"

#include "runtime/scope_allocator.h"

using namespace soul;
using namespace Demo;

class ImGuiRenderModule {

public:
	struct Data {
		gpu::BufferNodeID vertexBuffer;
		gpu::BufferNodeID indexBuffer;
		gpu::BufferNodeID transformBuffer;
		gpu::TextureNodeID targetTex;
		std::map<gpu::TextureNodeID, gpu::TextureNodeID> imTextures;
	};

	void init(gpu::System* system) {
		runtime::ScopeAllocator<> scope_allocator("Imgui render module init");
		const char* vertSrc = LoadFile("shaders/imgui_render.vert.glsl", &scope_allocator);
		gpu::ShaderDesc vertShaderDesc;
		vertShaderDesc.name = "Imgui vertex shader";
		vertShaderDesc.source = vertSrc;
		vertShaderDesc.sourceSize = strlen(vertSrc);
		_vertShaderID = system->create_shader(vertShaderDesc, gpu::ShaderStage::VERTEX);

		const char* fragSrc = LoadFile("shaders/imgui_render.frag.glsl", &scope_allocator);
		gpu::ShaderDesc fragShaderDesc;
		fragShaderDesc.name = "Imgui fragment shader";
		fragShaderDesc.source = fragSrc;
		fragShaderDesc.sourceSize = strlen(fragSrc);
		_fragShaderID = system->create_shader(fragShaderDesc, gpu::ShaderStage::FRAGMENT);

		gpu::ProgramDesc programDesc;
		programDesc.shaderIDs[gpu::ShaderStage::VERTEX] = _vertShaderID;
		programDesc.shaderIDs[gpu::ShaderStage::FRAGMENT] = _fragShaderID;
		_programID = system->request_program(programDesc);

		unsigned char* fontPixels;
		int width, height;
		ImGuiIO io = ImGui::GetIO();
		io.Fonts->GetTexDataAsRGBA32(&fontPixels, &width, &height);
		const auto font_tex_desc = gpu::TextureDesc::D2("Font Texture", gpu::TextureFormat::RGBA8, 1, { gpu::TextureUsage::SAMPLED }, { gpu::QueueType::GRAPHIC }, Vec2ui32(width, height));

		gpu::TextureRegionLoad regionLoad;
		regionLoad.textureRegion.baseArrayLayer = 0;
		regionLoad.textureRegion.layerCount = 1;
		regionLoad.textureRegion.mipLevel = 0;
		regionLoad.textureRegion.extent = { soul::cast<uint32>(width), soul::cast<uint32>(height), 1 };

		gpu::TextureLoadDesc loadDesc;
		loadDesc.data = fontPixels;
		loadDesc.dataSize = width * height * 4 * sizeof(char);
		loadDesc.regionLoadCount = 1;
		loadDesc.regionLoads = &regionLoad;

		_fontTex = system->create_texture(font_tex_desc, loadDesc);

		const gpu::SamplerDesc samplerDesc = gpu::SamplerDesc::SameFilterWrap(gpu::TextureFilter::LINEAR, gpu::TextureWrap::CLAMP_TO_EDGE);
		_fontSampler = system->request_sampler(samplerDesc);

	}

	gpu::TextureID getFontTexture() {
		return _fontTex;
	}

	const Data& addPass(gpu::System* system, gpu::RenderGraph* renderGraph, const ImDrawData& drawData, gpu::TextureID targetTextureID) {
		if (drawData.TotalVtxCount == 0) return Data();
		runtime::ScopeAllocator<> scopeAllocator("imgui::addPass");

		int fb_width = (int)(drawData.DisplaySize.x * drawData.FramebufferScale.x);
		int fb_height = (int)(drawData.DisplaySize.y * drawData.FramebufferScale.y);
		Vec2ui32 fbDim;
		fbDim.x = fb_width;
		fbDim.y = fb_height;

		gpu::TextureNodeID targetTex = renderGraph->import_texture("Color output", targetTextureID);

		gpu::BufferDesc vertexBufferDesc;
		vertexBufferDesc.typeSize = sizeof(ImDrawVert);
		vertexBufferDesc.typeAlignment = alignof(ImDrawVert);
		vertexBufferDesc.count = drawData.TotalVtxCount;
		vertexBufferDesc.usageFlags = { gpu::BufferUsage::VERTEX };
		vertexBufferDesc.queueFlags = { gpu::QueueType::GRAPHIC };
		Array<ImDrawVert> imDrawVerts(&scopeAllocator);
		imDrawVerts.reserve(drawData.TotalVtxCount);
		for (soul_size cmdListIdx = 0; cmdListIdx < drawData.CmdListsCount; cmdListIdx++)
		{
			ImDrawList* cmdList = drawData.CmdLists[cmdListIdx];
			for (ImDrawVert& vertexBuffer : cmdList->VtxBuffer)
			{
				imDrawVerts.add(vertexBuffer);
			}
		}

		const gpu::BufferID vertexBuffer = system->create_buffer(vertexBufferDesc, imDrawVerts.data());
		system->destroy_buffer(vertexBuffer);
		gpu::BufferNodeID vertexNodeID = renderGraph->import_buffer("Vertex buffers", vertexBuffer);

		gpu::BufferDesc indexBufferDesc;
		indexBufferDesc.typeSize = sizeof(ImDrawIdx);
		indexBufferDesc.typeAlignment = alignof(ImDrawIdx);
		indexBufferDesc.count = drawData.TotalIdxCount;
		indexBufferDesc.usageFlags ={ gpu::BufferUsage::INDEX };
		indexBufferDesc.queueFlags = { gpu::QueueType::GRAPHIC };
		Array<ImDrawIdx> imDrawIndexes(&scopeAllocator);
		imDrawIndexes.reserve(drawData.TotalIdxCount);
		for (soul_size cmdListIdx = 0; cmdListIdx < drawData.CmdListsCount; cmdListIdx++)
		{
			ImDrawList* cmdList = drawData.CmdLists[cmdListIdx];
			for (ImDrawIdx& indexBuffer : cmdList->IdxBuffer)
			{
				imDrawIndexes.add(indexBuffer);
			}
		}
		const gpu::BufferID indexBuffer = system->create_buffer(indexBufferDesc ,imDrawIndexes.data());
		system->destroy_buffer(indexBuffer);
		gpu::BufferNodeID indexNodeID = renderGraph->import_buffer("Index Buffer", indexBuffer);

		struct TransformUBO {
			float scale[2];
			float translate[2];
		} transformUBO;
		transformUBO.scale[0] = 2.0f / drawData.DisplaySize.x;
		transformUBO.scale[1] = 2.0f / drawData.DisplaySize.y;
		transformUBO.translate[0] = -1.0f - drawData.DisplayPos.x * transformUBO.scale[0];
		transformUBO.translate[1] = -1.0f - drawData.DisplayPos.y * transformUBO.scale[1];
		gpu::BufferDesc transformBufferDesc;
		transformBufferDesc.typeSize = sizeof(TransformUBO);
		transformBufferDesc.typeAlignment = alignof(TransformUBO);
		transformBufferDesc.count = 1;
		transformBufferDesc.usageFlags = { gpu::BufferUsage::UNIFORM };
		transformBufferDesc.queueFlags = { gpu::QueueType::GRAPHIC };
		gpu::BufferID transformBufferID = system->create_buffer(transformBufferDesc, &transformUBO);
		gpu::BufferNodeID transformNodeID = renderGraph->import_buffer("Transform uBO", transformBufferID);
		system->destroy_buffer(transformBufferID);

		const gpu::ColorAttachmentDesc color_desc = {
			.nodeID = targetTex,
			.clear = true
		};

		return renderGraph->add_graphic_pass<Data>(
			"Imgui Pass",
			gpu::RGRenderTargetDesc(
				fbDim,
				color_desc
			),
			[this, vertexNodeID, indexNodeID, transformNodeID, drawData]
				(gpu::RGShaderPassDependencyBuilder& builder, Data& data) {

				data.vertexBuffer = builder.add_vertex_buffer(vertexNodeID);
				data.indexBuffer = builder.add_index_buffer(indexNodeID);
				data.transformBuffer = builder.add_shader_buffer(transformNodeID, { gpu::ShaderStage::VERTEX }, gpu::ShaderBufferReadUsage::UNIFORM);

				for (int n = 0; n < drawData.CmdListsCount; n++) {
					const ImDrawList& cmdList = *drawData.CmdLists[n];
					for (int cmd_i = 0; cmd_i < cmdList.CmdBuffer.Size; cmd_i++) {
						const ImDrawCmd& cmd = cmdList.CmdBuffer[cmd_i];
						gpu::TextureNodeID imTexture = UI::SoulImTexture(cmd.TextureId).getTextureNodeID();
						if (imTexture != gpu::TEXTURE_NODE_ID_NULL && data.imTextures.find(imTexture) == data.imTextures.end()) {
							data.imTextures[imTexture] = builder.add_shader_texture(imTexture, { gpu::ShaderStage::FRAGMENT }, gpu::ShaderTextureReadUsage::UNIFORM);
						}
					}
				}
				
			},
			[drawData, this, fbDim]
			(const Data& data, gpu::RenderGraphRegistry& registry, gpu::GraphicCommandList& command_list) {

				gpu::GraphicPipelineStateDesc pipelineDesc;
				pipelineDesc.programID = this->_programID;

				pipelineDesc.inputBindings[0] = { sizeof(ImDrawVert) };
				pipelineDesc.inputAttributes[0] = { 0, offsetof(ImDrawVert, pos) };
				pipelineDesc.inputAttributes[1] = { 0, offsetof(ImDrawVert, uv) };
				pipelineDesc.inputAttributes[2] = { 0, offsetof(ImDrawVert, col) };

				pipelineDesc.viewport.width = fbDim.x;
				pipelineDesc.viewport.height = fbDim.y;

				pipelineDesc.colorAttachmentCount = 1;
				gpu::GraphicPipelineStateDesc::ColorAttachmentDesc& targetAttchDesc = pipelineDesc.colorAttachments[0];
				targetAttchDesc.blendEnable = true;
				targetAttchDesc.srcColorBlendFactor = gpu::BlendFactor::SRC_ALPHA;
				targetAttchDesc.dstColorBlendFactor = gpu::BlendFactor::ONE_MINUS_SRC_ALPHA;
				targetAttchDesc.colorBlendOp = gpu::BlendOp::ADD;
				targetAttchDesc.srcAlphaBlendFactor = gpu::BlendFactor::ONE;
				targetAttchDesc.dstAlphaBlendFactor = gpu::BlendFactor::ZERO;
				targetAttchDesc.alphaBlendOp = gpu::BlendOp::ADD;

				// Will project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_off = drawData.DisplayPos;         // (0,0) unless using multi-viewports
				ImVec2 clip_scale = drawData.FramebufferScale; // (1,1) unless using retina display which are often (2,2)

				int global_vtx_offset = 0;
				int global_idx_offset = 0;

				gpu::Descriptor transformDescriptor = gpu::Descriptor::Uniform(registry.get_buffer(data.transformBuffer), 0, { gpu::ShaderStage::VERTEX });
				gpu::ShaderArgSetID argSet0 = registry.get_shader_arg_set(0, { 1, &transformDescriptor });

				using DrawCommand = gpu::RenderCommandDrawIndex;
				Array<DrawCommand> commands;

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

								
								pipelineDesc.scissor.offsetX = (int32_t)(clip_rect.x);
								pipelineDesc.scissor.offsetY = (int32_t)(clip_rect.y);
								pipelineDesc.scissor.width = (uint32_t)(clip_rect.z - clip_rect.x);
								pipelineDesc.scissor.height = (uint32_t)(clip_rect.w - clip_rect.y);

								UI::SoulImTexture soulImTexture(cmd.TextureId);
								gpu::Descriptor imageDescriptor = gpu::Descriptor::SampledImage(registry.get_texture(soulImTexture.getTextureNodeID()), _fontSampler, { gpu::ShaderStage::FRAGMENT });

								gpu::ShaderArgSetID argSet1 = registry.get_shader_arg_set(1, { 1, &imageDescriptor });

								const DrawCommand command = {
									.pipelineStateID = registry.get_pipeline_state(pipelineDesc),
									.shaderArgSetIDs = { argSet0, argSet1 },
									.vertexBufferID = registry.get_buffer(data.vertexBuffer),
									.indexBufferID = registry.get_buffer(data.indexBuffer),
									.indexOffset = soul::cast<uint16>(cmd.IdxOffset + global_idx_offset),
									.vertexOffset = soul::cast<uint16>(cmd.VtxOffset + global_vtx_offset),
									.indexCount = soul::cast<uint16>(cmd.ElemCount)
								};
								commands.add(command);
							}
						}
					}
					global_idx_offset += cmdList.IdxBuffer.Size;
					global_vtx_offset += cmdList.VtxBuffer.Size;
				}

				command_list.push(commands.size(), commands.data());
			}
		).get_parameter();

	}
	gpu::TextureID _fontTex;
private:
	gpu::ShaderID _vertShaderID;
	gpu::ShaderID _fragShaderID;
	gpu::ProgramID _programID;

	gpu::SamplerID _fontSampler;

};

