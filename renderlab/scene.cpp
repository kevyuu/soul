#include "scene.h"
#include "camera_controller.h"
#include "core/compiler.h"
#include "core/matrix.h"
#include "core/option.h"
#include "ecs.h"
#include "gpu/render_graph.h"
#include "gpu/render_graph_registry.h"
#include "math/aabb.h"
#include "math/quaternion.h"
#include "math/scalar.h"
#include "mesh_preprocessor.h"
#include "type.h"
#include "type.shared.hlsl"

#include "scene.hlsl"

#include "core/log.h"
#include "gpu/id.h"
#include "gpu/system.h"
#include "gpu/type.h"
#include "math/math.h"
#include "misc/image_data.h"
#include "runtime/scope_allocator.h"

#include <fstream>
#include <numeric>
#include <ranges>

namespace
{
  auto halton_sequence(i32 base, i32 index) -> f32
  {
    f32 result = 0;
    f32 f      = 1;
    while (index > 0)
    {
      f /= base;
      result += f * (index % base);
      index = floor(index / base);
    }

    return result;
  }

  auto create_hdr_texture_from_file(NotNull<gpu::System*> gpu_system, const Path& path)
    -> gpu::TextureID
  {
    auto image_data = soul::ImageData::FromFile(path, 4);

    const auto usage        = gpu::TextureUsageFlags({gpu::TextureUsage::SAMPLED});
    const auto texture_desc = gpu::TextureDesc::d2(
      gpu::TextureFormat::RGBA32F,
      1,
      usage,
      {
        gpu::QueueType::GRAPHIC,
        gpu::QueueType::COMPUTE,
      },
      image_data.dimension());

    const gpu::TextureRegionUpdate region_load = {
      .subresource = {.layer_count = 1},
      .extent      = vec3u32(image_data.dimension(), 1),
    };

    const auto raw_data = image_data.cspan();

    const gpu::TextureLoadDesc load_desc = {
      .data            = raw_data.data(),
      .data_size       = raw_data.size_in_bytes(),
      .regions         = {&region_load, 1},
      .generate_mipmap = false,
    };
    const auto texture_id = gpu_system->create_texture(""_str, texture_desc, load_desc);
    gpu_system->flush_texture(texture_id, usage);
    return texture_id;
  }

} // namespace

namespace renderlab
{
  auto Scene::Create(NotNull<gpu::System*> gpu_system) -> Scene
  {
    return Scene(gpu_system);
  }

  Scene::Scene(NotNull<gpu::System*> gpu_system)
      : gpu_system_(gpu_system), render_camera_(EntityId::Null())
  {
    render_camera_ = create_camera_entity(CameraEntityDesc{
      .name = "Camera"_str,
      .camera_transform =
        {
          .position = vec3f32(0.0f, 0.0f, 20.0f),
          .target   = vec3f32(0),
          .up       = vec3f32(0, 1, 0),
        },
      .parent_entity_id = EntityId::Null(),
      .camera_component =
        {
          .fovy         = math::radians(45.0f),
          .near_z       = 0.1f,
          .far_z        = 100000,
          .aspect_ratio = 1920.0f / 1080.0f,
        },
    });

    for (i32 jitter_sample_i = 0; jitter_sample_i < jitter_samples_.size(); jitter_sample_i++)
    {
      jitter_samples_[jitter_sample_i] = vec2f32(
        (2.0f * halton_sequence(2, jitter_sample_i) - 1.0f),
        (2.0f * halton_sequence(3, jitter_sample_i) - 1.0f));
    }

    env_map_.texture_id = create_hdr_texture_from_file(
      gpu_system_, Path::From("resources/textures/hdri/farm_sunset_8k.hdr"_str));
    env_map_.setting_data = {
      .transform     = mat4f32::Identity(),
      .inv_transform = mat4f32::Identity(),
      .tint          = vec3f32(1),
      .intensity     = 1.0f,
    };
  }

  auto Scene::create_material_texture(const MaterialTextureDesc& desc) -> MaterialTextureID
  {
    SOUL_ASSERT(
      0, desc.format == gpu::TextureFormat::RGBA8 || desc.format == gpu::TextureFormat::SRGBA8, "");
    const auto usage = gpu::TextureUsageFlags({gpu::TextureUsage::SAMPLED});
    const auto mip_levels =
      std::max<u16>(cast<u16>(math::floor_log2(std::max(desc.dimension.x, desc.dimension.y))), 1);
    const auto gpu_texture_desc = gpu::TextureDesc::d2(
      desc.format,
      mip_levels,
      usage,
      {
        gpu::QueueType::GRAPHIC,
      },
      desc.dimension);

    const gpu::TextureRegionUpdate region_load = {
      .subresource = {.layer_count = 1},
      .extent      = vec3u32(desc.dimension, 1),
    };

    // TODO(kevinyu) : get dimension data size from texture format
    const u64 data_size = cast<u64>(desc.dimension.x) * desc.dimension.y * 4;

    const gpu::TextureLoadDesc load_desc = {
      .data            = desc.data,
      .data_size       = data_size,
      .regions         = {&region_load, 1},
      .generate_mipmap = true,
    };
    const auto index = material_textures_.add(
      gpu_system_->create_texture(String::From(desc.name), gpu_texture_desc, load_desc));
    gpu_system_->flush_texture(material_textures_[index], usage);
    return MaterialTextureID(index);
  };

  auto Scene::create_material(const MaterialDesc& desc) -> MaterialID
  {
    update_flags_.set(UpdateType::MATERIAL_CHANGED);
    auto get_srv_id = [this](MaterialTextureID mat_tex_id)
    {
      if (mat_tex_id.is_null())
      {
        return gpu::DescriptorID();
      } else
      {
        return gpu_system_->get_srv_descriptor_id(material_textures_[mat_tex_id.id]);
      }
    };

    materials_.push_back(Material{
      .base_color_texture_id         = get_srv_id(desc.base_color_texture_id),
      .metallic_roughness_texture_id = get_srv_id(desc.metallic_roughness_texture_id),
      .normal_texture_id             = get_srv_id(desc.normal_texture_id),
      .emissive_texture_id           = get_srv_id(desc.emissive_texture_id),
      .base_color_factor             = desc.base_color_factor,
      .metallic_factor               = desc.metallic_factor,
      .roughness_factor              = desc.roughness_factor,
      .emissive_factor               = desc.emissive_factor,
    });
    material_names_.push_back(String::From(desc.name));

    return MaterialID(materials_.size() - 1);
  };

