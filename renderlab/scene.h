#pragma once

#include "core/vector.h"
#include "gpu/render_graph.h"
#include "gpu/render_graph_registry.h"
#include "gpu/sl_type.h"
#include "gpu/type.h"

#include "ecs.h"
#include "math/aabb.h"
#include "scene.hlsl"
#include "type.h"
#include "type.shared.hlsl"

using namespace soul;

namespace renderlab
{
  class Scene
  {
  public:
    enum class UpdateType : u32
    {
      MATERIAL_CHANGED,
      MESH_CHANGED,
      ENTITY_CHANGED,
      RENDERABLE_CHANGED,
      LIGHT_CHANGED,
      COUNT
    };
    using UpdateFlags = FlagSet<UpdateType>;

    struct RasterizeDesc
    {
      const void* push_constant_data = nullptr;
      u32 push_constant_size         = 0;
      gpu::ProgramID program_id;
      gpu::Viewport viewport;
      gpu::Rect2D scissor;
      u8 color_attachment_count = 0;
      Array<gpu::ColorAttachmentDesc, gpu::MAX_COLOR_ATTACHMENT_PER_SHADER> color_attachments;
      gpu::DepthStencilAttachmentDesc depth_stencil_attachment;
      gpu::DepthBiasDesc depth_bias_desc;
    };

    struct DrawArgs
    {
      gpu::BufferID buffer;
      usize count               = 0;
      gpu::IndexType index_type = gpu::IndexType::UINT16;
    };

    struct RenderData
    {
      math::AABB scene_aabb;
      GPUCameraData prev_camera_data;
      GPUCameraData current_camera_data;

      gpu::BufferID world_matrixes_buffer;
      gpu::BufferNodeID world_matrixes_buffer_node;

      gpu::BufferID prev_world_matrixes_buffer;
      gpu::BufferNodeID prev_world_matrixes_buffer_node;

      gpu::BufferID normal_matrixes_buffer;
      gpu::BufferNodeID normal_matrixes_buffer_node;

      gpu::BufferID prev_normal_matrixes_buffer;
      gpu::BufferNodeID prev_normal_matrixes_buffer_node;

      gpu::BufferID static_vertex_buffer;
      gpu::BufferID index_buffer;

      gpu::BufferID material_buffer;

      Vector<MeshInstance> mesh_instances;
      gpu::BufferID mesh_instances_buffer;

      Vector<GPULightInstance> light_instances;
      gpu::BufferID light_instance_buffer;

      Vector<DrawArgs> draw_args_list;

      Vector<gpu::BlasID> blas_ids;
      gpu::BlasGroupID blas_group_id;
      gpu::BlasGroupNodeID blas_group_node_id;

      Vector<gpu::RTInstanceDesc> rt_instance_descs;
      gpu::BufferNodeID rt_instances_node_id;
      gpu::TlasID tlas_id;
      gpu::TlasNodeID tlas_node_id;

      gpu::BufferNodeID scene_buffer_node;

      usize num_frames = 0;
    };

  private:
    NotNull<gpu::System*> gpu_system_;

    EntityManager<RenderComponent, LightComponent, CameraComponent> entity_manager_;
    EntityId render_camera_;

    Array<vec2f32, 16> jitter_samples_;
    vec2u32 viewport_ = {1920, 1080};

    Vector<StaticVertexData> vertices_ = Vector<StaticVertexData>::WithCapacity(100000);
    Vector<u32> indexes_               = Vector<u32>::WithCapacity(100000);
    Vector<MeshGroup> mesh_groups_;

    Vector<String> material_names_;
    Vector<Material> materials_;
    Vector<gpu::TextureID> material_textures_;
    MaterialTextureID default_material_texture_;

    UpdateFlags update_flags_ = {};

    EnvMap env_map_;

    RenderSetting render_setting_;

    RenderData render_data_;

  public:
    [[nodiscard]]
    static auto Create(NotNull<gpu::System*> gpu_system) -> Scene;

    auto create_material_texture(const MaterialTextureDesc& desc) -> MaterialTextureID;

    auto create_material(const MaterialDesc& desc) -> MaterialID;

    auto material_ref(MaterialID material_id) -> Material&;

    auto create_mesh_group(const MeshGroupDesc& desc) -> MeshGroupID;

    auto create_entity(const EntityDesc& desc) -> EntityId;

