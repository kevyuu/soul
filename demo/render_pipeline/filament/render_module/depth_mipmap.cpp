#include "depth_mipmap.h"
#include "runtime/scope_allocator.h"
#include "../../../utils.h"

namespace SoulFila
{
	static constexpr char const* const DEPTH_MIPMAP_VERT_GLSL = "shaders/filament/depth_mipmap.vert.glsl";
	static constexpr char const* const DEPTH_MIPMAP_FRAG_GLSL = "shaders/filament/depth_mipmap.frag.glsl";

	void DepthMipmapPass::init(soul::gpu::System* gpuSystemIn)
	{
		this->gpuSystem = gpuSystemIn;
		soul::runtime::ScopeAllocator<> allocator("Depth Mipmap Code Text");
		char* vert_code = LoadFile(DEPTH_MIPMAP_VERT_GLSL, &allocator);
		char* frag_code = LoadFile(DEPTH_MIPMAP_FRAG_GLSL, &allocator);

		soul::gpu::ProgramDesc programDesc;
		programDesc.shaderIDs[gpu::ShaderStage::VERTEX] = gpuSystem->create_shader({ "depth_mipmap_pass", vert_code, soul::cast<uint32>(strlen(vert_code)) }, gpu::ShaderStage::VERTEX);
		programDesc.shaderIDs[gpu::ShaderStage::FRAGMENT] = gpuSystem->create_shader({ "depth_mipmap_fragment", frag_code, soul::cast<uint32>(strlen(frag_code)) }, gpu::ShaderStage::FRAGMENT);
		this->programID = gpuSystem->request_program(programDesc);
	}

	DepthMipmapPass::Output DepthMipmapPass::computeRenderGraph(soul::gpu::RenderGraph& render_graph, const Input& input, const Scene& scene)
	{

		const gpu::RGTextureDesc depth_tex_desc = render_graph.get_texture_desc(input.depthMap, *gpuSystem);
		// We limit the lowest lod size to 32 pixels (which is where the -5 comes from)
		const uint32 level_count = depth_tex_desc.mipLevels;
		SOUL_ASSERT(0, level_count >= 1, "");

		struct MipmapParameter
		{
			gpu::TextureNodeID depthTexture;
		};

		const gpu::SamplerDesc sampler_desc = gpu::SamplerDesc::SameFilterWrap(gpu::TextureFilter::NEAREST, gpu::TextureWrap::REPEAT);
		const gpu::SamplerID sampler_id = gpuSystem->request_sampler(sampler_desc);

		gpu::TextureNodeID depth_mipmap = input.depthMap;
		Vec2ui32 dimension = depth_tex_desc.extent.xy;
		for (uint8 target_level = 1; target_level < level_count; target_level++)
		{
			uint8 source_level = target_level - 1;
			dimension = dimension / 2u;
			const gpu::DepthStencilAttachmentDesc depth_attachment_desc = {
				.nodeID = depth_mipmap,
				.view = gpu::SubresourceIndex(target_level, 0),
				.depthWriteEnable = true,
				.clear = false
			};
			depth_mipmap = render_graph.add_graphic_pass<MipmapParameter>("Depth Mipmap Pass", gpu::RGRenderTargetDesc(dimension, depth_attachment_desc),
				[depth_mipmap, target_level](gpu::RGGraphicPassDependencyBuilder& builder, MipmapParameter& params)
				{
					params.depthTexture = builder.add_shader_texture(depth_mipmap, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }, gpu::ShaderTextureReadUsage::UNIFORM, gpu::SubresourceIndexRange(gpu::SubresourceIndex(), target_level, 1));
				},
				[program_id = this->programID, &scene, sampler_id, source_level, dimension](const MipmapParameter& params, gpu::RenderGraphRegistry& registry, gpu::GraphicCommandList& command_list)
				{
					const gpu::Descriptor set0_descriptors[] = {
						gpu::Descriptor::SampledImage(registry.get_texture(params.depthTexture), sampler_id, {gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT}, gpu::SubresourceIndex(source_level, 0))
					};
					const gpu::ShaderArgSetID set0 = registry.get_shader_arg_set(0, { std::size(set0_descriptors), set0_descriptors });

					using DrawCommand = gpu::RenderCommandDrawPrimitive;
					const gpu::GraphicPipelineStateDesc pipeline_desc = {
						.programID = program_id,
						.inputBindings = {
							{.stride = sizeof(Vec2f)}
						},
						.inputAttributes = {
							{.binding = 0, .offset = 0, .type = gpu::VertexElementType::FLOAT2}
						},
						.viewport = {0, 0, soul::cast<uint16>(dimension.x), soul::cast<uint16>(dimension.y)},
						.scissor = { false, 0, 0, soul::cast<uint16>(dimension.x), soul::cast<uint16>(dimension.y)},
						.raster = {
							.cullMode = gpu::CullMode::NONE
						},
						.depthStencilAttachment = {true, true, gpu::CompareOp::ALWAYS},
					};
					const DrawCommand draw_command = {
						.pipelineStateID =  registry.get_pipeline_state(pipeline_desc),
						.shaderArgSetIDs = {set0},
						.vertexBufferIDs = {scene.getFullScreenVertexBuffer()},
						.indexBufferID = scene.getFullScreenIndexBuffer()
					};
					command_list.push(draw_command);
				}).get_render_target().depthStencilAttachment.outNodeID;
		}
		
		return { depth_mipmap };
	}

}