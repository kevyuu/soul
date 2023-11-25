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
#include "shaders/rt_basic_type.hlsl"
#include "shaders/rt_type.hlsl"
#include "stb_image.h"

using namespace soul;

struct ObjModel {
  uint32_t indices_count{0};
  uint32_t vertices_count{0};
  gpu::BufferID vertex_buffer;    // Device buffer of all 'Vertex'
  gpu::BufferID index_buffer;     // Device buffer of the indices forming triangles
  gpu::BufferID mat_color_buffer; // Device buffer of array of 'Wavefront material'
  gpu::BufferID mat_index_buffer; // Device buffer of array of 'Wavefront material'
};

struct ObjInstance {
  mat4f transform;  // Matrix of the instance
  u32 obj_index{0}; // Model index reference
};

struct Texture {
  String name;
  gpu::TextureID texture_id;

  Texture() = default;
};

class RTBasicSampleApp final : public App
{
  Texture2DRGPass texture_2d_pass;

  SBOVector<ObjModel> models_;
  SBOVector<RTObjDesc> gpu_obj_descs_;
  gpu::BufferID gpu_obj_buffer_;
  SBOVector<Texture> textures_;
  SBOVector<ObjInstance> instances_;
  SBOVector<gpu::BlasID> blas_ids_;
  gpu::TlasID tlas_id_;
  gpu::BlasGroupID blas_group_id_;
  gpu::SamplerID sampler_id_;

  gpu::ProgramID program_id_;
  gpu::ShaderTableID shader_table_id_;
  RTObjScene gpu_scene_;
  bool need_rebuild_blas_ = false;
  bool need_rebuild_tlas_ = false;

  vec4f clear_color_ = vec4f(1.0f, 1.0f, 1.0f, 1.0f);

  struct Light {
    vec3f position = {10.f, 15.f, 8.f};
    float intensity = 100.0f;
    int type = 0;
  } light_;

  AABB bounding_box_;

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

    auto blas_group_node_id = render_graph.import_blas_group("Blas Group", blas_group_id_);
    if (need_rebuild_blas_) {
      struct BuildBlasParameter {
        gpu::BlasGroupNodeID blas_group_node_id;
      };
      blas_group_node_id =
        render_graph
          .add_non_shader_pass<BuildBlasParameter>(
            "Build blas group",
            gpu::QueueType::COMPUTE,
            [blas_group_node_id](auto& parameter, auto& builder) {
              parameter.blas_group_node_id = builder.add_as_build_dst(blas_group_node_id);
            },
            [this](const auto& parameter, const auto& registry, auto& command_list) {
              runtime::ScopeAllocator scope_allocator("build blas execute");
              Vector<gpu::RenderCommandBuildBlas> render_commands(&scope_allocator);
              for (usize model_idx = 0; model_idx < models_.size(); model_idx++) {
                const auto& model = models_[model_idx];
                const auto geom_desc =
                  scope_allocator.create<gpu::RTGeometryDesc>(gpu::RTGeometryDesc{
                    .type = gpu::RTGeometryType::TRIANGLE,
                    .flags = {gpu::RTGeometryFlag::OPAQUE},
                    .content =
                      {
                        .triangles =
                          {
                            .vertex_format = gpu::TextureFormat::RGB32F,
                            .vertex_data = gpu_system_->get_gpu_address(model.vertex_buffer),
                            .vertex_stride = sizeof(VertexObj),
                            .vertex_count = model.vertices_count,
                            .index_type = gpu::IndexType::UINT32,
                            .index_data = gpu_system_->get_gpu_address(model.index_buffer),
                            .index_count = model.indices_count,
                          },
                      },
                  });

                render_commands.push_back(gpu::RenderCommandBuildBlas{
                  .src_blas_id = gpu::BlasID::null(),
                  .dst_blas_id = blas_ids_[model_idx],
                  .build_mode = gpu::RTBuildMode::REBUILD,
                  .build_desc =
                    {
                      .flags = {gpu::RTBuildFlag::PREFER_FAST_BUILD},
                      .geometry_count = 1,
                      .geometry_descs = geom_desc,
                    },
                });
              }

              static constexpr auto MAX_BLAS_BUILD_MEMORY = 1ull << 29;
              command_list.push(gpu::RenderCommandBatchBuildBlas{
                .builds = u32cspan(render_commands.data(), render_commands.size()),
                .max_build_memory_size = MAX_BLAS_BUILD_MEMORY,
              });
            })
          .get_parameter()
          .blas_group_node_id;
      need_rebuild_blas_ = false;
    }