  auto Scene::create_mesh_group(const MeshGroupDesc& mesh_group_desc) -> MeshGroupID
  {
    update_flags_.set(UpdateType::MESH_CHANGED);
    auto mesh_idx = 0;
    auto meshes   = Vector<Mesh>::Transform(
      mesh_group_desc.mesh_descs,
      [this, &mesh_idx](const MeshDesc& mesh_desc) -> Mesh
      {
        const auto preprocess_result = MeshPreprocessor::GenerateVertexes(mesh_desc);

        const auto mesh = Mesh{
            .flags        = preprocess_result.indexes.has_value<Vector<u16>>()
                              ? MeshInstanceFlags{MeshInstanceFlag::USE_16_BIT_INDICES}
                              : MeshInstanceFlags{},
            .vb_offset    = cast<u32>(vertices_.size()),
            .ib_offset    = cast<u32>(indexes_.size()),
            .vertex_count = cast<u32>(preprocess_result.vertexes.size()),
            .index_count  = preprocess_result.indexes.visit(
            [](const auto& indexes)
            {
              return cast<u32>(indexes.size());
            }),
            .material_id = mesh_desc.material_id,
            .aabb        = mesh_desc.aabb,
        };

        vertices_.append(preprocess_result.vertexes);

        const auto append_to_scene_indexes = soul::VisitorSet{
          [this](const Vector<u32>& indexes)
          {
            indexes_.append(indexes);
          },
          [this](const Vector<u16>& indexes)
          {
            const auto size_as_u32_vec = indexes.size() / 2 + indexes.size() % 2;
            const auto old_size        = indexes_.size();
            indexes_.resize(old_size + size_as_u32_vec);
            std::memcpy(indexes_.data() + old_size, indexes.data(), indexes.size() * sizeof(u16));
          },
        };
        preprocess_result.indexes.visit(append_to_scene_indexes);

        mesh_idx++;
        return mesh;
      });

    math::AABB group_aabb;
    for (const auto& mesh : meshes)
    {
      group_aabb = math::combine(group_aabb, mesh.aabb);
    }

    return MeshGroupID(mesh_groups_.add(MeshGroup{
      .name   = String::From(mesh_group_desc.name),
      .meshes = std::move(meshes),
      .aabb   = group_aabb,
    }));
  }

  auto Scene::create_entity(const EntityDesc& entity_desc) -> EntityId
  {
    update_flags_.set(UpdateType::ENTITY_CHANGED);
    return entity_manager_.create(entity_desc);
  }

  auto Scene::create_camera_entity(const CameraEntityDesc& desc) -> EntityId
  {
    EntityId entity_id = create_entity({
      .name             = desc.name,
      .local_transform  = math::inverse(math::look_at(
        desc.camera_transform.position, desc.camera_transform.target, desc.camera_transform.up)),
      .parent_entity_id = desc.parent_entity_id,
    });
    entity_manager_.add_component<CameraComponent>(entity_id, desc.camera_component);
    return entity_id;
  }

  auto Scene::get_root_entity_id() const -> EntityId
  {
    return entity_manager_.root_entity_id();
  }

  auto Scene::get_entity_first_child(EntityId entity_id) const -> EntityId
  {
    return entity_manager_.hierarchy_data_ref(entity_id).first_child;
  }

  auto Scene::get_entity_next_sibling(EntityId entity_id) const -> EntityId
  {
    return entity_manager_.hierarchy_data_ref(entity_id).next_sibling;
  }

  auto Scene::get_entity_name(EntityId entity_id) const -> StringView
  {
    return entity_manager_.name_ref(entity_id).cview();
  }

  auto Scene::entity_world_transform_ref(EntityId entity_id) const -> const mat4f32&
  {
    return entity_manager_.world_transform_ref(entity_id);
  }

  void Scene::set_world_transform(EntityId entity_id, const mat4f32& world_transform)
  {
    entity_manager_.set_world_transform(entity_id, world_transform);
  }

  auto Scene::entity_local_transform_ref(EntityId entity_id) const -> const mat4f32&
  {
    return entity_manager_.local_transform_ref(entity_id);
  }

  void Scene::set_local_transform(EntityId entity_id, const mat4f32& local_transform)
  {
    entity_manager_.set_local_transform(entity_id, local_transform);
  }

  void Scene::add_render_component(EntityId entity_id, const RenderComponent& render_comp)
  {
    update_flags_.set(UpdateType::RENDERABLE_CHANGED);
    entity_manager_.add_component<RenderComponent>(entity_id, render_comp);
  }

  void Scene::add_light_component(EntityId entity_id, const LightComponent& light_comp)
  {
    update_flags_.set(UpdateType::LIGHT_CHANGED);
    entity_manager_.add_component<LightComponent>(entity_id, light_comp);
  }

  void Scene::set_light_component(EntityId entity_id, const LightComponent& light_component)
  {
    entity_manager_.component_ref<LightComponent>(entity_id) = light_component;
  }

  auto Scene::try_get_light_component(EntityId entity_id) const -> MaybeNull<const LightComponent*>
  {
    if (entity_manager_.has_component<LightComponent>(entity_id))
    {
      return &entity_manager_.component_ref<LightComponent>(entity_id);
    }
    return nilopt;
  }

  void Scene::add_camera_component(EntityId entity_id, const CameraComponent& camera_comp)
  {
    entity_manager_.add_component<CameraComponent>(entity_id, camera_comp);
  }

  void Scene::set_camera_component(EntityId entity_id, const CameraComponent& camera_comp)
  {
    entity_manager_.component_ref<CameraComponent>(entity_id) = camera_comp;
  }

  auto Scene::get_env_map_setting() -> EnvMapSetting
  {
    return {
      .transform = env_map_.setting_data.transform,
      .tint      = env_map_.setting_data.tint,
      .intensity = env_map_.setting_data.intensity,
    };
  }

