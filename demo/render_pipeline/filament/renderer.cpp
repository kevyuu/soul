#include "renderer.h"
#include "core/dev_util.h"
#include "gpu_program_registry.h"
#include "runtime/scope_allocator.h"

using namespace Soul;

namespace SoulFila {

	void Renderer::init() {
		GPU::TextureDesc texDesc;
		texDesc.width = 1;
		texDesc.height = 1;
		texDesc.depth = 1;
		texDesc.type = GPU::TextureType::D2;
		texDesc.format = GPU::TextureFormat::RGBA8;
		texDesc.mipLevels = 1;
		texDesc.usageFlags = GPU::TEXTURE_USAGE_SAMPLED_BIT;
		texDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		texDesc.name = "Stub Texture";
		uint32 stubTextureData = 0;
		_stubTexture = _gpuSystem->textureCreate(texDesc, (byte*)&stubTextureData, sizeof(stubTextureData));
	}

	GPU::TextureNodeID Renderer::computeRenderGraph(GPU::RenderGraph* renderGraph) {

		Runtime::ScopeAllocator<> scopeAllocator("computeRenderGraph");

		Vec2ui32 sceneResolution = _scene.getViewport();

		GPU::RGTextureDesc renderTargetDesc;
		renderTargetDesc.width = sceneResolution.x;
		renderTargetDesc.height = sceneResolution.y;
		renderTargetDesc.depth = 1;
		renderTargetDesc.clear = true;
		renderTargetDesc.clearValue = {};
		renderTargetDesc.format = GPU::TextureFormat::RGBA8;
		renderTargetDesc.mipLevels = 1;
		renderTargetDesc.type = GPU::TextureType::D2;
		GPU::TextureNodeID finalRenderTarget = renderGraph->createTexture("Final Render Target", renderTargetDesc);

		// TODO: Should remove this in the future. make this function run even when there is no object or use better check
		if (_scene.meshes().size() == 0) {
			return finalRenderTarget;
		}
		
		struct Parameter {
			Soul::Array<Soul::GPU::BufferNodeID> vertexBuffers;
			Soul::Array<Soul::GPU::BufferNodeID> indexBuffers;
			Soul::Array<Soul::GPU::TextureNodeID> sceneTextures;
			Soul::GPU::BufferNodeID frameUniformBuffer;
			Soul::GPU::BufferNodeID lightUniformBuffer;
			Soul::GPU::BufferNodeID froxelRecordsUniformBuffer;
			Soul::GPU::BufferNodeID objectUniformBuffer;
			Soul::GPU::BufferNodeID boneUniformBuffer;
			Soul::GPU::BufferNodeID materialUniformBuffer;
			Soul::GPU::TextureNodeID renderTarget;
			Soul::GPU::TextureNodeID depthTarget;
			Soul::GPU::TextureNodeID stubTexture;
		};

		Parameter inputParam;

		for (const Mesh& mesh : _scene.meshes()) {
			for (const Primitive& primitive : mesh.primitives) {
				for (int vertBufIdx = 0; vertBufIdx < primitive.vertexBindingCount; vertBufIdx++) {
					inputParam.vertexBuffers.add(renderGraph->importBuffer("Vertex Buffer", primitive.vertexBuffers[vertBufIdx]));
				}
				inputParam.indexBuffers.add(renderGraph->importBuffer("Index Buffer", primitive.indexBuffer));
			}
		}

		inputParam.sceneTextures.reserve(_scene.textures().size());
		for (const Texture& texture : _scene.textures()) {
			inputParam.sceneTextures.add(renderGraph->importTexture("Scene Textures", texture.gpuHandle));
		}

		Soul::GPU::BufferDesc frameUBODesc;
		frameUBODesc.typeSize = sizeof(FrameUBO);
		frameUBODesc.typeAlignment = alignof(FrameUBO);
		frameUBODesc.count = 1;
		frameUBODesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		frameUBODesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		// prepare frame UBO
		FrameUBO frameUBO{};
		// prepare camera
		const CameraInfo& cameraInfo = _scene.getActiveCamera();
		frameUBO.viewFromWorldMatrix = cameraInfo.view;
		frameUBO.worldFromViewMatrix = cameraInfo.model;
		frameUBO.clipFromViewMatrix = cameraInfo.projection;
		frameUBO.viewFromClipMatrix = mat4Inverse(cameraInfo.projection);
		frameUBO.clipFromWorldMatrix = cameraInfo.projection * cameraInfo.view;
		frameUBO.worldFromClipMatrix = cameraInfo.model * mat4Inverse(cameraInfo.projection);
		frameUBO.cameraPosition = cameraInfo.getPosition();
		frameUBO.worldOffset = cameraInfo.worldOffset;
		frameUBO.cameraFar = cameraInfo.zf;
		frameUBO.clipControl = Soul::Vec2f(-0.5f, 0.5f);
		// prepare viewport
		Soul::Vec2ui32 viewport = _scene.getViewport();
		frameUBO.resolution = Soul::Vec4f(viewport.x, viewport.y, 1.0f / viewport.x, 1.0f / viewport.y);
		frameUBO.origin = Soul::Vec2f(0, 0);
		Soul::GPU::BufferID frameGPUBuffer = _gpuSystem->bufferCreate(frameUBODesc, &frameUBO);
		_gpuSystem->bufferDestroy(frameGPUBuffer);
		inputParam.frameUniformBuffer = renderGraph->importBuffer("Frame Uniform Buffer", frameGPUBuffer);

		Soul::GPU::BufferDesc lightUBODesc;
		lightUBODesc.typeSize = sizeof(LightsUBO);
		lightUBODesc.typeAlignment = alignof(LightsUBO);
		lightUBODesc.count = 1;
		lightUBODesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		lightUBODesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		LightsUBO lightsUBO = {};
		Soul::GPU::BufferID lightGPUBuffer = _gpuSystem->bufferCreate(lightUBODesc, &lightsUBO);
		_gpuSystem->bufferDestroy(lightGPUBuffer);
		inputParam.lightUniformBuffer = renderGraph->importBuffer("Light Uniform Buffer", lightGPUBuffer);


		Soul::GPU::BufferDesc froxelRecordsBufferDesc;
		froxelRecordsBufferDesc.typeSize = sizeof(FroxelRecordsUBO);
		froxelRecordsBufferDesc.typeAlignment = alignof(FroxelRecordsUBO);
		froxelRecordsBufferDesc.count = 1;
		froxelRecordsBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		froxelRecordsBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		FroxelRecordsUBO froxelRecordsUBO = {};
		Soul::GPU::BufferID froxelRecordsGPUBuffer = _gpuSystem->bufferCreate(froxelRecordsBufferDesc, &froxelRecordsUBO);
		_gpuSystem->bufferDestroy(froxelRecordsGPUBuffer);
		inputParam.froxelRecordsUniformBuffer = renderGraph->importBuffer("Froxel Records Uniform Buffer", froxelRecordsGPUBuffer);


		Soul::GPU::BufferDesc materialUBODesc;
		materialUBODesc.typeSize = sizeof(MaterialUBO);
		materialUBODesc.typeAlignment = alignof(MaterialUBO);
		materialUBODesc.count = _scene.materials().size();
		materialUBODesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		materialUBODesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;

		Array<MaterialUBO> materialUBOs(&scopeAllocator);
		materialUBOs.resize(materialUBODesc.count);
		for (soul_size materialIdx = 0; materialIdx < _scene.materials().size(); materialIdx++)
		{
			materialUBOs[materialIdx] = _scene.materials()[materialIdx].buffer;
		}

		Soul::GPU::BufferID materialGPUBuffer = _gpuSystem->bufferCreate(materialUBODesc,materialUBOs.data());
		_gpuSystem->bufferDestroy(materialGPUBuffer);
		inputParam.materialUniformBuffer = renderGraph->importBuffer("Material Uniform Buffer", materialGPUBuffer);


		Soul::Array<PerRenderableUBO> renderableUBOs;
		_scene.forEachRenderable([&renderableUBOs](const TransformComponent& transformComp, const RenderComponent& renderComp) -> void {
			PerRenderableUBO renderableUBO;
			renderableUBO.worldFromModelMatrix = transformComp.world;

			Soul::Mat3f m = cofactor(Soul::Mat3f(transformComp.world));
			Soul::Mat3f mTranspose = mat3Transpose(m);
			
			float mFactor = 1.0f / std::sqrt(std::max({ squareLength(mTranspose.rows[0]), squareLength(mTranspose.rows[1]), squareLength(mTranspose.rows[2]) }));

			Mat3f mIdentityFactor;
			mIdentityFactor.elem[0][0] = mFactor;
			mIdentityFactor.elem[1][1] = mFactor;
			mIdentityFactor.elem[2][2] = mFactor;

			m *= mIdentityFactor;

			renderableUBO.worldFromModelNormalMatrix = Soul::GLSLMat3f(m);
			renderableUBO.skinningEnabled = uint32(renderComp.visibility.skinning);
			renderableUBO.morphingEnabled = uint32(renderComp.visibility.morphing);
			renderableUBO.screenSpaceContactShadows = uint32(renderComp.visibility.screenSpaceContactShadows);
			renderableUBO.morphWeights = renderComp.morphWeights;

			renderableUBOs.add(renderableUBO);
			
		});
		Soul::GPU::BufferDesc renderableUBODesc;
		renderableUBODesc.typeSize = sizeof(PerRenderableUBO);
		renderableUBODesc.typeAlignment = alignof(PerRenderableUBO);
		renderableUBODesc.count = renderableUBOs.size();
		renderableUBODesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		renderableUBODesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
			Soul::GPU::BufferID renderableGPUBuffer = _gpuSystem->bufferCreate(renderableUBODesc, renderableUBOs.data());
		_gpuSystem->bufferDestroy(renderableGPUBuffer);
		inputParam.objectUniformBuffer = renderGraph->importBuffer("Renderable Uniform Buffer", renderableGPUBuffer);

		Soul::Array<BonesUBO> bonesUBOs;
		const Array<Skin>& skins = _scene.skins();
		bonesUBOs.resize(skins.size());
		for (uint64 skinIdx = 0; skinIdx < skins.size(); skinIdx++) {
			const Skin& skin = skins[skinIdx];
			std::copy(skin.bones.data(), skin.bones.data() + skin.bones.size(), bonesUBOs[skinIdx].bones);
		}
		if (bonesUBOs.size() == 0) {
			bonesUBOs.add(BonesUBO());
		}
		Soul::GPU::BufferDesc bonesUBODesc;
		bonesUBODesc.typeSize = sizeof(BonesUBO);
		bonesUBODesc.typeAlignment = alignof(BonesUBO);
		bonesUBODesc.count = bonesUBOs.size();
		bonesUBODesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		bonesUBODesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		Soul::GPU::BufferID bonesUBOGPUBuffer = _gpuSystem->bufferCreate(bonesUBODesc, bonesUBOs.data());
		_gpuSystem->bufferDestroy(bonesUBOGPUBuffer);
		inputParam.boneUniformBuffer = renderGraph->importBuffer("Bones Uniform Buffer", bonesUBOGPUBuffer);

		inputParam.renderTarget = finalRenderTarget;

		GPU::RGTextureDesc sceneDepthTexDesc;
		sceneDepthTexDesc.width = sceneResolution.x;
		sceneDepthTexDesc.height = sceneResolution.y;
		sceneDepthTexDesc.depth = 1;
		sceneDepthTexDesc.clear = true;
		sceneDepthTexDesc.clearValue.depthStencil = { 0.0f, 0 };
		sceneDepthTexDesc.format = GPU::TextureFormat::DEPTH32F;
		sceneDepthTexDesc.mipLevels = 1;
		sceneDepthTexDesc.type = GPU::TextureType::D2;
		inputParam.depthTarget = renderGraph->createTexture("Depth target", sceneDepthTexDesc);

		inputParam.stubTexture = renderGraph->importTexture("Stub Texture", _stubTexture);

		Parameter outputParam = renderGraph->addGraphicPass<Parameter>("Test1 pass",
			[this, &inputParam](GPU::GraphicNodeBuilder* builder, Parameter* params) {
				for (GPU::BufferNodeID nodeID : inputParam.vertexBuffers) {
					params->vertexBuffers.add(builder->addVertexBuffer(nodeID));
				}

				for (GPU::BufferNodeID nodeID : inputParam.indexBuffers) {
					params->indexBuffers.add(builder->addIndexBuffer(nodeID));
				}

				for (GPU::TextureNodeID nodeID : inputParam.sceneTextures) {
					params->sceneTextures.add(builder->addShaderTexture(nodeID, GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderTextureReadUsage::UNIFORM));
				}

				params->frameUniformBuffer = builder->addShaderBuffer(inputParam.frameUniformBuffer, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderBufferReadUsage::UNIFORM);
				params->lightUniformBuffer = builder->addShaderBuffer(inputParam.lightUniformBuffer, GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderBufferReadUsage::UNIFORM);
				params->froxelRecordsUniformBuffer = builder->addShaderBuffer(inputParam.froxelRecordsUniformBuffer, GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderBufferReadUsage::UNIFORM);
				params->materialUniformBuffer = builder->addShaderBuffer(inputParam.materialUniformBuffer, GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderBufferReadUsage::UNIFORM);
				params->boneUniformBuffer = builder->addShaderBuffer(inputParam.boneUniformBuffer, GPU::SHADER_STAGE_VERTEX, GPU::ShaderBufferReadUsage::UNIFORM);
				params->objectUniformBuffer = builder->addShaderBuffer(inputParam.objectUniformBuffer, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderBufferReadUsage::UNIFORM);
				params->stubTexture = builder->addShaderTexture(inputParam.stubTexture, GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderTextureReadUsage::UNIFORM);

				GPU::ColorAttachmentDesc colorAttchDesc;
				colorAttchDesc.clear = true;
				colorAttchDesc.clearValue.color.float32 = { 1.0f, 0.0f, 0.0f, 1.0f };
				params->renderTarget = builder->addColorAttachment(inputParam.renderTarget, colorAttchDesc);

				GPU::DepthStencilAttachmentDesc depthAttchDesc;
				depthAttchDesc.clear = true;
				depthAttchDesc.clearValue.depthStencil = { 0.0f, 0 };
				depthAttchDesc.depthWriteEnable = true;
				params->depthTarget = builder->setDepthStencilAttachment(inputParam.depthTarget, depthAttchDesc);

				builder->setRenderTargetDimension(_scene.getViewport());

			},
			[this](GPU::RenderGraphRegistry* registry, const Parameter& params, GPU::RenderCommandBucket* commandBucket) {
				Soul::Vec2ui32 sceneResolution = _scene.getViewport();

				GPU::PipelineStateDesc pipelineDesc;
				pipelineDesc.viewport = { 0, 0, uint16(sceneResolution.x), uint16(sceneResolution.y) };
				pipelineDesc.scissor = { false, 0, 0, uint16(sceneResolution.x), uint16(sceneResolution.y) };
				pipelineDesc.raster.cullMode = GPU::CullMode::NONE;
				pipelineDesc.raster.frontFace = GPU::FrontFace::COUNTER_CLOCKWISE;
				pipelineDesc.colorAttachmentCount = 1;
				pipelineDesc.depthStencilAttachment = { true, true, GPU::CompareOp::GREATER_OR_EQUAL };

				GPU::SamplerDesc samplerDesc = {};
				samplerDesc.minFilter = GPU::TextureFilter::LINEAR;
				samplerDesc.magFilter = GPU::TextureFilter::LINEAR;
				samplerDesc.mipmapFilter = GPU::TextureFilter::LINEAR;
				samplerDesc.wrapU = GPU::TextureWrap::REPEAT;
				samplerDesc.wrapV = GPU::TextureWrap::REPEAT;
				samplerDesc.wrapW = GPU::TextureWrap::REPEAT;
				samplerDesc.anisotropyEnable = false;
				samplerDesc.maxAnisotropy = 0.0f;
				GPU::SamplerID samplerID = _gpuSystem->samplerRequest(samplerDesc);

				GPU::Descriptor set0Descriptors[] = {
					GPU::Descriptor::Uniform(registry->getBuffer(params.frameUniformBuffer), 0, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT),
					GPU::Descriptor::Uniform(registry->getBuffer(params.lightUniformBuffer), 0, GPU::SHADER_STAGE_FRAGMENT),
					{},
					GPU::Descriptor::Uniform(registry->getBuffer(params.froxelRecordsUniformBuffer), 0, GPU::SHADER_STAGE_FRAGMENT),
					GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_FRAGMENT),
					GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_FRAGMENT),
					GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_FRAGMENT),
					GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_FRAGMENT),
					GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_FRAGMENT),
					GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_FRAGMENT),
					GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_FRAGMENT),
				};
				GPU::ShaderArgSetID set0 = registry->getShaderArgSet(0, { sizeof(set0Descriptors) / sizeof(set0Descriptors[0]), set0Descriptors });

				uint32 renderableID = 0;

				_scene.forEachRenderable([&](const TransformComponent& transform, const RenderComponent& renderComp) {
					using DrawCommand = GPU::RenderCommandDrawPrimitive;
					DrawCommand command;
					const Mesh& mesh = _scene.meshes()[renderComp.meshID.id];
					for (const Primitive& primitive : mesh.primitives) {
						const Material& material = _scene.materials()[primitive.materialID.id];
						
						GPUProgramVariant matVariant;
						matVariant.setDirectionalLighting(true);
						matVariant.setSkinning(renderComp.visibility.skinning || renderComp.visibility.morphing);
						pipelineDesc.programID = _programRegistry.getProgram(material.programSetID, matVariant);

						GPU::PipelineStateDesc::InputBindingDesc inputBindings[GPU::MAX_INPUT_BINDING_PER_SHADER];
						GPU::PipelineStateDesc::InputAttrDesc inputAttributes[GPU::MAX_INPUT_PER_SHADER];

						for (uint32 attribIdx = 0; attribIdx < VertexAttribute::COUNT; attribIdx++) {
							Attribute attribute = primitive.attributes[attribIdx];
							GPU::VertexElementFlags elemFlags = attribute.elementFlags;
							GPU::VertexElementType elemType = attribute.elementType;
							if (attribute.buffer == Attribute::BUFFER_UNUSED) {
								attribute = primitive.attributes[0];
								if (attribIdx == VertexAttribute::BONE_INDICES) {
									elemFlags = GPU::VERTEX_ELEMENT_INTEGER_TARGET;
									elemType = GPU::VertexElementType::UBYTE4;
								}
								else {
									elemFlags = GPU::VERTEX_ELEMENT_NORMALIZED;
									elemType = GPU::VertexElementType::BYTE4;
								}
							}

							inputBindings[attribIdx] = { attribute.stride };
							inputAttributes[attribIdx] = {
								attribIdx,
								attribute.offset,
								elemType,
								elemFlags
							};
						}
						
						memcpy(pipelineDesc.inputBindings, inputBindings, sizeof(inputBindings));
						memcpy(pipelineDesc.inputAttributes, inputAttributes, sizeof(inputAttributes));

						GPU::Descriptor set1Descriptors[] = {
							GPU::Descriptor::Uniform(registry->getBuffer(params.materialUniformBuffer), primitive.materialID.id, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT)
						};
						GPU::ShaderArgSetID set1 = registry->getShaderArgSet(1, { sizeof(set1Descriptors) / sizeof(set1Descriptors[0]), set1Descriptors });

						GPU::TextureID stubTexture = registry->getTexture(params.stubTexture);
						GPU::Descriptor set2Descriptors[] = {
							GPU::Descriptor::SampledImage(material.textures.baseColorTexture.isNull() ? stubTexture : _scene.textures()[material.textures.baseColorTexture.id].gpuHandle, samplerID, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT),
							GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT),
							GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT),
							GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT),
							GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT),
							GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT),
							GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT),
							GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT),
							GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT),
							GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT),
							GPU::Descriptor::SampledImage(registry->getTexture(params.stubTexture), samplerID, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT)
						};
						GPU::ShaderArgSetID set2 = registry->getShaderArgSet(2, { sizeof(set2Descriptors) / sizeof(set2Descriptors[0]), set2Descriptors });

						Soul::Array<GPU::Descriptor> set3Descriptors;
						set3Descriptors.reserve(GPU::MAX_BINDING_PER_SET);
						set3Descriptors.add(GPU::Descriptor::Uniform(registry->getBuffer(params.objectUniformBuffer), renderableID, GPU::SHADER_STAGE_FRAGMENT | GPU::SHADER_STAGE_VERTEX));
						if (renderComp.visibility.skinning || renderComp.visibility.morphing) {
							uint32 skinIndex = renderComp.skinID.isNull() ? 0 : renderComp.skinID.id;
							set3Descriptors.add(GPU::Descriptor::Uniform(registry->getBuffer(params.boneUniformBuffer), skinIndex, GPU::SHADER_STAGE_VERTEX));
						}
						
						GPU::ShaderArgSetID set3 = registry->getShaderArgSet(3, { uint32(set3Descriptors.size()), set3Descriptors.data() });

						DrawCommand* command = commandBucket->add<DrawCommand>(renderableID);
						command->pipelineStateID = registry->getPipelineState(pipelineDesc);
						for (uint32 attribIdx = 0; attribIdx < VertexAttribute::COUNT; attribIdx++) {
							Attribute attribute = primitive.attributes[attribIdx];
							if (attribute.buffer == Attribute::BUFFER_UNUSED) {
								attribute = primitive.attributes[0];
							}
							command->vertexBufferIDs[attribIdx] = primitive.vertexBuffers[attribute.buffer];
						}
						command->indexBufferID = primitive.indexBuffer;
						command->shaderArgSetIDs[0] = set0;
						command->shaderArgSetIDs[1] = set1;
						command->shaderArgSetIDs[2] = set2;
						command->shaderArgSetIDs[3] = set3;

					}

					renderableID++;
				});

			});
		

		return outputParam.renderTarget;
	}
}