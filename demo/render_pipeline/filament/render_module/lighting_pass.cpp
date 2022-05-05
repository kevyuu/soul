#include "lighting_pass.h"
#include "../data.h"
#include "draw_item.h"

#include <set>
#include <iostream>

namespace soul_fila
{
    void setup_key(const ProgramSetInfo& program_set_info, 
        const MaterialID material_id, const GPUProgramVariant base_variant, const GPUProgramSetID program_set_id,
        GPUProgramRegistry& program_registry, DrawItem& drawItem)
    {
        GPUProgramVariant variant;
        variant.key = GPUProgramVariant::filterVariant(base_variant.key, program_set_info.isLit);

        uint64_t key_blending = drawItem.key;
        key_blending &= ~(PASS_MASK | BLENDING_MASK);
        key_blending |= to_underlying(Pass::BLENDED);
        key_blending |= to_underlying(CustomCommand::PASS);

        BlendingMode blending_mode = program_set_info.blendingMode;
        bool has_screen_space_refraction = program_set_info.refractionMode == RefractionMode::SCREEN_SPACE;
        bool is_blending_command = !has_screen_space_refraction &&
            (blending_mode != BlendingMode::OPAQUE && blending_mode != BlendingMode::MASKED);

        uint64_t key_draw = drawItem.key;
        key_draw &= ~(PASS_MASK | BLENDING_MASK | MATERIAL_MASK);
        key_draw |= to_underlying(has_screen_space_refraction ? Pass::REFRACT : Pass::COLOR);
        key_draw |= to_underlying(CustomCommand::PASS);
        key_draw |= MakeMaterialSortingKey(soul::cast<uint32>(material_id.id), variant);
        key_draw |= MakeField(program_set_info.blendingMode == BlendingMode::MASKED, BLENDING_MASK, BLENDING_SHIFT);

        drawItem.key = is_blending_command ? key_blending : key_draw;
        drawItem.programID = program_registry.getProgram(program_set_id, base_variant);
    }