  void Scene::set_env_map_setting(const EnvMapSetting& env_map_setting)
  {
    env_map_.setting_data.transform     = env_map_setting.transform;
    env_map_.setting_data.inv_transform = math::inverse(env_map_setting.transform);
    env_map_.setting_data.tint          = env_map_setting.tint;
    env_map_.setting_data.intensity     = env_map_setting.intensity;
  }

  auto Scene::get_render_setting() -> RenderSetting
  {
    return render_setting_;
  }

  void Scene::set_render_setting(const RenderSetting& render_setting)
  {
    render_setting_ = render_setting;
  }

  auto Scene::get_render_camera_data() const -> GPUCameraData
  {
    const auto& camera_component = entity_manager_.component_ref<CameraComponent>(render_camera_);

    vec2f32 current_jitter = render_setting_.enable_jitter
                               ? jitter_samples_[render_data_.num_frames % jitter_samples_.size()] /
                                   vec2f32(viewport_.x, viewport_.y) * 0.5f
                               : vec2f32(0, 0);

    const auto near_z = camera_component.near_z;
    const auto far_z  = camera_component.far_z;

    const auto proj_mat_no_jitter =
      math::perspective(camera_component.fovy, camera_component.aspect_ratio, near_z, far_z);
    const auto proj_mat      = math::translate(proj_mat_no_jitter, vec3f32(current_jitter, 0.0f));
    const auto model_mat     = entity_manager_.local_transform_ref(render_camera_);
    const auto view_mat      = math::inverse(model_mat);
    const auto proj_view_mat = math::mul(proj_mat, view_mat);
    const auto inv_view_mat  = math::inverse(view_mat);
    const auto inv_proj_mat  = math::inverse(proj_mat);
    const auto inv_proj_view_mat = math::inverse(proj_view_mat);

    const auto camera_transform = CameraTransform::FromModelMat(model_mat);
    const auto camera_w         = camera_transform.target - camera_transform.position;
    const auto camera_u         = model_mat.col(0).xyz();
    const auto camera_v         = camera_transform.up;

    return {
      .view_mat                = view_mat,
      .proj_mat                = proj_mat,
      .proj_view_mat           = proj_view_mat,
      .proj_view_mat_no_jitter = math::mul(proj_mat_no_jitter, view_mat),
      .inv_view_mat            = inv_view_mat,
      .inv_proj_mat            = inv_proj_mat,
      .inv_proj_view_mat       = inv_proj_view_mat,
      .position                = camera_transform.position,
      .target                  = camera_transform.target,
      .up                      = camera_transform.up,
      .near_z                  = near_z,
      .far_z                   = far_z,
      .camera_w                = camera_w,
      .camera_u                = camera_u,
      .camera_v                = camera_v,
      .jitter                  = current_jitter,
    };
  }

  auto Scene::get_scene_aabb() const -> math::AABB
  {
    math::AABB scene_aabb;
    entity_manager_.for_each_component_with_entity_id<RenderComponent>(
      [this, &scene_aabb](const RenderComponent& comp, EntityId entity_id)
      {
        math::AABB entity_local_aabb = mesh_groups_[comp.mesh_group_id.id].aabb;
        math::AABB entity_world_aabb =
          math::transform(entity_local_aabb, entity_manager_.world_transform_ref(entity_id));
        scene_aabb = math::combine(scene_aabb, entity_world_aabb);
      });
    return scene_aabb;
  }

  auto Scene::is_empty() const -> b8
  {
    return entity_manager_.is_empty();
  }

  void Scene::prepare_world_matrixes_buffer_node(NotNull<gpu::RenderGraph*> render_graph)
  {
    if (entity_manager_.is_empty())
    {
      return;
    }
    const auto world_transforms          = entity_manager_.world_transform_cspan();
    const auto new_world_matrixes_buffer = gpu_system_->create_buffer(
      "World Transforms"_str,
      {
        .size        = entity_manager_.world_transform_cspan().size_in_bytes(),
        .usage_flags = {gpu::BufferUsage::STORAGE},
        .queue_flags = {gpu::QueueType::GRAPHIC, gpu::QueueType::COMPUTE},

      },
      world_transforms.data());
    if (!render_data_.prev_world_matrixes_buffer.is_null())
    {
      if (render_data_.prev_world_matrixes_buffer != render_data_.world_matrixes_buffer)
      {
        gpu_system_->destroy_buffer(render_data_.prev_world_matrixes_buffer);
      }
      render_data_.prev_world_matrixes_buffer = render_data_.world_matrixes_buffer;
    } else
    {
      render_data_.prev_world_matrixes_buffer = new_world_matrixes_buffer;
    }
    render_data_.prev_world_matrixes_buffer_node = render_graph->import_buffer(
      "Prev world transform buffer"_str, render_data_.prev_world_matrixes_buffer);

    render_data_.world_matrixes_buffer = new_world_matrixes_buffer;
    render_data_.world_matrixes_buffer_node =
      render_graph->import_buffer("Wolrd transform buffer"_str, render_data_.world_matrixes_buffer);
  }

  void Scene::prepare_normal_matrixes_buffer_node(NotNull<gpu::RenderGraph*> render_graph)
  {
    if (entity_manager_.is_empty())
    {
      return;
    }
    const auto normal_transforms          = entity_manager_.normal_transform_cspan();
    const auto new_normal_matrixes_buffer = gpu_system_->create_buffer(
      "Normal matrixes"_str,
      {
        .size        = normal_transforms.size_in_bytes(),
        .usage_flags = {gpu::BufferUsage::STORAGE},
        .queue_flags = {gpu::QueueType::GRAPHIC, gpu::QueueType::COMPUTE},

      },
      normal_transforms.data());
    if (!render_data_.prev_normal_matrixes_buffer.is_null())
    {
      if (render_data_.prev_normal_matrixes_buffer != render_data_.normal_matrixes_buffer)
      {
        gpu_system_->destroy_buffer(render_data_.prev_normal_matrixes_buffer);
      }
      render_data_.prev_normal_matrixes_buffer = render_data_.normal_matrixes_buffer;
    } else
    {
      render_data_.prev_normal_matrixes_buffer = new_normal_matrixes_buffer;
    }
    render_data_.prev_normal_matrixes_buffer_node = render_graph->import_buffer(
      "Prev normal matrixes buffer"_str, render_data_.prev_normal_matrixes_buffer);

    render_data_.normal_matrixes_buffer      = new_normal_matrixes_buffer;
    render_data_.normal_matrixes_buffer_node = render_graph->import_buffer(
      "Normal matrixes buffer"_str, render_data_.normal_matrixes_buffer);
  }

