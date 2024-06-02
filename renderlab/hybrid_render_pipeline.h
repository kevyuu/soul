#pragma once

#include "render_pipeline.h"

#include "render_nodes/ddgi/ddgi_node.h"
#include "render_nodes/deferred_shading/deferred_shading_node.h"
#include "render_nodes/gbuffer_generate/gbuffer_generate_node.h"
#include "render_nodes/render_constant_name.h"
#include "render_nodes/rt_reflection/rt_reflection_node.h"
#include "render_nodes/rtao/rtao_node.h"
#include "render_nodes/shadow/shadow_node.h"
#include "render_nodes/taa/taa_node.h"
#include "render_nodes/tone_map/tone_map_node.h"

#include <fstream>

namespace renderlab
{
  struct HybridRenderPipeline
  {
  private:
    inline static void CreateBrdfLutFromFile(NotNull<RenderPipeline*> pipeline, const Path& path)
    {
      static constexpr auto brdf_lut_size = 512;

      Vector<u16> buffer = Vector<u16>::WithSize(brdf_lut_size * brdf_lut_size * 2);

      std::fstream f(path.c_str(), std::ios::in | std::ios::binary);

      f.seekp(0);
      f.read((char*)buffer.data(), buffer.size_in_bytes());
      f.close();

      const auto usage        = gpu::TextureUsageFlags({gpu::TextureUsage::SAMPLED});
      const auto texture_desc = gpu::TextureDesc::d2(
        gpu::TextureFormat::RG16F,
        1,
        usage,
        {
          gpu::QueueType::GRAPHIC,
          gpu::QueueType::COMPUTE,
        },
        {brdf_lut_size, brdf_lut_size});

      const gpu::TextureRegionUpdate region_load = {
        .subresource =
          {
            .layer_count = 1,
          },
        .extent = vec3u32(brdf_lut_size, brdf_lut_size, 1),
      };

      const auto raw_data = buffer.cspan();

      const gpu::TextureLoadDesc load_desc = {
        .data            = raw_data.data(),
        .data_size       = raw_data.size_in_bytes(),
        .regions         = {&region_load, 1},
        .generate_mipmap = false,
      };

      pipeline->create_constant_texture(
        RenderConstantName::BRDF_LUT_TEXTURE, texture_desc, load_desc);
    }

  public:
    static constexpr CompStr gbuffer_node_name          = "GBuffer Generation Node"_str;
    static constexpr CompStr deferred_shading_node_name = "Deferred Shading Node"_str;
    static constexpr CompStr shadow_node_name           = "Shadow Node"_str;
    static constexpr CompStr rtao_node_name             = "Rtao Node"_str;
    static constexpr CompStr ddgi_node_name             = "Ddgi Node"_str;
    static constexpr CompStr rt_reflection_node_name    = "Rt Reflection Node"_str;
    static constexpr CompStr taa_node_name              = "Taa Node"_str;
    static constexpr CompStr tone_map_node_name         = "Tone Map Node"_str;

