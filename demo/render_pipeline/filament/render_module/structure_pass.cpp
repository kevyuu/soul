#include "structure_pass.h"
#include "runtime/scope_allocator.h"

#include <variant>

#include "../../../utils.h"

#include "draw_item.h"

namespace soul_fila
{

	static constexpr float SCALE = 0.5f;
	StructurePass::Output StructurePass::computeRenderGraph(soul::gpu::RenderGraph& renderGraph, const Input& input, const RenderData& renderData, const Scene& scene)
	{
		struct Parameter
		{
			soul::gpu::BufferNodeID frameUBO;
			soul::gpu::BufferNodeID objectsUBO;
			soul::gpu::BufferNodeID bonesUBO;
			soul::gpu::BufferNodeID materialsUBO;
		};

		Parameter inputParam;
		inputParam.frameUBO = input.frameUb;
		inputParam.objectsUBO = input.objectsUb;
		inputParam.bonesUBO = input.bonesUb;
		inputParam.materialsUBO = input.materialsUb;

		Vec2ui32 scene_resolution = scene.get_viewport();
		uint32 width = std::max(32u, (uint32_t)std::ceil(scene_resolution.x * SCALE));
		uint32 height = std::max(32u, (uint32_t)std::ceil(scene_resolution.y * SCALE));
		const uint32 level_count = std::min(8, MaxLevelCount(width, width) - 5);
		SOUL_ASSERT(0, level_count >= 1, "");

		const auto depth_target_desc = gpu::RGTextureDesc::create_d2(gpu::TextureFormat::DEPTH32F, level_count, Vec2ui32(width, height), true);
		const gpu::DepthStencilAttachmentDesc depth_stencil_attachment_desc = {
			.nodeID = renderGraph.create_texture("Structure Texture", depth_target_desc),
			.view = gpu::SubresourceIndex(0, 0),
			.depthWriteEnable = true,
			.clear = true
		};

		const auto& node = renderGraph.add_graphic_pass<Parameter>("Structure Pass",
			gpu::RGRenderTargetDesc(
				Vec2ui32(width, height),
				depth_stencil_attachment_desc
			),
			[this, &inputParam](gpu::RGShaderPassDependencyBuilder& builder, Parameter& params)
			{
				params.frameUBO = builder.add_shader_buffer(inputParam.frameUBO, gpu::ShaderStageFlags({ gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }), gpu::ShaderBufferReadUsage::UNIFORM);
				params.materialsUBO = builder.add_shader_buffer(inputParam.materialsUBO, gpu::ShaderStageFlags({ gpu::ShaderStage::FRAGMENT }), gpu::ShaderBufferReadUsage::UNIFORM);
				params.bonesUBO = builder.add_shader_buffer(inputParam.bonesUBO, gpu::ShaderStageFlags({ gpu::ShaderStage::VERTEX }), gpu::ShaderBufferReadUsage::UNIFORM);
				params.objectsUBO = builder.add_shader_buffer(inputParam.objectsUBO, gpu::ShaderStageFlags({ gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }), gpu::ShaderBufferReadUsage::UNIFORM);
			},
			[&scene, this, &renderData, viewport = Vec2ui32(width, height)]( const Parameter& params, gpu::RenderGraphRegistry& registry, gpu::GraphicCommandList& command_list)
			{
				const CameraInfo& cameraInfo = renderData.cameraInfo;
				Vec3f cameraPosition = cameraInfo.get_position();
				Vec3f cameraForward = cameraInfo.get_forward_vector();

				const auto& renderables = renderData.renderables;
				auto const* const SOUL_RESTRICT soa_world_aabb_center = renderables.data<RenderablesIdx::WORLD_AABB_CENTER>();
				auto const* const SOUL_RESTRICT soa_reversed_winding = renderables.data<RenderablesIdx::REVERSED_WINDING_ORDER>();
				auto const* const SOUL_RESTRICT soa_visibility = renderables.data<RenderablesIdx::VISIBILITY_STATE>();
				auto const* const SOUL_RESTRICT soa_primitives = renderables.data<RenderablesIdx::PRIMITIVES>();
				auto const* const SOUL_RESTRICT soa_primitive_count = renderables.data<RenderablesIdx::SUMMED_PRIMITIVE_COUNT>();

				const auto visibleRenderables = renderData.visibleRenderables;
				Array<DrawItem> draw_items;
				uint32 drawItemCount = soa_primitive_count[visibleRenderables.last] - soa_primitive_count[visibleRenderables.first];
				draw_items.resize(drawItemCount);

				for (soul_size renderableIdx : visibleRenderables)
				{
					float distance = dot(soa_world_aabb_center[renderableIdx], cameraForward) - dot(cameraPosition, cameraForward);
					distance = -distance;
					const uint32_t distanceBits = reinterpret_cast<uint32_t&>(distance);
					auto variant = GPUProgramVariant(GPUProgramVariant::DEPTH_VARIANT);
					variant.setSkinning(soa_visibility[renderableIdx].skinning || soa_visibility[renderableIdx].morphing);
					// calculate the per-primitive face winding order inversion
					const bool inverseFrontFaces = soa_reversed_winding[renderableIdx];

					DrawItem item;
					item.key = to_underlying(Pass::DEPTH);
					item.key |= to_underlying(CustomCommand::PASS);
					item.key |= MakeField(soa_visibility[renderableIdx].priority, PRIORITY_MASK, PRIORITY_SHIFT);
					item.key |= MakeField(distanceBits, DISTANCE_BITS_MASK, DISTANCE_BITS_SHIFT);
					item.index = soul::cast<uint32>(renderableIdx);
					item.rasterState = {};
					item.rasterState.colorWrite = false;
					item.rasterState.depthWrite = true;
					item.rasterState.depthFunc = RasterState::DepthFunc::GREATER_OR_EQUAL;
					item.rasterState.inverseFrontFaces = inverseFrontFaces;

					soul_size offset = soa_primitive_count[renderableIdx] - soa_primitive_count[renderData.visibleRenderables.first];
					DrawItem* curr = &draw_items[offset];

					for (const Primitive& primitive : *soa_primitives[renderableIdx])
					{
						const Material& material = scene.materials()[primitive.materialID.id];
						const ProgramSetInfo& programSetInfo = programRegistry->getProgramSetInfo(material.programSetID);

						BlendingMode blendingMode = programSetInfo.blendingMode;
						bool translucent = (blendingMode != BlendingMode::OPAQUE && blendingMode != BlendingMode::MASKED);

						*curr = item;
						curr->primitive = &primitive;
						curr->material = &material;

						curr->key |= Select(translucent);
						curr->programID = programRegistry->getProgram(material.programSetID, variant);
						++curr;
					}
				}

				std::sort(draw_items.begin(), draw_items.end());
				DrawItem const* const last = std::partition_point(draw_items.begin(), draw_items.end(),
					[](DrawItem const& c) {
						return c.key != to_underlying(Pass::SENTINEL);
					});
				draw_items.resize(last - draw_items.begin());


				gpu::GraphicPipelineStateDesc pipeline_base_desc = {
					.viewport = { 0, 0, soul::cast<uint16>(viewport.x), soul::cast<uint16>(viewport.y) },
					.scissor = { false, 0, 0, soul::cast<uint16>(viewport.x), soul::cast<uint16>(viewport.y) },
					.colorAttachmentCount = 1,
					.depthStencilAttachment = { true, true, gpu::CompareOp::GREATER_OR_EQUAL}
				};
				
				const gpu::SamplerDesc sampler_desc = gpu::SamplerDesc::SameFilterWrap(gpu::TextureFilter::LINEAR, gpu::TextureWrap::REPEAT);
				const gpu::SamplerID sampler_id = gpuSystem->request_sampler(sampler_desc);
				const gpu::Descriptor set0_descriptors[] = {
					gpu::Descriptor::Uniform(registry.get_buffer(params.frameUBO), 0, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }),
				};
				const gpu::ShaderArgSetID set0 = registry.get_shader_arg_set(0, { std::size(set0_descriptors), set0_descriptors });

				auto get_material_gpu_texture = [&scene, stubTexture = renderData.stubTexture](TextureID sceneTextureID) -> soul::gpu::TextureID
				{
					return sceneTextureID.is_null() ? stubTexture : scene.textures()[sceneTextureID.id].gpuHandle;
				};

				using DrawCommand = gpu::RenderCommandDrawPrimitive;
				command_list.push<DrawCommand>(draw_items.size(), [&, sampler_id, set0](soul_size command_idx)
				{
					runtime::ScopeAllocator scope_allocator("Structure Pass Draw Command");
					const DrawItem& drawItem = draw_items[command_idx];
					const Primitive& primitive = *drawItem.primitive;
					const Material& material = *drawItem.material;
					gpu::GraphicPipelineStateDesc pipeline_desc = pipeline_base_desc;
					DrawItem::ToPipelineStateDesc(drawItem, pipeline_desc);

					const gpu::Descriptor set1Descriptors[] = {
							gpu::Descriptor::Uniform(registry.get_buffer(params.materialsUBO), soul::cast<uint32>(primitive.materialID.id), { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT })
					};
					const gpu::ShaderArgSetID set1 = registry.get_shader_arg_set(1, { std::size(set1Descriptors), set1Descriptors });

					const gpu::Descriptor set2_descriptors[] = {
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.baseColorTexture), sampler_id, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.metallicRoughnessTexture), sampler_id, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.normalTexture), sampler_id, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.occlusionTexture), sampler_id, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.emissiveTexture), sampler_id, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.clearCoatTexture), sampler_id, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.clearCoatRoughnessTexture), sampler_id, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.clearCoatNormalTexture), sampler_id, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.sheenColorTexture), sampler_id, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.sheenRoughnessTexture), sampler_id, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.transmissionTexture), sampler_id, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.volumeThicknessTexture), sampler_id, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT })
					};
					const gpu::ShaderArgSetID set2 = registry.get_shader_arg_set(2, { std::size(set2_descriptors), set2_descriptors });

					soul::Array<gpu::Descriptor> set3Descriptors(&scope_allocator);
					set3Descriptors.reserve(gpu::MAX_BINDING_PER_SET);
					set3Descriptors.push_back(gpu::Descriptor::Uniform(registry.get_buffer(params.objectsUBO), drawItem.index, { gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT }));

					const SkinID skin_id = renderables.elementAt<RenderablesIdx::SKIN_ID>(drawItem.index);
					const Visibility visibility = renderables.elementAt<RenderablesIdx::VISIBILITY_STATE>(drawItem.index);
					if (visibility.skinning || visibility.morphing) {
						uint32 skin_index = skin_id.is_null() ? 0 : soul::cast<uint32>(skin_id.id);
						set3Descriptors.push_back(gpu::Descriptor::Uniform(registry.get_buffer(params.bonesUBO), skin_index, { gpu::ShaderStage::VERTEX }));
					}

					const gpu::ShaderArgSetID set3 = registry.get_shader_arg_set(3, { uint32(set3Descriptors.size()), set3Descriptors.data() });

					DrawCommand draw_command = {
						.pipelineStateID = registry.get_pipeline_state(pipeline_desc),
						.shaderArgSetIDs = {set0, set1, set2, set3},
						.indexBufferID = primitive.indexBuffer
					};
					for (uint32 attrib_idx = 0; attrib_idx < to_underlying(VertexAttribute::COUNT); attrib_idx++) {
						Attribute attribute = primitive.attributes[attrib_idx];
						if (attribute.buffer == Attribute::BUFFER_UNUSED) {
							attribute = primitive.attributes[0];
						}
						draw_command.vertexBufferIDs[attrib_idx] = primitive.vertexBuffers[attribute.buffer];
					}
					return draw_command;
					
				});

			}
		);

		return { node.get_render_target().depthStencilAttachment.outNodeID };
	}
}