  void Scene::prepare_geometry_buffer(NotNull<gpu::RenderGraph*> render_graph)
  {
    if (update_flags_.test(UpdateType::MESH_CHANGED))
    {
      if (!render_data_.static_vertex_buffer.is_null())
      {
        gpu_system_->destroy_buffer(render_data_.static_vertex_buffer);
      }
      render_data_.static_vertex_buffer = gpu_system_->create_buffer(
        "Static Vertex buffer"_str,
        {
          .size = vertices_.size_in_bytes(),
          .usage_flags =
            {
              gpu::BufferUsage::VERTEX,
              gpu::BufferUsage::STORAGE,
              gpu::BufferUsage::AS_BUILD_INPUT,
            },
          .queue_flags = {gpu::QueueType::GRAPHIC, gpu::QueueType::COMPUTE},
        },
        vertices_.data());

      if (!render_data_.index_buffer.is_null())
      {
        gpu_system_->destroy_buffer(render_data_.index_buffer);
      }
      render_data_.index_buffer = gpu_system_->create_buffer(
        "Index Buffer"_str,
        {
          .size = indexes_.size_in_bytes(),
          .usage_flags =
            {
              gpu::BufferUsage::INDEX,
              gpu::BufferUsage::STORAGE,
              gpu::BufferUsage::AS_BUILD_INPUT,
            },
          .queue_flags = {gpu::QueueType::GRAPHIC, gpu::QueueType::COMPUTE},
        },
        indexes_.data());

      gpu_system_->flush_buffer(render_data_.static_vertex_buffer);
      gpu_system_->flush_buffer(render_data_.index_buffer);
    }
  }

  void Scene::prepare_material_buffer(NotNull<gpu::RenderGraph*> render_graph)
  {
    if (update_flags_.test(UpdateType::MATERIAL_CHANGED))
    {
      if (!render_data_.material_buffer.is_null())
      {
        gpu_system_->destroy_buffer(render_data_.material_buffer);
      }
      render_data_.material_buffer = gpu_system_->create_buffer(
        "Material Buffer"_str,
        {
          .size        = materials_.size_in_bytes(),
          .usage_flags = {gpu::BufferUsage::STORAGE},
          .queue_flags = {gpu::QueueType::GRAPHIC, gpu::QueueType::COMPUTE},
        },
        materials_.data());
      gpu_system_->flush_buffer(render_data_.material_buffer);
    }
  }

  void Scene::prepare_mesh_instance_buffer(NotNull<gpu::RenderGraph*> render_graph)
  {
    if (update_flags_.test(UpdateType::RENDERABLE_CHANGED))
    {
      if (!render_data_.mesh_instances_buffer.is_null())
      {
        gpu_system_->destroy_buffer(render_data_.mesh_instances_buffer);
      }
      render_data_.mesh_instances.clear();

      entity_manager_.for_each_component_with_entity_id<RenderComponent>(
        [this](const RenderComponent& render_component, EntityId entity_id)
        {
          const auto& mesh_group = mesh_groups_[render_component.mesh_group_id.id];
          for (u32 mesh_idx = 0; mesh_idx < mesh_group.meshes.size(); mesh_idx++)
          {
            const auto& mesh = mesh_group.meshes[mesh_idx];
            render_data_.mesh_instances.push_back(MeshInstance{
              .flags          = mesh.flags,
              .vb_offset      = mesh.vb_offset,
              .ib_offset      = mesh.ib_offset,
              .index_count    = mesh.index_count,
              .mesh_id        = (render_component.mesh_group_id.id << 16) | (mesh_idx),
              .material_index = mesh.material_id.id,
              .matrix_index   = static_cast<u32>(entity_manager_.get_internal_index(entity_id)),
            });
          };
        });

      render_data_.mesh_instances_buffer = gpu_system_->create_buffer(
        "Mesh instance buffer"_str,
        {
          .size        = render_data_.mesh_instances.size_in_bytes(),
          .usage_flags = {gpu::BufferUsage::STORAGE},
          .queue_flags = {gpu::QueueType::GRAPHIC, gpu::QueueType::COMPUTE},
        },
        render_data_.mesh_instances.data());
      gpu_system_->flush_buffer(render_data_.mesh_instances_buffer);
    }
  }

  void Scene::prepare_light_instance_buffer(NotNull<gpu::RenderGraph*> render_graph)
  {
    if (!render_data_.light_instance_buffer.is_null())
    {
      gpu_system_->destroy_buffer(render_data_.light_instance_buffer);
    }
    render_data_.light_instances.clear();

    entity_manager_.for_each_component_with_entity_id<LightComponent>(
      [this](const LightComponent& light_component, EntityId entity_id)
      {
        const auto& world_transform = entity_manager_.world_transform_ref(entity_id);
        const auto translation      = world_transform.col(3).xyz();
        const auto orientation      = world_transform.col(2).xyz();
        render_data_.light_instances.push_back(GPULightInstance{
          .radiation_type  = to_underlying(light_component.type),
          .position        = translation,
          .direction       = orientation,
          .intensity       = light_component.color * light_component.intensity,
          .cos_outer_angle = math::cos(light_component.outer_angle),
          .cos_inner_angle = math::cos(light_component.inner_angle),
        });
      });

    if (render_data_.light_instances.empty())
    {
      render_data_.light_instance_buffer = gpu::BufferID::Null();
      return;
    }

    render_data_.light_instance_buffer = gpu_system_->create_buffer(
      "Light instance buffer"_str,
      {
        .size        = render_data_.light_instances.size_in_bytes(),
        .usage_flags = {gpu::BufferUsage::STORAGE},
        .queue_flags = {gpu::QueueType::GRAPHIC, gpu::QueueType::COMPUTE},
      },
      render_data_.light_instances.data());
    gpu_system_->flush_buffer(render_data_.light_instance_buffer);
  }

