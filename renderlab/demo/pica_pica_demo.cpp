#include "pica_pica_demo.h"

#include "importer/gltf_importer.h"
#include "math/matrix.h"
#include "math/quaternion.h"
#include "type.h"

namespace renderlab
{
  void PicaPicaDemo::load_scene(NotNull<Scene*> scene)
  {
    GLTFImporter importer;
    const auto pica_pica_path = Path::From("resources/gltf/pica_pica.gltf"_str);
    importer.import(pica_pica_path, scene);
    light_entity_id_ = scene->create_entity(EntityDesc{
      .name            = "Light"_str,
      .local_transform = mat4f32::Identity(),
      .parent_entity_id =
        light_entity_id_.is_null() ? scene->get_root_entity_id() : light_entity_id_,
    });
    scene->add_light_component(
      light_entity_id_, LightComponent::Directional({1.0_f32, 1.0_f32, 1.0_f32}, 1.0_f32));
    const auto light_quat = math::quat_euler_angles(vec3f32(-1.429, -0.0, 0.269));
    const auto light_transform_mat =
      math::compose_transform(vec3f32(1.0_f32), light_quat, vec3f32(1.0_f32));
    scene->set_world_transform(light_entity_id_, light_transform_mat);
  }
} // namespace renderlab