    auto tlas_node_id = render_graph.import_tlas("Tlas", tlas_id_);
    if (need_rebuild_tlas_) {
      auto instance_buffer = render_graph.create_buffer(
        "Instance buffer", {.size = sizeof(gpu::RTInstanceDesc) * instances_.size()});
      struct UploadInstanceBufferParameter {
        gpu::BufferNodeID instance_buffer;
      };
      instance_buffer =
        render_graph
          .add_non_shader_pass<UploadInstanceBufferParameter>(
            "Upload instance buffer",
            gpu::QueueType::TRANSFER,
            [instance_buffer](auto& parameter, auto& builder) {
              parameter.instance_buffer = builder.add_dst_buffer(instance_buffer);
            },
            [this](const auto& parameter, const auto& registry, auto& command_list) {
              Vector<gpu::RTInstanceDesc> instance_descs;
              for (const auto& instance : instances_) {
                instance_descs.emplace_back(
                  instance.transform,
                  instance.obj_index,
                  0xFF,
                  0,
                  gpu::RTGeometryInstanceFlags{
                    gpu::RTGeometryInstanceFlag::TRIANGLE_FACING_CULL_DISABLE},
                  gpu_system_->get_gpu_address(blas_ids_[instance.obj_index]));
              }
              const gpu::BufferRegionCopy region = {
                .size = sizeof(gpu::RTInstanceDesc) * instances_.size()};
              command_list.push(gpu::RenderCommandUpdateBuffer{
                .dst_buffer = registry.get_buffer(parameter.instance_buffer),
                .data = instance_descs.data(),
                .regions = u32cspan(&region, 1),
              });
            })
          .get_parameter()
          .instance_buffer;

      struct BuildTlasParameter {
        gpu::BlasGroupNodeID blas_group_node_id;
        gpu::TlasNodeID tlas_node_id;
        gpu::BufferNodeID instance_buffer;
      };
      tlas_node_id =
        render_graph
          .add_non_shader_pass<BuildTlasParameter>(
            "Build Tlas Pass",
            gpu::QueueType::COMPUTE,
            [blas_group_node_id, tlas_node_id, instance_buffer](auto& parameter, auto& builder) {
              parameter.blas_group_node_id = builder.add_as_build_input(blas_group_node_id);
              parameter.tlas_node_id = builder.add_as_build_dst(tlas_node_id);
              parameter.instance_buffer = builder.add_as_build_input(instance_buffer);
            },
            [this](const auto& parameter, const auto& registry, auto& command_list) {
              command_list.push(gpu::RenderCommandBuildTlas{
                .tlas_id = registry.get_tlas(parameter.tlas_node_id),
                .build_desc =
                  {
                    .instance_data =
                      gpu_system_->get_gpu_address(registry.get_buffer(parameter.instance_buffer)),
                    .instance_count = soul::cast<u32>(instances_.size()),
                  },
              });
            })
          .get_parameter()
          .tlas_node_id;
      need_rebuild_tlas_ = false;
    }

    const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

    const auto scene_buffer =
      render_graph.create_buffer("Scene Buffer", {.size = sizeof(RTObjScene)});
    struct GPUSceneUploadPassParameter {
      gpu::BufferNodeID buffer;
    };

