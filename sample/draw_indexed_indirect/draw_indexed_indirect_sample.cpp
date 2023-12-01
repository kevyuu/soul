#include "core/log.h"
#include "core/sbo_vector.h"
#include "core/type.h"
#include "gpu/gpu.h"
#include "gpu/id.h"
#include "gpu/sl_type.h"
#include "gpu/type.h"
#include "math/math.h"
#include "obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <app.h>
#include <imgui/imgui.h>
#include <texture_2d_pass.h>

#include "camera_manipulator.h"
#include "runtime/scope_allocator.h"

#include "shaders/draw_indexed_indirect_type.hlsl"
#include "shaders/raster_type.hlsl"
#include "stb_image.h"

using namespace soul;

struct Texture {
  String name;
  gpu::TextureID texture_id;

  Texture() = default;
};

class DrawIndexedIndirectSampleApp final : public App
{
  Texture2DRGPass texture_2d_pass;

  gpu::BufferID vertex_buffer_;
  gpu::BufferID index_buffer_;
  gpu::BufferID indirect_buffer_;
  gpu::BufferID instance_buffer_;

  SBOVector<Texture> textures_;
  SBOVector<RasterObjInstanceData> instances_;
  SBOVector<VertexObj> vertex_data_;
  SBOVector<IndexObj> index_data_;
  SBOVector<gpu::DrawIndexedIndirectCommand> indirect_commands_;

  gpu::SamplerID sampler_id_;

  gpu::ProgramID program_id_;
  RasterObjScene gpu_scene_;

  vec4f clear_color_ = vec4f(1.0f, 1.0f, 1.0f, 1.0f);

  struct Light {
    vec3f position = {10.f, 15.f, 8.f};
    float intensity = 100.0f;
    int type = 0;
  } light_;

  AABB bounding_box_;