  void Scene::prepare_draw_args(NotNull<gpu::RenderGraph*> render_graph)
  {
    if (update_flags_.test(UpdateType::RENDERABLE_CHANGED))
    {
      using IndirectCommandList = Vector<gpu::DrawIndexedIndirectCommand>;
      FlagMap<gpu::IndexType, IndirectCommandList> draw_commands;

      for (u32 mesh_instance_idx = 0; mesh_instance_idx < render_data_.mesh_instances.size();
           mesh_instance_idx++)
      {
        const auto& mesh_instance = render_data_.mesh_instances[mesh_instance_idx];
        const auto index_type     = mesh_instance.flags.test(MeshInstanceFlag::USE_16_BIT_INDICES)
                                      ? gpu::IndexType::UINT16
                                      : gpu::IndexType::UINT32;
        const b8 use_16_bits      = index_type == gpu::IndexType::UINT16;
        draw_commands[index_type].push_back(gpu::DrawIndexedIndirectCommand{
          .index_count    = mesh_instance.index_count,
          .instance_count = 1,
          .first_index    = mesh_instance.ib_offset * (use_16_bits ? 2 : 1),
          .vertex_offset  = cast<i32>(mesh_instance.vb_offset),
          .first_instance = mesh_instance_idx,
        });
      }

      for (const auto& draw_args : render_data_.draw_args_list)
      {
        gpu_system_->destroy_buffer(draw_args.buffer);
      }
      render_data_.draw_args_list.clear();

      for (const auto index_type : FlagIter<gpu::IndexType>())
      {

        if (draw_commands[index_type].empty())
        {
          continue;
        }

        const gpu::BufferID buffer = gpu_system_->create_buffer(
          "Indirect buffer"_str,
          {
            .size        = draw_commands[index_type].size_in_bytes(),
            .usage_flags = {gpu::BufferUsage::INDIRECT},
            .queue_flags = {gpu::QueueType::GRAPHIC},
          },
          draw_commands[index_type].data());
        gpu_system_->flush_buffer(buffer);

        render_data_.draw_args_list.push_back(DrawArgs{
          .buffer     = buffer,
          .count      = draw_commands[index_type].size(),
          .index_type = index_type,
        });
      }
    }
  }

  void Scene::prepare_gpu_scene(NotNull<gpu::RenderGraph*> render_graph)
  {
    const auto scene_buffer_node = render_graph->create_buffer(
      "GPU scene buffer"_str,
      {
        .size = sizeof(GPUScene),
      });

    struct Parameter
    {
      gpu::BufferNodeID scene_buffer;
    };

    const auto setup_fn = [scene_buffer_node](auto& parameter, auto& builder)
    {
      parameter.scene_buffer =
        builder.add_dst_buffer(scene_buffer_node, gpu::TransferDataSource::CPU);
    };

    const auto execute_fn = [this](const auto& parameter, auto& registry, auto& command_list)
    {
      gpu::SamplerID linear_clamp_sampler_id =
        gpu_system_->request_sampler(gpu::SamplerDesc::same_filter_wrap(
          gpu::TextureFilter::LINEAR, gpu::TextureWrap::CLAMP_TO_EDGE, true, 10.0f));

      gpu::SamplerID linear_repeat_sampler_id =
        gpu_system_->request_sampler(gpu::SamplerDesc::same_filter_wrap(
          gpu::TextureFilter::LINEAR, gpu::TextureWrap::REPEAT, true, 10.0f));

      gpu::SamplerID nearest_clamp_sapmpler_id = gpu_system_->request_sampler(
        gpu::SamplerDesc::same_filter_wrap(gpu::TextureFilter::NEAREST, gpu::TextureWrap::REPEAT));

      const auto gpu_scene = GPUScene{
        .world_matrixes_buffer =
          gpu_system_->get_ssbo_descriptor_id(render_data_.world_matrixes_buffer),
        .normal_matrixes_buffer =
          gpu_system_->get_ssbo_descriptor_id(render_data_.normal_matrixes_buffer),
        .prev_world_matrixes_buffer =
          gpu_system_->get_ssbo_descriptor_id(render_data_.prev_world_matrixes_buffer),
        .prev_normal_matrixes_buffer =
          gpu_system_->get_ssbo_descriptor_id(render_data_.prev_normal_matrixes_buffer),
        .mesh_instance_buffer =
          gpu_system_->get_ssbo_descriptor_id(render_data_.mesh_instances_buffer),
        .vertices        = gpu_system_->get_ssbo_descriptor_id(render_data_.static_vertex_buffer),
        .indexes         = gpu_system_->get_ssbo_descriptor_id(render_data_.index_buffer),
        .material_buffer = gpu_system_->get_ssbo_descriptor_id(render_data_.material_buffer),
        .env_map_data =
          {
            .descriptor_id = gpu_system_->get_srv_descriptor_id(env_map_.texture_id),
            .setting_data  = env_map_.setting_data,
          },
        .linear_repeat_sampler = gpu_system_->get_sampler_descriptor_id(linear_repeat_sampler_id),
        .linear_clamp_sampler  = gpu_system_->get_sampler_descriptor_id(linear_clamp_sampler_id),
        .nearest_clamp_sampler = gpu_system_->get_sampler_descriptor_id(nearest_clamp_sapmpler_id),
        .tlas                  = gpu_system_->get_as_descriptor_id(render_data_.tlas_id),
        .camera_data           = render_data_.current_camera_data,
        .prev_camera_data      = render_data_.prev_camera_data,
        .light_count           = render_data_.light_instances.size(),
        .light_instance_buffer =
          gpu_system_->get_ssbo_descriptor_id(render_data_.light_instance_buffer),
      };

      using Command = gpu::RenderCommandUpdateBuffer;

      const auto region_copy = gpu::BufferRegionCopy{
        .src_offset = 0,
        .dst_offset = 0,
        .size       = sizeof(GPUScene),
      };

      const auto command = Command{
        .dst_buffer = registry.get_buffer(parameter.scene_buffer),
        .data       = soul::cast<void*>(&gpu_scene),
        .regions    = {&region_copy, 1},
      };

      command_list.push(command);
    };

    const auto& node = render_graph->add_non_shader_pass<Parameter>(
      "GPUScene upload"_str, gpu::QueueType::TRANSFER, setup_fn, execute_fn);
    render_data_.scene_buffer_node = node.get_parameter().scene_buffer;
  }