    LightingPass::Output LightingPass::computeRenderGraph(soul::gpu::RenderGraph& renderGraph, const Input& input, const RenderData& renderData, const Scene& scene)
    {

        struct Parameter {
            soul::gpu::BufferNodeID frameUniformBuffer;
            soul::gpu::BufferNodeID lightUniformBuffer;
            soul::gpu::BufferNodeID shadowUniformBuffer;
            soul::gpu::BufferNodeID froxelRecordsUniformBuffer;
            soul::gpu::BufferNodeID objectUniformBuffer;
            soul::gpu::BufferNodeID boneUniformBuffer;
            soul::gpu::BufferNodeID materialUniformBuffer;
            soul::gpu::TextureNodeID structureTex;
            soul::gpu::TextureNodeID shadowMap;
        };

        Parameter input_param = {
            .frameUniformBuffer = input.frameUb,
            .lightUniformBuffer = input.lightsUb,
            .shadowUniformBuffer = input.shadowUb,
            .froxelRecordsUniformBuffer = input.froxelRecrodUb,
            .objectUniformBuffer = input.objectsUb,
            .boneUniformBuffer = input.bonesUb,
            .materialUniformBuffer = input.materialsUb,
            .structureTex = input.structureTex,
            .shadowMap = input.shadowMap
        };
        
        Vec2ui32 scene_resolution = scene.get_viewport();

        Range<uint32> visible_renderables = renderData.visibleRenderables;
        
        const Renderables& renderables = renderData.renderables;

        auto const* const SOUL_RESTRICT soaWorldAABBCenter = renderables.data<RenderablesIdx::WORLD_AABB_CENTER>();
        auto const* const SOUL_RESTRICT soaReversedWinding = renderables.data<RenderablesIdx::REVERSED_WINDING_ORDER>();
        auto const* const SOUL_RESTRICT soaVisibility = renderables.data<RenderablesIdx::VISIBILITY_STATE>();
        auto const* const SOUL_RESTRICT soaPrimitives = renderables.data<RenderablesIdx::PRIMITIVES>();
        auto const* const SOUL_RESTRICT soaVisibilityMask = renderables.data<RenderablesIdx::VISIBLE_MASK>();
        auto const* const SOUL_RESTRICT soaPrimitiveCount = renderables.data<RenderablesIdx::SUMMED_PRIMITIVE_COUNT>();
        GPUProgramVariant base_variant;
        base_variant.setDirectionalLighting(renderData.flags & HAS_DIRECTIONAL_LIGHT);
        base_variant.setDynamicLighting(renderData.flags & HAS_DYNAMIC_LIGHTING);
        base_variant.setFog(renderData.flags & HAS_FOG);
        base_variant.setVsm((renderData.flags & HAS_VSM) && (renderData.flags & HAS_SHADOWING));

        uint32 draw_item_count = soaPrimitiveCount[visible_renderables.last] - soaPrimitiveCount[visible_renderables.first];
        draw_item_count *= 2;
        SOUL_ASSERT(0, draw_item_count != 0, "");
        drawItems.resize(draw_item_count);

    	const CameraInfo& camera_info = renderData.cameraInfo;
        Vec3f camera_position = camera_info.get_position();
        Vec3f camera_forward = camera_info.get_forward_vector();

        std::set<MaterialID> blended_materials;

        for (soul_size renderable_idx : visible_renderables)
        {
            soul_size offset = soaPrimitiveCount[renderable_idx] - soaPrimitiveCount[visible_renderables.first];
            offset *= 2;
        	DrawItem* curr = &drawItems[offset];
            const Array<Primitive>& primitives = *soaPrimitives[renderable_idx];

            if (SOUL_UNLIKELY(!(soaVisibilityMask[renderable_idx] & VISIBLE_RENDERABLE)))
            {
                for (soul_size offsetIdx = 0; offsetIdx < primitives.size(); offsetIdx++)
                {
                    curr->key = to_underlying(Pass::SENTINEL);
                    curr++;
                }
                continue;
            }

            // Signed distance from camera to object's center. Positive distances are in front of
            // the camera. Some objects with a center behind the camera can still be visible
            // so their distance will be negative (this happens a lot for the shadow map).

            // Using the center is not very good with large AABBs. Instead we can try to use
            // the closest point on the bounding sphere instead:
            //      d = soaWorldAABBCenter[i] - cameraPosition;
            //      d -= normalize(d) * length(soaWorldAABB[i].halfExtent);
            // However this doesn't work well at all for large planes.

            // Code below is equivalent to:
            // float3 d = soaWorldAABBCenter[i] - cameraPosition;
            // float distance = dot(d, cameraForward);
            // but saves a couple of instruction, because part of the math is done outside of the loop.
            float distance = dot(soaWorldAABBCenter[renderable_idx], camera_forward) - dot(camera_position, camera_forward);

            // We negate the distance to the camera in order to create a bit pattern that will
            // be sorted properly, this works because:
            // - positive distances (now negative), will still be sorted by their absolute value
            //   due to float representation.
            // - negative distances (now positive) will be sorted BEFORE everything else, and we
            //   don't care too much about their order (i.e. should objects far behind the camera
            //   be sorted first? -- unclear, and probably irrelevant).
            //   Here, objects close to the camera (but behind) will be drawn first.
            // An alternative that keeps the mathematical ordering is given here:
            //   distanceBits ^= ((int32_t(distanceBits) >> 31) | 0x80000000u);
            distance = -distance;
            const uint32_t distanceBits = reinterpret_cast<uint32_t&>(distance);

            // calculate the per-primitive face winding order inversion
            const bool inverseFrontFaces = soaReversedWinding[renderable_idx];
            GPUProgramVariant variant = base_variant;
            variant.setShadowReceiver(soaVisibility[renderable_idx].receiveShadows && (renderData.flags & HAS_SHADOWING));
            variant.setSkinning(soaVisibility[renderable_idx].skinning || soaVisibility[renderable_idx].morphing);
            
            for (const Primitive& primitive : primitives)
            {
                const Material& material = scene.materials()[primitive.materialID.id];
                const ProgramSetInfo& programSetInfo = programRegistry->getProgramSetInfo(material.programSetID);

                DrawItem draw_item;
                draw_item.key = MakeField(soaVisibility[renderable_idx].priority, PRIORITY_MASK, PRIORITY_SHIFT);
            	setup_key(programSetInfo, primitive.materialID, variant, material.programSetID, *programRegistry, draw_item);
            	draw_item.index = soul::cast<uint16>(renderable_idx);
                draw_item.primitive = &primitive;
                draw_item.material = &material;
                SOUL_ASSERT(0, material.cullMode != gpu::CullMode::COUNT, "");

                using BlendFunction = RasterState::BlendFunction;
                using DepthFunc = RasterState::DepthFunc;
                switch (programSetInfo.blendingMode) {
                case BlendingMode::OPAQUE:
                    draw_item.rasterState.blendFunctionSrcRGB = BlendFunction::ONE;
                    draw_item.rasterState.blendFunctionSrcAlpha = BlendFunction::ONE;
                    draw_item.rasterState.blendFunctionDstRGB = BlendFunction::ZERO;
                    draw_item.rasterState.blendFunctionDstAlpha = BlendFunction::ZERO;
                    draw_item.rasterState.depthWrite = true;
                    break;
                case BlendingMode::MASKED:
                    draw_item.rasterState.blendFunctionSrcRGB = BlendFunction::ONE;
                    draw_item.rasterState.blendFunctionSrcAlpha = BlendFunction::ZERO;
                    draw_item.rasterState.blendFunctionDstRGB = BlendFunction::ZERO;
                    draw_item.rasterState.blendFunctionDstAlpha = BlendFunction::ONE;
                    draw_item.rasterState.depthWrite = true;
                    break;
                case BlendingMode::TRANSPARENT:
                case BlendingMode::FADE:
                    draw_item.rasterState.blendFunctionSrcRGB = BlendFunction::ONE;
                    draw_item.rasterState.blendFunctionSrcAlpha = BlendFunction::ONE;
                    draw_item.rasterState.blendFunctionDstRGB = BlendFunction::ONE_MINUS_SRC_ALPHA;
                    draw_item.rasterState.blendFunctionDstAlpha = BlendFunction::ONE_MINUS_SRC_ALPHA;
                    draw_item.rasterState.depthWrite = false;
                    break;
                case BlendingMode::ADD:
                    draw_item.rasterState.blendFunctionSrcRGB = BlendFunction::ONE;
                    draw_item.rasterState.blendFunctionSrcAlpha = BlendFunction::ONE;
                    draw_item.rasterState.blendFunctionDstRGB = BlendFunction::ONE;
                    draw_item.rasterState.blendFunctionDstAlpha = BlendFunction::ONE;
                    draw_item.rasterState.depthWrite = false;
                    break;
                case BlendingMode::MULTIPLY:
                    draw_item.rasterState.blendFunctionSrcRGB = BlendFunction::ZERO;
                    draw_item.rasterState.blendFunctionSrcAlpha = BlendFunction::ZERO;
                    draw_item.rasterState.blendFunctionDstRGB = BlendFunction::SRC_COLOR;
                    draw_item.rasterState.blendFunctionDstAlpha = BlendFunction::SRC_COLOR;
                    draw_item.rasterState.depthWrite = false;
                    break;
                case BlendingMode::SCREEN:
                    draw_item.rasterState.blendFunctionSrcRGB = BlendFunction::ONE;
                    draw_item.rasterState.blendFunctionSrcAlpha = BlendFunction::ONE;
                    draw_item.rasterState.blendFunctionDstRGB = BlendFunction::ONE_MINUS_SRC_COLOR;
                    draw_item.rasterState.blendFunctionDstAlpha = BlendFunction::ONE_MINUS_SRC_COLOR;
                    draw_item.rasterState.depthWrite = false;
                    break;
                }
                draw_item.rasterState.culling = material.cullMode;
                draw_item.rasterState.inverseFrontFaces = inverseFrontFaces;
                draw_item.rasterState.colorWrite = true;
                draw_item.rasterState.depthFunc = gpu::CompareOp::GREATER_OR_EQUAL;

                if (Pass(draw_item.key & PASS_MASK) == Pass::BLENDED)
                {
                    // TODO: at least for transparent objects, AABB should be per primitive
                    // blend pass:
                    // this will sort back-to-front for blended, and honor explicit ordering
                    // for a given Z value
                    draw_item.key &= ~BLEND_ORDER_MASK;
                    draw_item.key &= ~BLEND_DISTANCE_MASK;
                    draw_item.key |= MakeField(~distanceBits,
                        BLEND_DISTANCE_MASK, BLEND_DISTANCE_SHIFT);
                    draw_item.key |= MakeField(0,
                        BLEND_ORDER_MASK, BLEND_ORDER_SHIFT); // TODO: cutomizable primitive's blend order

                    const TransparencyMode mode = material.transparencyMode;

                    // handle transparent objects, two techniques:
                    //
                    //   - TWO_PASSES_ONE_SIDE: draw the front faces in the depth buffer then
                    //     front faces with depth test in the color buffer.
                    //     In this mode we actually do not change the user's culling mode
                    //
                    //   - TWO_PASSES_TWO_SIDES: draw back faces first,
                    //     then front faces, both in the color buffer.
                    //     In this mode, we override the user's culling mode.

                    // TWO_PASSES_TWO_SIDES: this command will be issued 2nd, draw front faces
                    draw_item.rasterState.culling =
                        (mode == TransparencyMode::TWO_PASSES_TWO_SIDES) ?
						gpu::CullMode::BACK : draw_item.rasterState.culling;

                    uint64_t key = draw_item.key;

                    // draw this command AFTER THE NEXT ONE
                    key |= MakeField(1, BLEND_TWO_PASS_MASK, BLEND_TWO_PASS_SHIFT);
                    
                    // correct for TransparencyMode::DEFAULT -- i.e. cancel the command
                    key |= Select(mode == TransparencyMode::DEFAULT);

                    *curr = draw_item;
                    curr->key = key;
                    ++curr;

                    // TWO_PASSES_TWO_SIDES: this command will be issued first, draw back sides (i.e. cull front)
                    draw_item.rasterState.culling =
                        (mode == TransparencyMode::TWO_PASSES_TWO_SIDES) ?
                        gpu::CullMode::FRONT : draw_item.rasterState.culling;

                    // TWO_PASSES_ONE_SIDE: this command will be issued first, draw (back side) in depth buffer only
                    draw_item.rasterState.depthWrite |= Select(mode == TransparencyMode::TWO_PASSES_ONE_SIDE);
                    draw_item.rasterState.colorWrite &= ~Select(mode == TransparencyMode::TWO_PASSES_ONE_SIDE);
                    draw_item.rasterState.depthFunc =
                        (mode == TransparencyMode::TWO_PASSES_ONE_SIDE) ?
                        gpu::CompareOp::GREATER_OR_EQUAL : draw_item.rasterState.depthFunc;

                }
                else
                {
                    // color pass:
                   // This will bucket objects by Z, front-to-back and then sort by material
                   // in each buckets. We use the top 10 bits of the distance, which
                   // bucketizes the depth by its log2 and in 4 linear chunks in each bucket.
                    draw_item.key &= ~Z_BUCKET_MASK;
                    draw_item.key |= MakeField(distanceBits >> 22u, Z_BUCKET_MASK,
                        Z_BUCKET_SHIFT);

                    curr->key = to_underlying(Pass::SENTINEL);
                    ++curr;
                }
                
                
                *curr = draw_item;
                ++curr;
            }
        }

        std::sort(drawItems.begin(), drawItems.end());
        DrawItem const* const last = std::partition_point(drawItems.begin(), drawItems.end(),
            [](DrawItem const& c) {
                return c.key != to_underlying(Pass::SENTINEL);
            });
        drawItems.resize(last - drawItems.begin());
        const auto& items = drawItems;

        constexpr gpu::TextureSampleCount msaa_sample_count = gpu::TextureSampleCount::COUNT_4;

        const gpu::ColorAttachmentDesc color_attachment_desc = {
            .nodeID = renderGraph.create_texture(
            "Lighting Color Target",
                gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA8, 1, scene_resolution, true, {}, msaa_sample_count)),
            .clear = true,
            .clearValue = gpu::ClearValue(Vec4f(0.0f, 0.0f, 1.0f, 1.0f), 0.0f, 0)
        };