  struct Vertex {
    vec3f position;
    vec3f normal;
    vec3f color;
    vec2f tex_coord;
  };
  auto render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph)
    -> gpu::TextureNodeID override
  {
    if (ImGui::Begin("Options")) {
      ImGui::ColorEdit3("Clear color", reinterpret_cast<float*>(&clear_color_));
      if (ImGui::CollapsingHeader("Light")) {
        ImGui::RadioButton("Point", &light_.type, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Infinite", &light_.type, 1);

        ImGui::SliderFloat3("Position", &light_.position.x, -20.f, 20.f);
        ImGui::SliderFloat("Intensity", &light_.intensity, 0.f, 150.f);
      }
      ImGui::End();
    }

    const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

    const auto scene_buffer =
      render_graph.create_buffer("Scene Buffer", {.size = sizeof(RasterObjScene)});

    const auto projection = math::perspective(
      math::radians(45.0f), math::fdiv(viewport.x, viewport.y), 0.1f, 1000000000.0f);

    const auto projection_inverse = math::inverse(projection);

    const auto view_inverse = math::inverse(camera_man_.get_view_matrix());

    ////////////////////////
    //
    //
    // RasterSceneUploadPass
    gpu_scene_ = {
      .instance_buffer_descriptor_id = gpu_system_->get_ssbo_descriptor_id(instance_buffer_),
      .view = camera_man_.get_view_matrix(),
      .projection = projection,
      .camera_position = camera_man_.get_position(),
      .light_position = light_.position,
      .light_intensity = light_.intensity,
      .light_type = light_.type,
    };

    struct RasterSceneUploadPassParameter {
      gpu::BufferNodeID buffer;
    };

    const auto scene_upload_parameter =
      render_graph
        .add_non_shader_pass<RasterSceneUploadPassParameter>(
          "GPUScene upload",
          gpu::QueueType::TRANSFER,
          [scene_buffer](auto& parameter, auto& builder) {
            parameter.buffer = builder.add_dst_buffer(scene_buffer, gpu::TransferDataSource::CPU);
          },
          [this](const auto& parameter, auto& registry, auto& command_list) {
            using Command = gpu::RenderCommandUpdateBuffer;
            const gpu::BufferRegionCopy region_copy = {
              .dst_offset = 0, .size = sizeof(RasterObjScene)};
            const Command command = {
              .dst_buffer = registry.get_buffer(parameter.buffer),
              .data = soul::cast<void*>(&gpu_scene_),
              .regions = {&region_copy, 1},
            };
            command_list.push(command);
          })
        .get_parameter();

    /////////////////////
    //
    //
    //
    // Draw Indirect Pass
    struct RasterPassParameter {
      gpu::BufferNodeID scene_buffer;
    };

    const gpu::RGColorAttachmentDesc color_attachment_desc = {
      .node_id = render_target,
      .clear = true,
    };
    auto clear_value = gpu::ClearValue();
    clear_value.depth_stencil.depth = 1.0f;
    const gpu::RGDepthStencilAttachmentDesc depth_attachment_desc = {
      .node_id = render_graph.create_texture(
        "Depth Target",
        gpu::RGTextureDesc::create_d2(gpu::TextureFormat::DEPTH32F, 1, {viewport.x, viewport.y})),
      .clear = true,
      .clear_value = clear_value,
    };
    const auto& raster_node = render_graph.add_raster_pass<RasterPassParameter>(
      "Render Pass",
      gpu::RGRenderTargetDesc(viewport, color_attachment_desc, depth_attachment_desc),
      [raster_scene_buffer = scene_upload_parameter.buffer](auto& parameter, auto& builder) {
        parameter.scene_buffer = builder.add_shader_buffer(
          raster_scene_buffer,
          {gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT},
          gpu::ShaderBufferReadUsage::STORAGE);
      },
      [viewport, this](const auto& parameter, auto& registry, auto& command_list) {
        const gpu::GraphicPipelineStateDesc pipeline_desc = {
          .program_id = program_id_,
          .input_bindings =
            {
              .list =
                {
                  {.stride = sizeof(Vertex)},
                },
            },
          .input_attributes =
            {
              .list =
                {
                  {
                    .binding = 0,
                    .offset = offsetof(Vertex, position),
                    .type = gpu::VertexElementType::FLOAT3,
                  },
                  {
                    .binding = 0,
                    .offset = offsetof(Vertex, normal),
                    .type = gpu::VertexElementType::FLOAT3,
                  },
                  {
                    .binding = 0,
                    .offset = offsetof(Vertex, color),
                    .type = gpu::VertexElementType::FLOAT3,
                  },
                  {
                    .binding = 0,
                    .offset = offsetof(Vertex, tex_coord),
                    .type = gpu::VertexElementType::FLOAT2,
                  },
                },
            },
          .viewport =
            {
              .width = static_cast<float>(viewport.x),
              .height = static_cast<float>(viewport.y),
            },
          .scissor = {.extent = viewport},
          .raster =
            {
              .cull_mode = {gpu::CullMode::BACK},
              .front_face = gpu::FrontFace::COUNTER_CLOCKWISE,
            },
          .color_attachment_count = 1,
          .depth_stencil_attachment =
            {
              .depth_test_enable = true,
              .depth_write_enable = true,
              .depth_compare_op = gpu::CompareOp::LESS,
            },
        };
        const auto pipeline_state_id = registry.get_pipeline_state(pipeline_desc);

        const RasterPushConstant push_constant = {
          .gpu_scene_id =
            gpu_system_->get_ssbo_descriptor_id(registry.get_buffer(parameter.scene_buffer))};

        static constexpr gpu::IndexType INDEX_TYPE =
          sizeof(IndexObj) == 2 ? gpu::IndexType::UINT16 : gpu::IndexType::UINT32;

        command_list.push(gpu::RenderCommandDrawIndexedIndirect{
          .pipeline_state_id = pipeline_state_id,
          .push_constant_data = soul::cast<void*>(&push_constant),
          .push_constant_size = sizeof(RasterPushConstant),
          .vertex_buffer_ids = {vertex_buffer_},
          .index_buffer_id = index_buffer_,
          .index_type = INDEX_TYPE,
          .buffer_id = indirect_buffer_,
          .offset = 0,
          .draw_count = cast<u32>(instances_.size()),
          .stride = sizeof(gpu::DrawIndexedIndirectCommand),

        });
      });

    return raster_node.get_color_attachment_node_id();
  }

  auto load_model(
    const Path& model_path,
    const mat4f transform = mat4f::identity(),
    soulsl::float3 debug_color = {1.0f, 0.0f, 0.0f}) -> void
  {

    ObjLoader obj_loader;
    obj_loader.load_model(model_path);

    for (const auto& texture_name : obj_loader.textures) {
      textures_.push_back(Texture{});
      auto& texture = textures_.back();
      texture.name = String::From(texture_name.c_str());
      const auto texture_path = get_media_path() / "textures" / texture_name;
      int texture_width, texture_height, texture_channel_count; // NOLINT
      stbi_uc* texture_pixels = stbi_load(
        texture_path.string().c_str(),
        &texture_width,
        &texture_height,
        &texture_channel_count,
        STBI_rgb_alpha);

      const gpu::TextureDesc texture_desc = gpu::TextureDesc::d2(
        texture.name.data(),
        gpu::TextureFormat::SRGBA8,
        1,
        {gpu::TextureUsage::SAMPLED},
        {gpu::QueueType::COMPUTE},
        {soul::cast<u32>(texture_width), soul::cast<u32>(texture_height)});

      const gpu::TextureRegionUpdate region_load = {
        .subresource = {.layer_count = 1},
        .extent = {static_cast<u32>(texture_width), static_cast<u32>(texture_height), 1},
      };

      const gpu::TextureLoadDesc load_desc = {
        .data = texture_pixels,
        .data_size = soul::cast<usize>(texture_width * texture_height * 4),
        .regions = {&region_load, 1},
        .generate_mipmap = true,
      };

      texture.texture_id = gpu_system_->create_texture(texture_desc, load_desc);

      stbi_image_free(texture_pixels);
    }

    const auto gpu_materials = soul::SBOVector<WavefrontMaterial>::Transform(
      obj_loader.materials, [this](const auto& material) {
        return WavefrontMaterial{
          .ambient = material.ambient,
          .diffuse = material.diffuse,
          .specular = material.specular,
          .transmittance = material.transmittance,
          .emission = material.emission,
          .shininess = material.shininess,
          .ior = material.ior,
          .dissolve = material.dissolve,
          .illum = material.illum,
          .diffuse_texture_id =
            material.texture_id == -1
              ? gpu::DescriptorID::null()
              : gpu_system_->get_srv_descriptor_id(textures_[material.texture_id].texture_id),
        };
      });

    const gpu::BufferDesc material_buffer_desc = {
      .size = gpu_materials.size() * sizeof(WavefrontMaterial),
      .usage_flags = {gpu::BufferUsage::STORAGE},
      .queue_flags = {gpu::QueueType::GRAPHIC},
      .name = "Material buffer",
    };
    const auto material_buffer =
      gpu_system_->create_buffer(material_buffer_desc, gpu_materials.data());
    gpu_system_->flush_buffer(material_buffer);

    const gpu::BufferDesc material_indices_buffer_desc = {
      .size = obj_loader.mat_indexes.size() * sizeof(MaterialIndexObj),
      .usage_flags = {gpu::BufferUsage::STORAGE},
      .queue_flags = {gpu::QueueType::GRAPHIC},
      .name = "Material indices buffer",
    };
    const auto material_indices_buffer =
      gpu_system_->create_buffer(material_indices_buffer_desc, obj_loader.mat_indexes.data());
    gpu_system_->flush_buffer(material_indices_buffer);

    instances_.push_back(RasterObjInstanceData{
      .transform = transform,
      .normal_matrix = math::transpose(math::inverse(transform)),
      .material_buffer_descriptor_id = gpu_system_->get_ssbo_descriptor_id(material_buffer),
      .material_indices_descriptor_id =
        gpu_system_->get_ssbo_descriptor_id(material_indices_buffer),
      .debug_color = debug_color,
    });

    u32 instance_idx = indirect_commands_.size();
    indirect_commands_.push_back(gpu::DrawIndexedIndirectCommand{
      .index_count = cast<u32>(obj_loader.indices.size()),
      .instance_count = 1,
      .first_index = 0,
      .vertex_offset = cast<i32>(vertex_data_.size()),
      .first_instance = instance_idx,
    });

    vertex_data_.append(obj_loader.vertices);
    index_data_.append(obj_loader.indices);

    bounding_box_ = math::combine(obj_loader.bounding_box, bounding_box_);
  }

public:
  explicit DrawIndexedIndirectSampleApp(const AppConfig& app_config)
      : App(app_config), texture_2d_pass(gpu_system_)
  {
    const auto shader_source = gpu::ShaderSource::From(
      gpu::ShaderFile{.path = Path::From("shaders/draw_indexed_indirect_sample.hlsl"_str)});
    const auto search_path = Path::From("shaders/"_str);
    constexpr auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::VERTEX, "vs_main"_str},
      gpu::ShaderEntryPoint{gpu::ShaderStage::FRAGMENT, "ps_main"_str},
    };
    const gpu::ProgramDesc program_desc = {
      .search_paths = u32cspan(&search_path, 1),
      .sources = u32cspan(&shader_source, 1),
      .entry_points = entry_points.cspan<u32>(),
    };
    auto result = gpu_system_->create_program(program_desc);
    if (result.is_err()) {
      SOUL_PANIC("Fail to create program");
    }
    program_id_ = result.ok_ref();

    sampler_id_ = gpu_system_->request_sampler(
      gpu::SamplerDesc::same_filter_wrap(gpu::TextureFilter::LINEAR, gpu::TextureWrap::REPEAT));

    load_model(
      get_media_path() / "scenes"_str / "plane.obj"_str, mat4f::identity(), {1.0f, 0.0f, 0.0f});
    load_model(
      get_media_path() / "scenes"_str / "Medieval_building.obj"_str,
      mat4f::identity(),
      {0.0f, 1.0f, 0.0f});

    vertex_buffer_ = gpu_system_->create_buffer(
      {
        .size = vertex_data_.size() * sizeof(VertexObj),
        .usage_flags = {gpu::BufferUsage::VERTEX},
        .queue_flags = {gpu::QueueType::GRAPHIC},
        .name = "Vertex buffer",
      },
      vertex_data_.data());
    gpu_system_->flush_buffer(vertex_buffer_);

    index_buffer_ = gpu_system_->create_buffer(
      {
        .size = index_data_.size() * sizeof(IndexObj),
        .usage_flags =
          {gpu::BufferUsage::INDEX, gpu::BufferUsage::STORAGE, gpu::BufferUsage::AS_BUILD_INPUT},
        .queue_flags = {gpu::QueueType::GRAPHIC},
        .name = "Index buffer",
      },
      index_data_.data());
    gpu_system_->flush_buffer(index_buffer_);

    SOUL_LOG_INFO(
      "Indirrect buffer size: {}",
      indirect_commands_.size() * sizeof(gpu::DrawIndexedIndirectCommand));
    indirect_buffer_ = gpu_system_->create_buffer(
      {
        .size = indirect_commands_.size() * sizeof(gpu::DrawIndexedIndirectCommand),
        .usage_flags = {gpu::BufferUsage::INDIRECT},
        .queue_flags = {gpu::QueueType::GRAPHIC},
        .name = "Index buffer",
      },
      indirect_commands_.data());
    gpu_system_->flush_buffer(indirect_buffer_);

    instance_buffer_ = gpu_system_->create_buffer(
      {
        .size = instances_.size() * sizeof(RasterObjInstanceData),
        .usage_flags = {gpu::BufferUsage::STORAGE},
        .queue_flags = {gpu::QueueType::GRAPHIC},
        .name = "Instance data",
      },
      instances_.data());
    gpu_system_->flush_buffer(instance_buffer_);

    constexpr auto distance_multiplier = 2.0f;
    const auto camera_target = bounding_box_.center();
    const auto camera_position =
      camera_target + (bounding_box_.max - camera_target) * distance_multiplier;
    camera_man_.set_camera(camera_position, camera_target, vec3f(0, 1, 0));
  }
};

auto main(int /* argc */, char* /* argv */[]) -> int
{
  DrawIndexedIndirectSampleApp app({.enable_imgui = true});
  app.run();
  return 0;
}