  void Scene::prepare_blas(NotNull<gpu::RenderGraph*> render_graph)
  {

    runtime::ScopeAllocator scope_allocator("prepare blas"_str);
    if (!update_flags_.test(UpdateType::MESH_CHANGED))
    {
      if (!render_data_.blas_group_id.is_null())
      {
        render_data_.blas_group_node_id =
          render_graph->import_blas_group("Blas Group"_str, render_data_.blas_group_id);
      }
      return;
    }

    if (!render_data_.blas_group_id.is_null())
    {
      gpu_system_->destroy_blas_group(render_data_.blas_group_id);
    }
    render_data_.blas_ids.clear();

    render_data_.blas_group_id = gpu_system_->create_blas_group("Blas Group"_str);
    for (const auto& mesh_group : mesh_groups_)
    {
      const auto geometry_descs = Vector<gpu::RTGeometryDesc>::Transform(
        mesh_group.meshes,
        [](const Mesh& mesh) -> gpu::RTGeometryDesc
        {
          return {
            .type    = gpu::RTGeometryType::TRIANGLE,
            .content = {
              .triangles =
                {
                  .vertex_format = gpu::TextureFormat::RGB32F,
                  .vertex_stride = sizeof(StaticVertexData),
                  .vertex_count  = mesh.vertex_count,
                  .index_type    = mesh.get_gpu_index_type(),
                  .index_count   = mesh.index_count,
                },
            }};
        },
        scope_allocator);

      const gpu::BlasBuildDesc build_desc = {
        .geometry_count = soul::cast<u32>(geometry_descs.size()),
        .geometry_descs = geometry_descs.data(),
      };

      const auto blas_size = gpu_system_->get_blas_size_requirement(build_desc);

      render_data_.blas_ids.push_back(
        gpu_system_->create_blas("Unnamed"_str, {.size = blas_size}, render_data_.blas_group_id));
    }

    render_data_.blas_group_node_id =
      render_graph->import_blas_group("Blas Group"_str, render_data_.blas_group_id);

    struct BuildBlasParameter
    {
      gpu::BlasGroupNodeID blas_group_node_id;
    };

    const auto& build_pass = render_graph->add_non_shader_pass<BuildBlasParameter>(
      "Build blas group"_str,
      gpu::QueueType::COMPUTE,
      [&render_data = render_data_](auto& parameter, auto& builder)
      {
        parameter.blas_group_node_id = builder.add_as_build_dst(render_data.blas_group_node_id);
      },
      [this](const auto& parameter, const auto& registry, auto& command_list)
      {
        runtime::ScopeAllocator scope_allocator("build blas execute"_str);

        const auto geometry_desc_count = std::accumulate(
          mesh_groups_.begin(),
          mesh_groups_.end(),
          0LLU,
          [](u64 current_count, const MeshGroup& mesh_group)
          {
            return current_count + mesh_group.meshes.size();
          });

        auto geometry_descs =
          Vector<gpu::RTGeometryDesc>::WithCapacity(geometry_desc_count, scope_allocator);

        auto render_commands =
          Vector<gpu::RenderCommandBuildBlas>::WithCapacity(mesh_groups_.size(), scope_allocator);

        for (usize mesh_group_idx = 0; mesh_group_idx < mesh_groups_.size(); mesh_group_idx++)
        {
          const auto& mesh_group   = mesh_groups_[mesh_group_idx];
          const auto geometry_data = geometry_descs.end();
          for (const auto& mesh : mesh_group.meshes)
          {
            const auto vertex_data = gpu_system_->get_gpu_address(
              render_data_.static_vertex_buffer, mesh.vb_offset * sizeof(StaticVertexData));

            const auto index_data =
              gpu_system_->get_gpu_address(render_data_.index_buffer, mesh.ib_offset * sizeof(u32));

            geometry_descs.push_back(gpu::RTGeometryDesc{
              .type  = gpu::RTGeometryType::TRIANGLE,
              .flags = {gpu::RTGeometryFlag::OPAQUE},
              .content =
                {
                  .triangles =
                    {
                      .vertex_format = gpu::TextureFormat::RGB32F,
                      .vertex_data   = vertex_data,
                      .vertex_stride = sizeof(StaticVertexData),
                      .vertex_count  = mesh.vertex_count,
                      .index_type    = mesh.get_gpu_index_type(),
                      .index_data    = index_data,
                      .index_count   = mesh.index_count,
                    },
                },
            });
          }

          render_commands.push_back(gpu::RenderCommandBuildBlas{
            .src_blas_id = gpu::BlasID(),
            .dst_blas_id = render_data_.blas_ids[mesh_group_idx],
            .build_mode  = gpu::RTBuildMode::REBUILD,
            .build_desc  = {
               .flags          = {gpu::RTBuildFlag::PREFER_FAST_BUILD},
               .geometry_count = cast<u32>(mesh_group.meshes.size()),
               .geometry_descs = geometry_data,
            }});
        }

        static constexpr auto MAX_BLAS_BUILD_MEMORY = 1ull << 28;
        command_list.push(gpu::RenderCommandBatchBuildBlas{
          .builds                = u32cspan(render_commands.data(), render_commands.size()),
          .max_build_memory_size = MAX_BLAS_BUILD_MEMORY,
        });
      });
    render_data_.blas_group_node_id = build_pass.get_parameter().blas_group_node_id;
  }