    const auto projection =
      math::perspective(math::radians(45.0f), math::fdiv(viewport.x, viewport.y), 0.1f, 10000.0f);
    const auto projection_inverse = math::inverse(projection);
    const auto view_inverse = math::inverse(camera_man_.get_view_matrix());
    gpu_scene_ = {
      .gpu_obj_buffer_descriptor_id = gpu_system_->get_ssbo_descriptor_id(gpu_obj_buffer_),
      .camera_position = camera_man_.get_position(),
      .view_inverse = view_inverse,
      .projection_inverse = projection_inverse,
      .clear_color = clear_color_,
      .light_position = light_.position,
      .light_intensity = light_.intensity,
      .light_type = light_.type,
    };

    const auto scene_upload_parameter =
      render_graph
        .add_non_shader_pass<GPUSceneUploadPassParameter>(
          "GPUScene upload",
          gpu::QueueType::TRANSFER,
          [scene_buffer](auto& parameter, auto& builder) {
            parameter.buffer = builder.add_dst_buffer(scene_buffer, gpu::TransferDataSource::CPU);
          },
          [this](const auto& parameter, auto& registry, auto& command_list) {
            using Command = gpu::RenderCommandUpdateBuffer;
            const gpu::BufferRegionCopy region_copy = {.dst_offset = 0, .size = sizeof(RTObjScene)};
            const Command command = {
              .dst_buffer = registry.get_buffer(parameter.buffer),
              .data = soul::cast<void*>(&gpu_scene_),
              .regions = {&region_copy, 1},
            };
            command_list.push(command);
          })
        .get_parameter();

    const gpu::TextureNodeID target_texture = render_graph.create_texture(
      "Target Texture",
      gpu::RGTextureDesc::create_d2(
        gpu::TextureFormat::RGBA8,
        1,
        viewport,
        true,
        gpu::ClearValue(vec4f{0.0f, 0.0f, 0.0f, 1.0f}, 0.0f, 0.0f)));

    struct RayTracingPassParameter {
      gpu::TextureNodeID target_texture;
      gpu::BufferNodeID scene_buffer;
      gpu::TlasNodeID tlas;
      gpu::BlasGroupNodeID blas_group;
    };
    const auto rt_pass_param =
      render_graph
        .add_ray_tracing_pass<RayTracingPassParameter>(
          "Ray Tracing Pass",
          [target_texture, scene_upload_parameter, tlas_node_id, blas_group_node_id](
            auto& parameter, auto& builder) {
            parameter.target_texture = builder.add_shader_texture(
              target_texture, {gpu::ShaderStage::RAYGEN}, gpu::ShaderTextureWriteUsage::STORAGE);
            parameter.scene_buffer = builder.add_shader_buffer(
              scene_upload_parameter.buffer,
              {gpu::ShaderStage::RAYGEN, gpu::ShaderStage::CLOSEST_HIT, gpu::ShaderStage::MISS},
              gpu::ShaderBufferReadUsage::STORAGE);
            parameter.tlas = builder.add_shader_tlas(tlas_node_id, gpu::SHADER_STAGES_RAY_TRACING);
            parameter.blas_group =
              builder.add_shader_blas_group(blas_group_node_id, gpu::SHADER_STAGES_RAY_TRACING);
          },
          [this, viewport](const auto& parameter, auto& registry, auto& command_list) {
            RayTracingPushConstant push_constant = {
              .scene_descriptor_id =
                gpu_system_->get_ssbo_descriptor_id(registry.get_buffer(parameter.scene_buffer)),
              .as_descriptor_id =
                gpu_system_->get_as_descriptor_id(registry.get_tlas(parameter.tlas)),
              .image_descriptor_id =
                gpu_system_->get_uav_descriptor_id(registry.get_texture(parameter.target_texture)),
              .sampler_descriptor_id = gpu_system_->get_sampler_descriptor_id(sampler_id_),
            };
            using Command = gpu::RenderCommandRayTrace;
            command_list.template push<Command>({
              .shader_table_id = shader_table_id_,
              .push_constant_data = &push_constant,
              .push_constant_size = sizeof(RayTracingPushConstant),
              .dimension = vec3ui32(viewport.x, viewport.y, 1.0),
            });
          })
        .get_parameter();

