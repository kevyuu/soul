
#include "editor/store.h"

#include "core/compiler.h"
#include "core/matrix.h"
#include "importer/gltf_importer.h"
#include "render_node.h"
#include "render_nodes/ddgi/ddgi_node.h"
#include "render_nodes/deferred_shading/deferred_shading_node.h"
#include "render_nodes/gbuffer_generate/gbuffer_generate_node.h"
#include "render_nodes/render_constant_name.h"
#include "render_nodes/rt_reflection/rt_reflection_node.h"
#include "render_nodes/rtao/rtao_node.h"
#include "render_nodes/shadow/shadow_node.h"
#include "render_nodes/taa/taa_node.h"
#include "render_nodes/tone_map/tone_map_node.h"

#include "camera_controller.h"
#include "ecs.h"
#include "hybrid_render_pipeline.h"
#include "render_pipeline.h"
#include "scene.h"
#include "type.h"
#include "type.shared.hlsl"

namespace renderlab
{
  static constexpr CompStr gbuffer_node_name          = "GBuffer Generation Node"_str;
  static constexpr CompStr deferred_shading_node_name = "Deferred Shading Node"_str;
  static constexpr CompStr shadow_node_name           = "Shadow Node"_str;
  static constexpr CompStr rtao_node_name             = "Rtao Node"_str;
  static constexpr CompStr ddgi_node_name             = "Ddgi Node"_str;
  static constexpr CompStr rt_reflection_node_name    = "Rt Reflection Node"_str;
  static constexpr CompStr taa_node_name              = "Taa Node"_str;
  static constexpr CompStr tone_map_node_name         = "Tone Map Node"_str;

  EditorStore::EditorStore(NotNull<Scene*> scene)
      : scene_(scene),
        render_pipeline_(HybridRenderPipeline::Create(scene)),
        active_entity_id_(EntityId::Null())
  {
  }

  void EditorStore::import(const Path& path)
  {
    GLTFImporter importer;
    importer.import(path, scene_);
  }

  auto EditorStore::scene_ref() const -> const Scene&
  {
    return *scene_;
  }

  void EditorStore::set_world_transform(EntityId entity_id, const mat4f32& world_transform)
  {
    scene_->set_world_transform(entity_id, world_transform);
  }

  void EditorStore::set_local_transform(EntityId entity_id, const mat4f32& local_transform)
  {
    scene_->set_local_transform(entity_id, local_transform);
  }

  auto EditorStore::active_render_pipeline_ref() -> RenderPipeline&
  {
    return render_pipeline_;
  }

  auto EditorStore::active_render_pipeline_ref() const -> const RenderPipeline&
  {
    return render_pipeline_;
  }

  auto EditorStore::get_render_output() const -> gpu::TextureNodeID
  {
    return render_pipeline_.get_output();
  }

  void EditorStore::select_entity(EntityId entity_id)
  {
    active_entity_id_ = entity_id;
  }

  auto EditorStore::get_selected_entity() const -> EntityId
  {
    return active_entity_id_;
  }

  void EditorStore::create_entity()
  {
    scene_->create_entity(EntityDesc{
      .name            = "Entity"_str,
      .local_transform = mat4f32::Identity(),
      .parent_entity_id =
        active_entity_id_.is_null() ? scene_->get_root_entity_id() : active_entity_id_,
    });
  }

  void EditorStore::create_light_entity(LightRadiationType radiation_type)
  {
    const auto entity_id       = scene_->create_entity(EntityDesc{
            .name            = "Light"_str,
            .local_transform = mat4f32::Identity(),
            .parent_entity_id =
        active_entity_id_.is_null() ? scene_->get_root_entity_id() : active_entity_id_,
    });
    const auto light_component = [&]()
    {
      switch (radiation_type)
      {
      case LightRadiationType::POINT:
        return LightComponent::Point({1.0_f32, 1.0_f32, 1.0_f32}, 100.0_f32);
      case LightRadiationType::SPOT:
        return LightComponent::Spot({1.0_f32, 1.0_f32, 1.0_f32}, 100.0_f32, 45.0_f32, 60.0_f32);
      case LightRadiationType::DIRECTIONAL:
        return LightComponent::Directional({1.0_f32, 1.0_f32, 1.0_f32}, 100.0_f32);
      case LightRadiationType::COUNT: unreachable();
      }
    }();
    scene_->add_light_component(entity_id, light_component);
  }

  void EditorStore::set_light_component(EntityId entity_id, const LightComponent& light_comp)
  {
    scene_->set_light_component(entity_id, light_comp);
  }

  auto EditorStore::get_env_map_setting() -> EnvMapSetting
  {
    return scene_->get_env_map_setting();
  }

  void EditorStore::set_env_map_setting(const EnvMapSetting& env_map_setting)
  {
    scene_->set_env_map_setting(env_map_setting);
  }

  auto EditorStore::get_render_setting() -> RenderSetting
  {
    return scene_->get_render_setting();
  }

  void EditorStore::set_render_setting(const RenderSetting& render_setting)
  {
    scene_->set_render_setting(render_setting);
  }
} // namespace renderlab