  void Scene::prepare_tlas(NotNull<gpu::RenderGraph*> render_graph)
  {
    render_data_.rt_instance_descs.clear();
    if (!render_data_.tlas_id.is_null())
    {
      gpu_system_->destroy_tlas(render_data_.tlas_id);
    }

    if (entity_manager_.is_empty() || render_data_.mesh_instances.empty())
    {
      return;
    }

    // Upload instances pass
    gpu::BufferNodeID instance_buffer_node = render_graph->create_buffer(
      "Instance buffer"_str,
      {
        .size = sizeof(gpu::RTInstanceDesc) * render_data_.mesh_instances.size(),
      });

    struct UploadParameter
    {
      gpu::BufferNodeID instance_buffer;
    };

    const auto& upload_pass = render_graph->add_non_shader_pass<UploadParameter>(
      "Upload instance buffer"_str,
      gpu::QueueType::TRANSFER,

      // SetupFn
      [instance_buffer_node](auto& parameter, auto& builder)
      {
        parameter.instance_buffer = builder.add_dst_buffer(instance_buffer_node);
      },

      // ExecuteFn
      [this](const auto& parameter, const auto& registry, auto& command_list)
      {
        u32 instance_id = 0;
        entity_manager_.for_each_component_with_entity_id<RenderComponent>(
          [this, &instance_id](const RenderComponent& render_component, EntityId entity_id)
          {
            const auto blas_id = render_data_.blas_ids[render_component.mesh_group_id.id];
            render_data_.rt_instance_descs.push_back(gpu::RTInstanceDesc(
              entity_manager_.world_transform_ref(entity_id),
              instance_id,
              0xFF,
              0,
              gpu::RTGeometryInstanceFlags{
                gpu::RTGeometryInstanceFlag::TRIANGLE_FACING_CULL_DISABLE},
              gpu_system_->get_gpu_address(blas_id)));
            instance_id += mesh_groups_[render_component.mesh_group_id.id].meshes.size();
          });

        const gpu::BufferID instance_buffer = registry.get_buffer(parameter.instance_buffer);

        const gpu::BufferRegionCopy region = {
          .src_offset = 0,
          .dst_offset = 0,
          .size       = render_data_.rt_instance_descs.size_in_bytes(),
        };

        command_list.push(gpu::RenderCommandUpdateBuffer{
          .dst_buffer = instance_buffer,
          .data       = render_data_.rt_instance_descs.data(),
          .regions    = u32cspan(&region, 1),
        });
      });

    render_data_.rt_instances_node_id = upload_pass.get_parameter().instance_buffer;

    // Build Tlas Pass
    const auto tlas_size = gpu_system_->get_tlas_size_requirement({
      .build_flags    = {gpu::RTBuildFlag::PREFER_FAST_BUILD},
      .instance_count = soul::cast<u32>(render_data_.mesh_instances.size()),
    });

    render_data_.tlas_id = gpu_system_->create_tlas(
      "Scene Tlas"_str,
      {
        .size = tlas_size,
      });

    render_data_.tlas_node_id = render_graph->import_tlas("Tlas"_str, render_data_.tlas_id);

    struct BuildParameter
    {
      gpu::BlasGroupNodeID blas_group_node_id;
      gpu::TlasNodeID tlas_node_id;
      gpu::BufferNodeID instance_buffer;
    };

    const auto& build_pass = render_graph->add_non_shader_pass<BuildParameter>(
      "Build Tlas"_str,
      gpu::QueueType::COMPUTE,

      // SetupFn:
      [this](BuildParameter& parameter, auto& builder)
      {
        parameter.blas_group_node_id = builder.add_as_build_input(render_data_.blas_group_node_id);
        parameter.tlas_node_id       = builder.add_as_build_dst(render_data_.tlas_node_id);
        parameter.instance_buffer = builder.add_as_build_input(render_data_.rt_instances_node_id);
      },

      // ExecuteFn:
      [this](const BuildParameter& parameter, const auto& registry, auto& command_list)
      {
        const gpu::BufferID instance_buffer = registry.get_buffer(parameter.instance_buffer);

        command_list.push(gpu::RenderCommandBuildTlas{
          .tlas_id    = registry.get_tlas(parameter.tlas_node_id),
          .build_desc = {
            .build_flags    = {gpu::RTBuildFlag::PREFER_FAST_BUILD},
            .geometry_flags = {gpu::RTGeometryFlag::OPAQUE},
            .instance_data  = gpu_system_->get_gpu_address(instance_buffer),
            .instance_count = soul::cast<u32>(render_data_.rt_instance_descs.size()),
          }});
      });

    render_data_.tlas_node_id = build_pass.get_parameter().tlas_node_id;
  }

  void Scene::prepare_render_data(NotNull<gpu::RenderGraph*> render_graph)
  {
    if (render_data_.num_frames == 0)
    {
      render_data_.prev_camera_data = get_render_camera_data();
    } else
    {
      render_data_.prev_camera_data = render_data_.current_camera_data;
    }
    render_data_.scene_aabb          = get_scene_aabb();
    render_data_.current_camera_data = get_render_camera_data();
    render_data_.num_frames++;
    prepare_world_matrixes_buffer_node(render_graph);
    prepare_normal_matrixes_buffer_node(render_graph);
    prepare_geometry_buffer(render_graph);
    prepare_material_buffer(render_graph);
    prepare_mesh_instance_buffer(render_graph);
    prepare_light_instance_buffer(render_graph);
    prepare_draw_args(render_graph);
    prepare_blas(render_graph);
    prepare_tlas(render_graph);
    prepare_gpu_scene(render_graph);
    update_flags_.reset();
  }

  auto Scene::build_scene_dependencies(
    NotNull<gpu::RGRasterDependencyBuilder*> dependency_builder) const -> gpu::BufferNodeID
  {

    if (!render_data_.world_matrixes_buffer.is_null())
    {
      dependency_builder->add_shader_buffer(
        render_data_.world_matrixes_buffer_node,
        {
          gpu::ShaderStage::VERTEX,
          gpu::ShaderStage::FRAGMENT,
        },
        gpu::ShaderBufferReadUsage::STORAGE);

      dependency_builder->add_shader_buffer(
        render_data_.normal_matrixes_buffer_node,
        {
          gpu::ShaderStage::VERTEX,
          gpu::ShaderStage::FRAGMENT,
        },
        gpu::ShaderBufferReadUsage::STORAGE);
    }

    return dependency_builder->add_shader_buffer(
      render_data_.scene_buffer_node,
      {
        gpu::ShaderStage::VERTEX,
        gpu::ShaderStage::FRAGMENT,
      },
      gpu::ShaderBufferReadUsage::STORAGE);
  }

  auto Scene::build_scene_dependencies(
    NotNull<gpu::RGComputeDependencyBuilder*> dependency_builder) const -> gpu::BufferNodeID
  {

    if (!render_data_.world_matrixes_buffer.is_null())
    {
      dependency_builder->add_shader_buffer(
        render_data_.world_matrixes_buffer_node,
        {
          gpu::ShaderStage::COMPUTE,
        },
        gpu::ShaderBufferReadUsage::STORAGE);

      dependency_builder->add_shader_buffer(
        render_data_.normal_matrixes_buffer_node,
        {
          gpu::ShaderStage::COMPUTE,
        },
        gpu::ShaderBufferReadUsage::STORAGE);
    }

    return dependency_builder->add_shader_buffer(
      render_data_.scene_buffer_node,
      {
        gpu::ShaderStage::COMPUTE,
      },
      gpu::ShaderBufferReadUsage::STORAGE);
  }