        const gpu::ResolveAttachmentDesc resolve_attachment_desc = {
        	.nodeID = renderGraph.create_texture(
                "Resolve Target",
                gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA8, 1, scene_resolution, true))
        };

        const gpu::DepthStencilAttachmentDesc depth_attachment_desc = {
            .nodeID = renderGraph.create_texture(
                "Depth Target",
                gpu::RGTextureDesc::create_d2(gpu::TextureFormat::DEPTH32F, 1, scene_resolution, true, gpu::ClearValue(), msaa_sample_count)),
            .clear = true,
        };

        uint8 structure_mip_level = soul::cast<uint8>(renderGraph.get_texture_desc(input.structureTex, *gpuSystem).mipLevels);

        const auto& node = renderGraph.add_graphic_pass<Parameter>("Lighting Pass",
            gpu::RGRenderTargetDesc(
                scene_resolution,
                msaa_sample_count,
                color_attachment_desc,
                resolve_attachment_desc,
				depth_attachment_desc
			),
	        [this, &input_param, structure_mip_level](gpu::RGShaderPassDependencyBuilder& builder, Parameter& params)
	        {
	            params.frameUniformBuffer = builder.add_shader_buffer(input_param.frameUniformBuffer, { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT }, gpu::ShaderBufferReadUsage::UNIFORM);
	            params.lightUniformBuffer = builder.add_shader_buffer(input_param.lightUniformBuffer, { gpu::ShaderStage::FRAGMENT }, gpu::ShaderBufferReadUsage::UNIFORM);
	            params.shadowUniformBuffer = builder.add_shader_buffer(input_param.shadowUniformBuffer, { gpu::ShaderStage::FRAGMENT }, gpu::ShaderBufferReadUsage::UNIFORM);
	            params.froxelRecordsUniformBuffer = builder.add_shader_buffer(input_param.froxelRecordsUniformBuffer, { gpu::ShaderStage::FRAGMENT }, gpu::ShaderBufferReadUsage::UNIFORM);
	            params.materialUniformBuffer = builder.add_shader_buffer(input_param.materialUniformBuffer, { gpu::ShaderStage::FRAGMENT }, gpu::ShaderBufferReadUsage::UNIFORM);
	            params.boneUniformBuffer = builder.add_shader_buffer(input_param.boneUniformBuffer, { gpu::ShaderStage::VERTEX }, gpu::ShaderBufferReadUsage::UNIFORM);
	            params.objectUniformBuffer = builder.add_shader_buffer(input_param.objectUniformBuffer, { gpu::ShaderStage::VERTEX , gpu::ShaderStage::FRAGMENT }, gpu::ShaderBufferReadUsage::UNIFORM);
	        	params.structureTex = builder.add_shader_texture(input_param.structureTex, { gpu::ShaderStage::FRAGMENT }, gpu::ShaderTextureReadUsage::UNIFORM, gpu::SubresourceIndexRange(gpu::SubresourceIndex(0, 0), structure_mip_level));
	            params.shadowMap = builder.add_shader_texture(input_param.shadowMap, { gpu::ShaderStage::FRAGMENT }, gpu::ShaderTextureReadUsage::UNIFORM);
	        },
			[&scene, &renderData, this, items](const Parameter& params, gpu::RenderGraphRegistry& registry, gpu::GraphicCommandList& command_list)
			{
                soul::Vec2ui16 resolution = { soul::cast<uint16>(scene.get_viewport().x) , soul::cast<uint16>(scene.get_viewport().y) };

	            gpu::GraphicPipelineStateDesc pipeline_base_desc;
	            pipeline_base_desc.viewport = { 0, 0, resolution.x, resolution.y };
	            pipeline_base_desc.scissor = { false, 0, 0, resolution.x, resolution.y };
	            pipeline_base_desc.colorAttachmentCount = 1;
	            pipeline_base_desc.depthStencilAttachment = { true, true, gpu::CompareOp::GREATER_OR_EQUAL };

	            gpu::SamplerDesc sampler_desc = gpu::SamplerDesc::SameFilterWrap(gpu::TextureFilter::LINEAR, gpu::TextureWrap::REPEAT);
	            gpu::SamplerID sampler_id = gpuSystem->request_sampler(sampler_desc);

	            gpu::SamplerDesc structure_sampler_desc = gpu::SamplerDesc::SameFilterWrap(gpu::TextureFilter::NEAREST, gpu::TextureWrap::CLAMP_TO_EDGE);
	            gpu::SamplerID structure_sampler_id = gpuSystem->request_sampler(structure_sampler_desc);

	            gpu::SamplerDesc shadow_sampler_desc = gpu::SamplerDesc::SameFilterWrap(gpu::TextureFilter::LINEAR, gpu::TextureWrap::CLAMP_TO_EDGE, false, 1.00, true, gpu::CompareOp::GREATER_OR_EQUAL);
	            gpu::SamplerID shadow_sampler_id = gpuSystem->request_sampler(shadow_sampler_desc);

	            const IBL& ibl = scene.get_ibl();
	            const DFG& dfg = scene.get_dfg();

	            gpu::TextureID stub_texture = renderData.stubTexture;

	            gpu::Descriptor set0_descriptors[] = {
	                gpu::Descriptor::Uniform(registry.get_buffer(params.frameUniformBuffer), 0, gpu::SHADER_STAGES_VERTEX_FRAGMENT),
	                gpu::Descriptor::Uniform(registry.get_buffer(params.lightUniformBuffer), 0, { gpu::ShaderStage::FRAGMENT }),
	                gpu::Descriptor::Uniform(registry.get_buffer(params.shadowUniformBuffer), 0, { gpu::ShaderStage::FRAGMENT }),
	                gpu::Descriptor::Uniform(registry.get_buffer(params.froxelRecordsUniformBuffer), 0, { gpu::ShaderStage::FRAGMENT }),
	                gpu::Descriptor::SampledImage(registry.get_texture(params.shadowMap), shadow_sampler_id, { gpu::ShaderStage::FRAGMENT }),
	                gpu::Descriptor::SampledImage(stub_texture, sampler_id, { gpu::ShaderStage::FRAGMENT }),
	                gpu::Descriptor::SampledImage(
	                    dfg.tex,
	                    gpuSystem->request_sampler(gpu::SamplerDesc::SameFilterWrap(gpu::TextureFilter::LINEAR, gpu::TextureWrap::CLAMP_TO_EDGE)),
	                    { gpu::ShaderStage::FRAGMENT }),
	                gpu::Descriptor::SampledImage(
	                    ibl.reflectionTex,
	                    gpuSystem->request_sampler(gpu::SamplerDesc::SameFilterWrap(gpu::TextureFilter::LINEAR, gpu::TextureWrap::CLAMP_TO_EDGE)),
	                    { gpu::ShaderStage::FRAGMENT }),
	                gpu::Descriptor::SampledImage(stub_texture, sampler_id, { gpu::ShaderStage::FRAGMENT }),
	                gpu::Descriptor::SampledImage(stub_texture, sampler_id, { gpu::ShaderStage::FRAGMENT }),
	                gpu::Descriptor::SampledImage(registry.get_texture(params.structureTex), structure_sampler_id, {gpu::ShaderStage::FRAGMENT}),
	            };
	            gpu::ShaderArgSetID set0 = registry.get_shader_arg_set(0, { std::size(set0_descriptors), set0_descriptors });

	            using DrawCommand = gpu::RenderCommandDrawPrimitive;

	            const auto get_material_gpu_texture = [&scene, stub_texture](TextureID sceneTextureID) -> soul::gpu::TextureID
	            {
	                return sceneTextureID.is_null() ? stub_texture : scene.textures()[sceneTextureID.id].gpuHandle;
	            };

	            command_list.push<DrawCommand>(items.size(), [&, items, sampler_id, set0](soul_size command_index)
	            {
	                gpu::GraphicPipelineStateDesc pipeline_desc = pipeline_base_desc;

	                const DrawItem& draw_item = items[command_index];
	                const Primitive& primitive = *draw_item.primitive;
	                const Material& material = *draw_item.material;
	                DrawItem::ToPipelineStateDesc(draw_item, pipeline_desc);

	                const gpu::Descriptor set1_descriptors[] = {
	                    gpu::Descriptor::Uniform(registry.get_buffer(params.materialUniformBuffer), soul::cast<uint32>(primitive.materialID.id), gpu::SHADER_STAGES_VERTEX_FRAGMENT)
	                };
	                const gpu::ShaderArgSetID set1 = registry.get_shader_arg_set(1, { std::size(set1_descriptors), set1_descriptors});

	                const gpu::Descriptor set2_descriptors[] = {
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.baseColorTexture), sampler_id, gpu::SHADER_STAGES_VERTEX_FRAGMENT),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.metallicRoughnessTexture), sampler_id, gpu::SHADER_STAGES_VERTEX_FRAGMENT),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.normalTexture), sampler_id, gpu::SHADER_STAGES_VERTEX_FRAGMENT),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.occlusionTexture), sampler_id, gpu::SHADER_STAGES_VERTEX_FRAGMENT),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.emissiveTexture), sampler_id, gpu::SHADER_STAGES_VERTEX_FRAGMENT),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.clearCoatTexture), sampler_id, gpu::SHADER_STAGES_VERTEX_FRAGMENT),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.clearCoatRoughnessTexture), sampler_id, gpu::SHADER_STAGES_VERTEX_FRAGMENT),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.clearCoatNormalTexture), sampler_id, gpu::SHADER_STAGES_VERTEX_FRAGMENT),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.sheenColorTexture), sampler_id, gpu::SHADER_STAGES_VERTEX_FRAGMENT),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.sheenRoughnessTexture), sampler_id, gpu::SHADER_STAGES_VERTEX_FRAGMENT),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.transmissionTexture), sampler_id, gpu::SHADER_STAGES_VERTEX_FRAGMENT),
						gpu::Descriptor::SampledImage(get_material_gpu_texture(material.textures.volumeThicknessTexture), sampler_id, gpu::SHADER_STAGES_VERTEX_FRAGMENT)
	                };
	                const gpu::ShaderArgSetID set2 = registry.get_shader_arg_set(2, { std::size(set2_descriptors), set2_descriptors});

	                soul::Array<gpu::Descriptor> set3_descriptors;
	                set3_descriptors.reserve(gpu::MAX_BINDING_PER_SET);
	                set3_descriptors.add(gpu::Descriptor::Uniform(registry.get_buffer(params.objectUniformBuffer), draw_item.index, gpu::SHADER_STAGES_VERTEX_FRAGMENT));
	                const SkinID skin_id = renderData.renderables.elementAt<RenderablesIdx::SKIN_ID>(draw_item.index);
	                if (const Visibility visibility = renderData.renderables.elementAt<RenderablesIdx::VISIBILITY_STATE>(draw_item.index); visibility.skinning || visibility.morphing) {
	                    uint32 skin_index = skin_id.is_null() ? 0 : soul::cast<uint32>(skin_id.id);
	                    set3_descriptors.add(gpu::Descriptor::Uniform(registry.get_buffer(params.boneUniformBuffer), skin_index, { gpu::ShaderStage::VERTEX }));
	                }
	                const gpu::ShaderArgSetID set3 = registry.get_shader_arg_set(3, { soul::cast<uint32>(set3_descriptors.size()), set3_descriptors.data() });

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

			});

        return {
            node.get_render_target().resolveAttachments[0].outNodeID,
            node.get_render_target().depthStencilAttachment.outNodeID
        };
    }


}