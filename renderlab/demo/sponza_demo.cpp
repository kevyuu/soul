#include "sponza_demo.h"

#include "importer/gltf_importer.h"
#include "math/matrix.h"
#include "math/quaternion.h"
#include "type.h"

namespace renderlab
{
  void SponzaDemo::load_scene(NotNull<Scene*> scene)
  {
    GLTFImporter importer;
    const auto interior_path = Path::From("C:/Users/kevin/Dev/asset/AIUE_vol1_06.glb"_str);
    const auto sponza_path   = Path::From("C:/Users/kevin/Dev/asset/sponza.glb"_str);

    importer.import(interior_path, scene);
    light_entity_id_    = scene->create_entity(EntityDesc{
         .name            = "Light"_str,
         .local_transform = mat4f32::Identity(),
         .parent_entity_id =
        light_entity_id_.is_null() ? scene->get_root_entity_id() : light_entity_id_,
    });
    vec3f32 light_color = vec3f32(218.0 / 255.0f, 210.0f / 255.0f, 167.0f / 255.0f);
    scene->add_light_component(
      light_entity_id_, LightComponent::Directional({light_color}, 50.0_f32));
    const auto light_quat = math::quat_euler_angles(vec3f32(1.237, 1.088, -2.824));
    const auto light_transform_mat =
      math::compose_transform(vec3f32(1.0_f32), light_quat, vec3f32(1.0_f32));
    scene->set_world_transform(light_entity_id_, light_transform_mat);

    const auto camera_quat = math::quat_euler_angles(vec3f32(0.049f, 1.180f, 0.0f));
    const auto camera_transform_mat =
      math::compose_transform(vec3f32(9.944_f32, 0.979f, 0.963f), camera_quat, vec3f32(1.0));
    const auto camera_entity_id = scene->get_render_camera_entity_id();
    scene->set_world_transform(camera_entity_id, camera_transform_mat);
  }
} // namespace renderlab