  auto Scene::build_scene_dependencies(
    NotNull<gpu::RGRayTracingDependencyBuilder*> dependency_builder) const -> gpu::BufferNodeID
  {

    if (!render_data_.world_matrixes_buffer.is_null())
    {
      dependency_builder->add_shader_buffer(
        render_data_.world_matrixes_buffer_node,
        gpu::SHADER_STAGES_RAY_TRACING,
        gpu::ShaderBufferReadUsage::STORAGE);

      dependency_builder->add_shader_buffer(
        render_data_.normal_matrixes_buffer_node,
        gpu::SHADER_STAGES_RAY_TRACING,
        gpu::ShaderBufferReadUsage::STORAGE);
    }

    return dependency_builder->add_shader_buffer(
      render_data_.scene_buffer_node,
      gpu::SHADER_STAGES_RAY_TRACING,
      gpu::ShaderBufferReadUsage::STORAGE);
  }

  void Scene::build_rasterize_dependencies(NotNull<gpu::RGRasterDependencyBuilder*> builder) const
  {
  }

  void Scene::rasterize(
    const RasterizeDesc& desc,
    NotNull<gpu::RenderGraphRegistry*> registry,
    NotNull<gpu::RasterCommandList*> command_list) const
  {
    if (render_data_.draw_args_list.empty())
    {
      return;
    }

    const gpu::GraphicPipelineStateDesc pipeline_desc = {
      .program_id = desc.program_id,
      .input_bindings =
        {
          .list = {{.stride = sizeof(StaticVertexData)}},
        },
      .input_attributes =
        {
          .list =
            {
              {
                .binding = 0,
                .offset  = offsetof(StaticVertexData, position),
                .type    = gpu::VertexElementType::FLOAT3,
              },
              {
                .binding = 0,
                .offset  = offsetof(StaticVertexData, normal),
                .type    = gpu::VertexElementType::FLOAT3,
              },
              {
                .binding = 0,
                .offset  = offsetof(StaticVertexData, tangent),
                .type    = gpu::VertexElementType::FLOAT4,
              },
              {
                .binding = 0,
                .offset  = offsetof(StaticVertexData, tex_coord),
                .type    = gpu::VertexElementType::FLOAT2,
              },
            },
        },
      .viewport = desc.viewport,
      .scissor  = desc.scissor,
      .raster =
        {
          .cull_mode  = {gpu::CullMode::BACK},
          .front_face = gpu::FrontFace::COUNTER_CLOCKWISE,
        },
      .color_attachment_count   = desc.color_attachment_count,
      .color_attachments        = desc.color_attachments,
      .depth_stencil_attachment = desc.depth_stencil_attachment,
    };
    const auto pipeline_state_id = registry->get_pipeline_state(pipeline_desc);

    for (const auto& draw_arg : render_data_.draw_args_list)
    {
      command_list->push(gpu::RenderCommandDrawIndexedIndirect{
        .pipeline_state_id  = pipeline_state_id,
        .push_constant_data = desc.push_constant_data,
        .push_constant_size = desc.push_constant_size,
        .vertex_buffer_ids  = {render_data_.static_vertex_buffer},
        .index_buffer_id    = render_data_.index_buffer,
        .index_type         = draw_arg.index_type,
        .buffer_id          = draw_arg.buffer,
        .offset             = 0,
        .draw_count         = cast<u32>(draw_arg.count),
        .stride             = sizeof(gpu::DrawIndexedIndirectCommand),
      });
    }
  }

  void Scene::RenderData::rasterize(
    const RasterizeDesc& desc,
    NotNull<gpu::RenderGraphRegistry*> registry,
    NotNull<gpu::RasterCommandList*> command_list) const
  {
    if (draw_args_list.empty())
    {
      return;
    }

    const gpu::GraphicPipelineStateDesc pipeline_desc = {
      .program_id = desc.program_id,
      .input_bindings =
        {
          .list = {{.stride = sizeof(StaticVertexData)}},
        },
      .input_attributes =
        {
          .list =
            {
              {
                .binding = 0,
                .offset  = offsetof(StaticVertexData, position),
                .type    = gpu::VertexElementType::FLOAT3,
              },
              {
                .binding = 0,
                .offset  = offsetof(StaticVertexData, normal),
                .type    = gpu::VertexElementType::FLOAT3,
              },
              {
                .binding = 0,
                .offset  = offsetof(StaticVertexData, tangent),
                .type    = gpu::VertexElementType::FLOAT4,
              },
              {
                .binding = 0,
                .offset  = offsetof(StaticVertexData, tex_coord),
                .type    = gpu::VertexElementType::FLOAT2,
              },
            },
        },
      .viewport = desc.viewport,
      .scissor  = desc.scissor,
      .raster =
        {
          .cull_mode  = {gpu::CullMode::BACK},
          .front_face = gpu::FrontFace::COUNTER_CLOCKWISE,
        },
      .color_attachment_count   = desc.color_attachment_count,
      .color_attachments        = desc.color_attachments,
      .depth_stencil_attachment = desc.depth_stencil_attachment,
    };
    const auto pipeline_state_id = registry->get_pipeline_state(pipeline_desc);

    for (const auto& draw_arg : draw_args_list)
    {
      command_list->push(gpu::RenderCommandDrawIndexedIndirect{
        .pipeline_state_id  = pipeline_state_id,
        .push_constant_data = desc.push_constant_data,
        .push_constant_size = desc.push_constant_size,
        .vertex_buffer_ids  = {static_vertex_buffer},
        .index_buffer_id    = index_buffer,
        .index_type         = draw_arg.index_type,
        .buffer_id          = draw_arg.buffer,
        .offset             = 0,
        .draw_count         = cast<u32>(draw_arg.count),
        .stride             = sizeof(gpu::DrawIndexedIndirectCommand),
      });
    }
  }
} // namespace renderlab