    static auto Create(NotNull<Scene*> scene) -> RenderPipeline
    {
      NotNull<gpu::System*> gpu_system = scene->get_gpu_system();
      RenderPipeline render_pipeline(scene);
      render_pipeline.create_constant_texture(
        RenderConstantName::SOBOL_TEXTURE,
        ImageData::FromFile(Path::From("resources/textures/blue_noise/sobol_256_4d.png"_str), 4),
        false);

      render_pipeline.create_constant_texture(
        RenderConstantName::SCRAMBLE_TEXTURE,
        ImageData::FromFile(
          Path::From("resources/textures/blue_noise/scrambling_ranking_128x128_2d_1spp.png"_str),
          4),
        false);

      CreateBrdfLutFromFile(&render_pipeline, Path::From("resources/textures/brdf_lut.bin"_str));

      using Vertex                            = vec2f32;
      static constexpr Vertex QUAD_VERTICES[] = {
        {-0.5f, -0.5f},
        {0.5f, -0.5f},
        {0.5f, 0.5f},
        {-0.5f, 0.5f},
      };

      using Index                           = u16;
      static constexpr Index QUAD_INDICES[] = {0, 1, 2, 2, 3, 0};

      render_pipeline.create_constant_buffer(
        RenderConstantName::QUAD_VERTEX_BUFFER,
        {
          .size        = sizeof(Vertex) * std::size(QUAD_VERTICES),
          .usage_flags = {gpu::BufferUsage::VERTEX},
          .queue_flags = {gpu::QueueType::GRAPHIC},
        },
        QUAD_VERTICES);

      render_pipeline.create_constant_buffer(
        RenderConstantName::QUAD_INDEX_BUFFER,
        {
          .size        = sizeof(Index) * std::size(QUAD_INDICES),
          .usage_flags = {gpu::BufferUsage::INDEX},
          .queue_flags = {gpu::QueueType::GRAPHIC},
        },
        QUAD_INDICES);

      static constexpr vec3f32 UNIT_CUBE_VERTICES[8] = {
        vec3f32(-0.5, -0.5, 0.5),
        vec3f32(0.5, -0.5, 0.5),
        vec3f32(-0.5, 0.5, 0.5),
        vec3f32(0.5, 0.5, 0.5),
        vec3f32(-0.5, 0.5, -0.5),
        vec3f32(0.5, 0.5, -0.5),
        vec3f32(-0.5, -0.5, -0.5),
        vec3f32(0.5, -0.5, -0.5),
      };

      static constexpr Index UNIT_CUBE_INDICES[36] = {
        0, 1, 2, 2, 1, 3, 2, 3, 4, 4, 3, 5, 4, 5, 6, 6, 5, 7,
        6, 7, 0, 0, 7, 1, 1, 7, 3, 3, 7, 5, 6, 0, 4, 4, 0, 2,
      };

      render_pipeline.create_constant_buffer(
        RenderConstantName::UNIT_CUBE_VERTEX_BUFFER,
        {
          .size        = sizeof(vec3f32) * std::size(UNIT_CUBE_VERTICES),
          .usage_flags = {gpu::BufferUsage::VERTEX},
          .queue_flags = {gpu::QueueType::GRAPHIC},
        },
        UNIT_CUBE_VERTICES);

      render_pipeline.create_constant_buffer(
        RenderConstantName::UNIT_CUBE_INDEX_BUFFER,
        {
          .size        = sizeof(Index) * std::size(UNIT_CUBE_INDICES),
          .usage_flags = {gpu::BufferUsage::INDEX},
          .queue_flags = {gpu::QueueType::GRAPHIC},
        },
        UNIT_CUBE_INDICES);

      render_pipeline.generate_node(
        String::From(gbuffer_node_name),
        [&]
        {
          return GBufferGenerateNode(gpu_system);
        });

      render_pipeline.generate_node(
        String::From(shadow_node_name),
        [&]
        {
          return ShadowNode(gpu_system);
        });

      render_pipeline.generate_node(
        String::From(rtao_node_name),
        [&]
        {
          return RtaoNode(gpu_system);
        });

      render_pipeline.generate_node(
        String::From(ddgi_node_name),
        [&]
        {
          return DdgiNode(gpu_system);
        });

      render_pipeline.generate_node(
        String::From(rt_reflection_node_name),
        [&]
        {
          return RtReflectionNode(gpu_system);
        });

      render_pipeline.generate_node(
        String::From(deferred_shading_node_name),
        [&]
        {
          return DeferredShadingNode(gpu_system);
        });

      render_pipeline.generate_node(
        String::From(taa_node_name),
        [&]
        {
          return TaaNode(gpu_system);
        });

      render_pipeline.generate_node(
        String::From(tone_map_node_name),
        [&]
        {
          return ToneMapNode(gpu_system);
        });

      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_NORMAL_ROUGHNESS,
        shadow_node_name,
        ShadowNode::GBUFFER_NORMAL_ROUGHNESS_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_MOTION_CURVE,
        shadow_node_name,
        ShadowNode::GBUFFER_MOTION_CURVE_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_MESHID,
        shadow_node_name,
        ShadowNode::GBUFFER_MESHID_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_DEPTH,
        shadow_node_name,
        ShadowNode::GBUFFER_DEPTH_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::PREV_GBUFFER_NORMAL_ROUGHNESS,
        shadow_node_name,
        ShadowNode::PREV_GBUFFER_NORMAL_ROUGHNESS_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::PREV_GBUFFER_MOTION_CURVE,
        shadow_node_name,
        ShadowNode::PREV_GBUFFER_MOTION_CURVE_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::PREV_GBUFFER_MESHID,
        shadow_node_name,
        ShadowNode::PREV_GBUFFER_MESHID_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::PREV_GBUFFER_DEPTH,
        shadow_node_name,
        ShadowNode::PREV_GBUFFER_DEPTH_INPUT);

      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_NORMAL_ROUGHNESS,
        rtao_node_name,
        RtaoNode::GBUFFER_NORMAL_ROUGHNESS_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_MOTION_CURVE,
        rtao_node_name,
        RtaoNode::GBUFFER_MOTION_CURVE_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_MESHID,
        rtao_node_name,
        RtaoNode::GBUFFER_MESHID_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_DEPTH,
        rtao_node_name,
        RtaoNode::GBUFFER_DEPTH_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::PREV_GBUFFER_NORMAL_ROUGHNESS,
        rtao_node_name,
        RtaoNode::PREV_GBUFFER_NORMAL_ROUGHNESS_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::PREV_GBUFFER_MOTION_CURVE,
        rtao_node_name,
        RtaoNode::PREV_GBUFFER_MOTION_CURVE_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::PREV_GBUFFER_MESHID,
        rtao_node_name,
        RtaoNode::PREV_GBUFFER_MESHID_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::PREV_GBUFFER_DEPTH,
        rtao_node_name,
        RtaoNode::PREV_GBUFFER_DEPTH_INPUT);

      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_NORMAL_ROUGHNESS,
        ddgi_node_name,
        DdgiNode::NORMAL_ROUGHNESS_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_DEPTH,
        ddgi_node_name,
        DdgiNode::DEPTH_INPUT);

      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_NORMAL_ROUGHNESS,
        rt_reflection_node_name,
        RtReflectionNode::GBUFFER_NORMAL_ROUGHNESS_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_MOTION_CURVE,
        rt_reflection_node_name,
        RtReflectionNode::GBUFFER_MOTION_CURVE_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_MESHID,
        rt_reflection_node_name,
        RtReflectionNode::GBUFFER_MESHID_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_DEPTH,
        rt_reflection_node_name,
        RtReflectionNode::GBUFFER_DEPTH_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::PREV_GBUFFER_NORMAL_ROUGHNESS,
        rt_reflection_node_name,
        RtReflectionNode::PREV_GBUFFER_NORMAL_ROUGHNESS_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::PREV_GBUFFER_MOTION_CURVE,
        rt_reflection_node_name,
        RtReflectionNode::PREV_GBUFFER_MOTION_CURVE_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::PREV_GBUFFER_MESHID,
        rt_reflection_node_name,
        RtReflectionNode::PREV_GBUFFER_MESHID_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::PREV_GBUFFER_DEPTH,
        rt_reflection_node_name,
        RtReflectionNode::PREV_GBUFFER_DEPTH_INPUT);

      render_pipeline.add_texture_edge(
        shadow_node_name,
        ShadowNode::OUTPUT,
        deferred_shading_node_name,
        DeferredShadingNode::LIGHT_VISIBILITY_INPUT);
      render_pipeline.add_texture_edge(
        rtao_node_name,
        RtaoNode::OUTPUT,
        deferred_shading_node_name,
        DeferredShadingNode::AO_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_ALBEDO_METAL,
        deferred_shading_node_name,
        DeferredShadingNode::ALBEDO_METALLIC_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_MOTION_CURVE,
        deferred_shading_node_name,
        DeferredShadingNode::MOTION_CURVE_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_NORMAL_ROUGHNESS,
        deferred_shading_node_name,
        DeferredShadingNode::NORMAL_ROUGHNESS_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_EMISSIVE,
        deferred_shading_node_name,
        DeferredShadingNode::EMISSIVE_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_DEPTH,
        deferred_shading_node_name,
        DeferredShadingNode::DEPTH_INPUT);
      render_pipeline.add_texture_edge(
        ddgi_node_name,
        DdgiNode::OUTPUT,
        deferred_shading_node_name,
        DeferredShadingNode::INDIRECT_DIFFUSE_INPUT);
      render_pipeline.add_texture_edge(
        rt_reflection_node_name,
        RtReflectionNode::OUTPUT,
        deferred_shading_node_name,
        DeferredShadingNode::INDIRECT_SPECULAR_INPUT);

      render_pipeline.add_texture_edge(
        deferred_shading_node_name,
        DeferredShadingNode::OUTPUT,
        taa_node_name,
        TaaNode::COLOR_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name, GBufferGenerateNode::GBUFFER_DEPTH, taa_node_name, TaaNode::DEPTH_INPUT);
      render_pipeline.add_texture_edge(
        gbuffer_node_name,
        GBufferGenerateNode::GBUFFER_MOTION_CURVE,
        taa_node_name,
        TaaNode::GBUFFER_MOTION_CURVE_INPUT);

      render_pipeline.add_texture_edge(
        taa_node_name, TaaNode::OUTPUT, tone_map_node_name, ToneMapNode::INPUT);

      render_pipeline.set_output(tone_map_node_name, DeferredShadingNode::OUTPUT);

      return std::move(render_pipeline);
    }
  };
} // namespace renderlab
