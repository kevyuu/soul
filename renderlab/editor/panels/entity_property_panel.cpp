#include "entity_property_panel.h"

#include "core/flag_map.h"
#include "ecs.h"
#include "editor/store.h"

#include "app/gui.h"
#include "app/icons.h"
#include "math/math.h"
#include "math/matrix.h"
#include "math/quaternion.h"
#include "type.shared.hlsl"

#include <imgui/imgui.h>

namespace renderlab
{
  EntityPropertyPanel::EntityPropertyPanel(NotNull<EditorStore*> store) : store_(store) {}

  void EntityPropertyPanel::on_gui_render(NotNull<app::Gui*> gui)
  {
    if (gui->begin_window(
          LABEL,
          vec2f32(1400, 1040),
          vec2f32(20, 40),
          {
            app::Gui::WindowFlag::SHOW_TITLE_BAR,
            app::Gui::WindowFlag::ALLOW_MOVE,
            app::Gui::WindowFlag::NO_SCROLLBAR,
          }))
    {
      gui->button(ICON_MD_SEARCH);
      const auto active_entity_id = store_->get_selected_entity();
      if (!active_entity_id.is_null())
      {
        gui->label_text("Name"_str, store_->scene_ref().get_entity_name(active_entity_id));
        if (gui->collapsing_header("Local Transform"_str))
        {
          auto local_transform =
            math::into_transform(store_->scene_ref().entity_local_transform_ref(active_entity_id));
          auto local_euler_angles      = math::into_euler_angles(local_transform.rotation);
          b8 is_local_transform_change = false;
          is_local_transform_change |=
            gui->input_vec3f32("Position##local"_str, &local_transform.position);
          is_local_transform_change |=
            gui->input_vec3f32("Rotation##local"_str, &local_euler_angles);
          is_local_transform_change |=
            gui->input_vec3f32("Scale##local"_str, &local_transform.scale);
          if (is_local_transform_change)
          {
            store_->set_local_transform(
              active_entity_id,
              math::compose_transform(
                local_transform.position,
                math::quat_euler_angles(local_euler_angles),
                local_transform.scale));
          }
        }
        if (gui->collapsing_header("World Transform"_str))
        {
          auto world_transform_mat =
            store_->scene_ref().entity_world_transform_ref(active_entity_id);
          auto world_transform = math::into_transform(world_transform_mat);

          auto world_euler_angles = math::into_euler_angles(world_transform.rotation);

          b8 is_world_transform_change = false;
          is_world_transform_change |=
            gui->input_vec3f32("Position##world"_str, &world_transform.position);
          is_world_transform_change |=
            gui->input_vec3f32("Rotation##world"_str, &world_euler_angles);
          is_world_transform_change |=
            gui->input_vec3f32("Scale##world"_str, &world_transform.scale);
          if (is_world_transform_change)
          {
            const auto new_quat = math::quat_euler_angles(world_euler_angles);
            const auto new_world_transform_mat =
              math::compose_transform(world_transform.position, new_quat, world_transform.scale);

            auto new_world_transform    = math::into_transform(new_world_transform_mat);
            auto new_world_euler_angles = math::into_euler_angles(new_world_transform.rotation);

            store_->set_world_transform(active_entity_id, new_world_transform_mat);
          }
        }

        const MaybeNull<const LightComponent*> light_comp_opt =
          store_->scene_ref().try_get_light_component(active_entity_id);
        if (light_comp_opt.is_some())
        {
          if (gui->collapsing_header("Light"_str))
          {
            LightComponent light_component = *light_comp_opt.some_ref();
            static constexpr FlagMap<LightRadiationType, CompStr> light_radiation_label = {
              "Point"_str,
              "Directional"_str,
              "Spot"_str,
            };
            b8 is_light_component_change = false;
            if (gui->begin_combo("Light Type"_str, light_radiation_label[light_component.type]))
            {
              for (const auto light_type : FlagIter<LightRadiationType>())
              {
                const b8 is_selected = (light_component.type == light_type);
                if (gui->selectable(light_radiation_label[light_type], is_selected))
                {
                  is_light_component_change = true;
                  light_component.type      = light_type;
                }
                if (is_selected)
                {
                  gui->set_item_default_focus();
                }
              }
              gui->end_combo();
            }

            is_light_component_change |= gui->color_edit3("Color"_str, &light_component.color);
            is_light_component_change |=
              gui->input_f32("Intensity"_str, &light_component.intensity);

            if (light_component.type == LightRadiationType::SPOT)
            {
              is_light_component_change |=
                gui->input_f32("Outer Angle"_str, &light_component.outer_angle);
              is_light_component_change |=
                gui->input_f32("Inner Angle"_str, &light_component.inner_angle);
            }

            if (is_light_component_change)
            {
              store_->set_light_component(active_entity_id, light_component);
            }
          }
        }
      }
    }
    gui->end_window();
  }
} // namespace renderlab