    auto create_camera_entity(const CameraEntityDesc& desc) -> EntityId;

    [[nodiscard]]
    auto get_root_entity_id() const -> EntityId;

    [[nodiscard]]
    auto get_entity_first_child(EntityId entity_id) const -> EntityId;

    [[nodiscard]]
    auto get_entity_next_sibling(EntityId entity_id) const -> EntityId;

    [[nodiscard]]
    auto get_entity_name(EntityId entity_id) const -> StringView;

    [[nodiscard]]
    auto entity_world_transform_ref(EntityId entity_id) const -> const mat4f32&;

    void set_world_transform(EntityId entity_id, const mat4f32& world_transform);

    [[nodiscard]]
    auto entity_local_transform_ref(EntityId entity_id) const -> const mat4f32&;

    void set_local_transform(EntityId entity_id, const mat4f32& local_transform);

    void add_render_component(EntityId entity_id, const RenderComponent& render_component);

    void add_light_component(EntityId entity_id, const LightComponent& light_component);

    void set_light_component(EntityId entity_id, const LightComponent& light_component);

    [[nodiscard]]
    auto try_get_light_component(EntityId entity_id) const -> MaybeNull<const LightComponent*>;

    void add_camera_component(EntityId entity_id, const CameraComponent& camera_component);

    void set_camera_component(EntityId entity_id, const CameraComponent& camera_component);

    [[nodiscard]]
    auto get_render_camera_entity_id() const -> EntityId
    {
      return render_camera_;
    }

    void set_viewport(vec2u32 viewport)
    {
      viewport_ = viewport;
    }

    auto get_env_map_setting() -> EnvMapSetting;

    void set_env_map_setting(const EnvMapSetting& env_map_setting);

    auto get_render_setting() -> RenderSetting;

    void set_render_setting(const RenderSetting& render_setting);

    [[nodiscard]]
    auto get_gpu_system() const -> NotNull<gpu::System*>
    {
      return gpu_system_;
    }

    [[nodiscard]]
    auto get_viewport() const -> vec2u32
    {
      return viewport_;
    }

    [[nodiscard]]
    auto get_render_camera_data() const -> GPUCameraData;

    [[nodiscard]]
    auto get_scene_aabb() const -> math::AABB;

    [[nodiscard]]
    auto is_empty() const -> b8;

    void prepare_world_matrixes_buffer_node(NotNull<gpu::RenderGraph*> render_graph);

    void prepare_normal_matrixes_buffer_node(NotNull<gpu::RenderGraph*> render_graph);

    void prepare_geometry_buffer(NotNull<gpu::RenderGraph*> render_graph);

    void prepare_material_buffer(NotNull<gpu::RenderGraph*> render_graph);

    void prepare_mesh_instance_buffer(NotNull<gpu::RenderGraph*> render_graph);

    void prepare_draw_args(NotNull<gpu::RenderGraph*> render_graph);

    void prepare_light_instance_buffer(NotNull<gpu::RenderGraph*> render_graph);

    void prepare_gpu_scene(NotNull<gpu::RenderGraph*> render_graph);

    void prepare_blas(NotNull<gpu::RenderGraph*> render_graph);

    void prepare_tlas(NotNull<gpu::RenderGraph*> render_graph);

    void prepare_render_data(NotNull<gpu::RenderGraph*> render_graph);

    [[nodiscard]]
    auto render_data_cref() const -> const RenderData&
    {
      return render_data_;
    }

    [[nodiscard]]
    auto build_scene_dependencies(NotNull<gpu::RGRasterDependencyBuilder*> builder) const
      -> gpu::BufferNodeID;

    [[nodiscard]]
    auto build_scene_dependencies(NotNull<gpu::RGComputeDependencyBuilder*> builder) const
      -> gpu::BufferNodeID;

    [[nodiscard]]
    auto build_scene_dependencies(NotNull<gpu::RGRayTracingDependencyBuilder*> builder) const
      -> gpu::BufferNodeID;

    void build_rasterize_dependencies(NotNull<gpu::RGRasterDependencyBuilder*> builder) const;

    void rasterize(
      const RasterizeDesc& raster_desc,
      NotNull<gpu::RenderGraphRegistry*> registry,
      NotNull<gpu::RasterCommandList*> command_list) const;

    explicit Scene(NotNull<gpu::System*> gpu_system);
  };
} // namespace renderlab
