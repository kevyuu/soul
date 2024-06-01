#pragma once

#include "ecs.h"
#include "gpu/render_graph.h"

#include "render_pipeline.h"
#include "type.h"

using namespace soul;

namespace renderlab
{
  class Scene;
  class OrbitCameraController;

  struct EditorIcons
  {
    gpu::TextureID gear;
    gpu::TextureID search;
  };

  class EditorStore
  {
  private:
    NotNull<Scene*> scene_;
    RenderPipeline render_pipeline_;
    EntityId active_entity_id_;
    EditorIcons icons_;

  public:
    explicit EditorStore(NotNull<Scene*> scene);

    void import(const Path& path);

    [[nodiscard]]
    auto scene_ref() const -> const Scene&;

    void set_world_transform(EntityId entity_id, const mat4f32& world_transform);

    void set_local_transform(EntityId entity_id, const mat4f32& local_transform);

    [[nodiscard]]
    auto entity_name_ref() -> String&;

    [[nodiscard]]
    auto active_render_pipeline_ref() -> RenderPipeline&;

    [[nodiscard]]
    auto active_render_pipeline_ref() const -> const RenderPipeline&;

    [[nodiscard]]
    auto get_render_output() const -> gpu::TextureNodeID;

    void select_entity(EntityId entity_id);

    [[nodiscard]]
    auto get_selected_entity() const -> EntityId;

    void create_entity();

    void create_light_entity(LightRadiationType radation_type);

    void set_light_component(EntityId entity_id, const LightComponent& light_component);

    auto get_env_map_setting() -> EnvMapSetting;

    void set_env_map_setting(const EnvMapSetting& env_map_setting);

    auto get_render_setting() -> RenderSetting;

    void set_render_setting(const RenderSetting& render_setting);
  };

} // namespace renderlab