    const Texture2DRGPass::Parameter texture_2d_parameter = {
      .sampled_texture = rt_pass_param.target_texture,
      .render_target = render_target,
    };
    return texture_2d_pass.add_pass(texture_2d_parameter, render_graph);
  }

  auto load_model(const Path& model_path, const mat4f transform = mat4f::identity()) -> void
  {
    instances_.push_back(
      ObjInstance{.transform = transform, .obj_index = soul::cast<u32>(models_.size())});

    ObjLoader obj_loader;
    obj_loader.load_model(model_path);

    const gpu::BufferDesc vertex_buffer_desc = {
      .size = obj_loader.vertices.size() * sizeof(VertexObj),
      .usage_flags =
        {gpu::BufferUsage::VERTEX, gpu::BufferUsage::STORAGE, gpu::BufferUsage::AS_BUILD_INPUT},
      .queue_flags = {gpu::QueueType::GRAPHIC},
      .name = "Vertex buffer",
    };
    const auto vertex_buffer =
      gpu_system_->create_buffer(vertex_buffer_desc, obj_loader.vertices.data());

    const gpu::BufferDesc index_buffer_desc = {
      .size = obj_loader.indices.size() * sizeof(IndexObj),
      .usage_flags =
        {gpu::BufferUsage::INDEX, gpu::BufferUsage::STORAGE, gpu::BufferUsage::AS_BUILD_INPUT},
      .queue_flags = {gpu::QueueType::GRAPHIC},
      .name = "Index buffer",
    };
    const auto index_buffer =
      gpu_system_->create_buffer(index_buffer_desc, obj_loader.indices.data());

    for (const auto& texture_name : obj_loader.textures) {
      textures_.push_back(Texture{});
      auto& texture = textures_.back();
      texture.name = String::From(texture_name.c_str());
      const auto texture_path = get_media_path() / "textures" / texture_name;
      int texture_width, texture_height, texture_channel_count;
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

    SBOVector<WavefrontMaterial> gpu_materials;
    for (const auto& material : obj_loader.materials) {
      gpu_materials.push_back(WavefrontMaterial{
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
      });
    }

    const gpu::BufferDesc material_buffer_desc = {
      .size = gpu_materials.size() * sizeof(WavefrontMaterial),
      .usage_flags = {gpu::BufferUsage::STORAGE},
      .queue_flags = {gpu::QueueType::GRAPHIC},
      .name = "Material buffer",
    };
    const auto material_buffer =
      gpu_system_->create_buffer(material_buffer_desc, gpu_materials.data());

    const gpu::BufferDesc material_indices_buffer_desc = {
      .size = obj_loader.mat_indexes.size() * sizeof(MaterialIndexObj),
      .usage_flags = {gpu::BufferUsage::STORAGE},
      .queue_flags = {gpu::QueueType::GRAPHIC},
      .name = "Material indices buffer",
    };
    const auto material_indices_buffer =
      gpu_system_->create_buffer(material_indices_buffer_desc, obj_loader.mat_indexes.data());

    const RTObjDesc gpu_obj_desc = {
      .vertex_descriptor_id = gpu_system_->get_ssbo_descriptor_id(vertex_buffer),
      .index_descriptor_id = gpu_system_->get_ssbo_descriptor_id(index_buffer),
      .material_descriptor_id = gpu_system_->get_ssbo_descriptor_id(material_buffer),
      .material_indices_descriptor_id =
        gpu_system_->get_ssbo_descriptor_id(material_indices_buffer),
    };
    gpu_obj_descs_.push_back(gpu_obj_desc);

    models_.push_back(ObjModel{
      .indices_count = soul::cast<u32>(obj_loader.indices.size()),
      .vertices_count = soul::cast<u32>(obj_loader.vertices.size()),
      .vertex_buffer = vertex_buffer,
      .index_buffer = index_buffer,
      .mat_color_buffer = material_buffer,
      .mat_index_buffer = material_indices_buffer,
    });

    bounding_box_ = math::combine(obj_loader.bounding_box, bounding_box_);
  }

  auto create_gpu_obj_desc_buffer() -> void
  {
    gpu_obj_buffer_ = gpu_system_->create_buffer(
      {
        .size = sizeof(RTObjDesc) * gpu_obj_descs_.size(),
        .usage_flags = {gpu::BufferUsage::STORAGE},
        .queue_flags = {gpu::QueueType::GRAPHIC},
        .name = "RTObj buffer",
      },
      gpu_obj_descs_.data());
  }

  auto create_blas() -> void
  {
    blas_group_id_ = gpu_system_->create_blas_group("Blas Group");

    for (const auto& model : models_) {
      const gpu::RTGeometryDesc geom_desc = {
        .type = gpu::RTGeometryType::TRIANGLE,
        .content = {
          .triangles = {
            .vertex_format = gpu::TextureFormat::RGB32F,
            .vertex_stride = sizeof(VertexObj),
            .vertex_count = model.vertices_count,
            .index_type = gpu::IndexType::UINT32,
            .index_count = model.indices_count,
          }}};
      const gpu::BlasBuildDesc build_desc = {.geometry_count = 1, .geometry_descs = &geom_desc};
      const auto blas_size = gpu_system_->get_blas_size_requirement(build_desc);
      blas_ids_.push_back(gpu_system_->create_blas({.size = blas_size}, blas_group_id_));
    }
  }

  auto create_tlas() -> void
  {
    const auto tlas_size = gpu_system_->get_tlas_size_requirement({
      .build_flags = {gpu::RTBuildFlag::PREFER_FAST_BUILD},
      .instance_count = soul::cast<u32>(instances_.size()),
    });
    tlas_id_ = gpu_system_->create_tlas({.name = "Tlas", .size = tlas_size});
  }

public:
  explicit RTBasicSampleApp(const AppConfig& app_config)
      : App(app_config), texture_2d_pass(gpu_system_)
  {
    const auto shader_source =
      gpu::ShaderSource::From(gpu::ShaderFile{.path = Path::From("rt_basic_sample.hlsl"_str)});
    const auto search_path = Path::From("shaders"_str);
    const auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::RAYGEN, "rgen_main"},
      gpu::ShaderEntryPoint{gpu::ShaderStage::MISS, "rmiss_main"},
      gpu::ShaderEntryPoint{gpu::ShaderStage::MISS, "rmiss_shadow_main"},
      gpu::ShaderEntryPoint{gpu::ShaderStage::CLOSEST_HIT, "rchit_main"},
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

    const auto miss_groups = soul::Array{
      gpu::RTGeneralShaderGroup{.entry_point = 1},
      gpu::RTGeneralShaderGroup{.entry_point = 2},
    };

    const gpu::RTTriangleHitGroup hit_group = {.closest_hit_entry_point = 3};
    const gpu::ShaderTableDesc shader_table_desc = {
      .program_id = program_id_,
      .raygen_group = {.entry_point = 0},
      .miss_groups = u32cspan(miss_groups.data(), miss_groups.size()),
      .hit_groups = u32cspan(&hit_group, 1),
      .name = "Shader Table",
    };
    shader_table_id_ = gpu_system_->create_shader_table(shader_table_desc);

    sampler_id_ = gpu_system_->request_sampler(
      gpu::SamplerDesc::same_filter_wrap(gpu::TextureFilter::LINEAR, gpu::TextureWrap::REPEAT));

    load_model(get_media_path() / "scenes"_str / "Medieval_building.obj"_str);
    load_model(get_media_path() / "scenes"_str / "plane.obj"_str);
    create_gpu_obj_desc_buffer();

    create_blas();
    create_tlas();
    need_rebuild_blas_ = true;
    need_rebuild_tlas_ = true;

    constexpr auto distance_multiplier = 2.0f;
    const auto camera_target = bounding_box_.center();
    const auto camera_position =
      camera_target + (bounding_box_.max - camera_target) * distance_multiplier;
    camera_man_.set_camera(camera_position, camera_target, vec3f(0, 1, 0));
  }
};

auto main(int /* argc */, char* /* argv */[]) -> int
{
  RTBasicSampleApp app({.enable_imgui = true});
  app.run();
  return 0;
}
