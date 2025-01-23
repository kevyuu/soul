#include "editor/panels/viewport_panel.h"
#include "app/input_state.h"
#include "camera_controller.h"
#include "core/matrix.h"
#include "core/type.h"
#include "editor/store.h"

#include "app/gui.h"
#include "gpu/render_graph.h"
#include "type.shared.hlsl"
#include <imgui/imgui.h>

#include <algorithm>

namespace renderlab
{
  ViewportPanel::ViewportPanel(NotNull<EditorStore*> store)
      : store_(store), gizmo_op_(soul::app::Gui::GizmoOp::TRANSLATE)
  {
  }

  void ViewportPanel::on_gui_render(NotNull<app::Gui*> gui)
  {
    if (gui->begin_window(
          LABEL,
          vec2f32{1900, 1040},
          vec2f32(20, 40),
          {
            app::Gui::WindowFlag::NO_SCROLLBAR,
          }))
    {

      gui->text(String::Format("FPS : {}", gui->get_frame_rate()).cview());

      const vec2f32 window_size = gui->get_window_size();
      const auto scene_viewport = store_->scene_ref().get_viewport();
      f32 aspect_ratio          = f32(scene_viewport.x) / f32(scene_viewport.y);

      const vec2f32 image_size = {
        std::min<f32>(window_size.x, aspect_ratio * f32(window_size.y)),
        std::min<f32>(window_size.y, f32(window_size.x) / aspect_ratio)};
      const vec2f32 image_offset = (window_size - image_size) / 2.0f;

      const gpu::TextureNodeID render_output = store_->get_render_output();
      if (render_output.is_valid())
      {
        gui->set_cursor_pos(image_offset);
        gui->image(store_->get_render_output(), image_size);
      }

      const auto camera_id        = store_->scene_ref().get_render_camera_entity_id();
      const auto camera_model_mat = store_->scene_ref().entity_world_transform_ref(camera_id);
      const auto camera_view_mat  = math::inverse(camera_model_mat);
      const auto delta_time       = gui->get_delta_time();

      if (gui->is_window_hovered())
      {
        const auto mouse_uv_delta = gui->get_mouse_delta() / image_size;
        if (
          gui->is_key_down(app::KeyboardKey::LEFT_ALT) ||
          gui->is_key_down(app::KeyboardKey::RIGHT_ALT))
        {
          const auto selected_entity_id = store_->get_selected_entity();
          const auto target             = [&]()
          {
            if (selected_entity_id.is_null())
            {
              return vec3f32(0, 0, 0);
            } else
            {
              const auto selected_entity_mat =
                store_->scene_ref().entity_world_transform_ref(selected_entity_id);
              return selected_entity_mat.col(3).xyz();
            }
          }();

          auto camera_controller = OrbitCameraController(orbit_config_, camera_model_mat, target);

          if (gui->is_mouse_dragging(app::MouseButton::MIDDLE))
          {
            camera_controller.orbit(mouse_uv_delta);
          } else if (gui->get_mouse_wheel_delta() != 0.0f)
          {
            camera_controller.zoom(gui->get_mouse_wheel_delta());
          }
          store_->set_world_transform(camera_id, camera_controller.get_model_matrix());

        } else if (
          gui->is_key_down(app::KeyboardKey::LEFT_SHIFT) ||
          gui->is_key_down(app::KeyboardKey::RIGHT_SHIFT))
        {
          auto camera_controller = MapCameraController(map_config_, camera_model_mat);
          if (gui->is_mouse_dragging(app::MouseButton::MIDDLE))
          {
            camera_controller.pan(mouse_uv_delta);
          } else if (gui->get_mouse_wheel_delta() != 0.0f)
          {
            camera_controller.zoom(gui->get_mouse_wheel_delta());
          }
          if (gui->is_key_down(app::KeyboardKey::W))
          {
            camera_controller.pan(vec2f32(0, 0.3f) * delta_time);
          } else if (gui->is_key_down(app::KeyboardKey::S))
          {
            camera_controller.pan(vec2f32(0, -0.3f) * delta_time);
          } else if (gui->is_key_down(app::KeyboardKey::A))
          {
            camera_controller.pan(vec2f32(0.3f, 0) * delta_time);
          } else if (gui->is_key_down(app::KeyboardKey::D))
          {
            camera_controller.pan(vec2f32(-0.3f, 0) * delta_time);
          }
          store_->set_world_transform(camera_id, camera_controller.get_model_matrix());
        } else
        {
          auto camera_controller = FlightCameraController(flight_config_, camera_model_mat);
          if (gui->is_mouse_dragging(app::MouseButton::MIDDLE))
          {
            camera_controller.pan(mouse_uv_delta);
          }
          camera_controller.zoom(gui->get_mouse_wheel_delta());
          if (gui->is_key_down(app::KeyboardKey::W))
          {
            camera_controller.zoom(1.0f * delta_time);
          } else if (gui->is_key_down(app::KeyboardKey::S))
          {
            camera_controller.zoom(-1.0f * delta_time);
          }
          store_->set_world_transform(camera_id, camera_controller.get_model_matrix());
        }
      }

      const auto selected_entity_id = store_->get_selected_entity();

      if (!selected_entity_id.is_null())
      {
        const auto camera_data = store_->scene_ref().get_render_camera_data();

        auto local_transform_mat =
          store_->scene_ref().entity_local_transform_ref(selected_entity_id);

        auto world_transform_mat =
          store_->scene_ref().entity_world_transform_ref(selected_entity_id);
        auto world_transform = math::into_transform(world_transform_mat);
        auto clip_position =
          math::mul(camera_data.proj_view_mat, vec4f32(world_transform.position, 1.0f));
        clip_position /= clip_position.w;

        const auto clip_delta        = gui->get_mouse_delta() * 2.0f / image_size;
        const auto new_clip_position = clip_position.xyz() + vec3f32(clip_delta.xy(), 0);

        // if (transform_mode_ == TransformMode::TRANSLATE)
        // {
        //   auto new_world_position =
        //     math::mul(camera_data.inv_proj_view_mat, vec4f32(new_clip_position, 1));
        //   new_world_position /= new_world_position.w;
        //
        //   store_->set_world_transform(
        //     selected_entity_id,
        //     math::compose_transform(
        //       new_world_position.xyz(), world_transform.rotation, world_transform.scale));
        // }
      }

      if (gui->is_key_pressed(soul::app::KeyboardKey::G))
      {
        transform_mode_ = TransformMode::TRANSLATE;
        gizmo_op_       = app::Gui::GizmoOp::TRANSLATE;
      } else if (gui->is_key_pressed(soul::app::KeyboardKey::R))
      {
        gizmo_op_ = app::Gui::GizmoOp::ROTATE;
      } else if (gui->is_key_pressed(soul::app::KeyboardKey::S))
      {
        gizmo_op_ = app::Gui::GizmoOp::SCALE;
      }

      if (gui->is_key_pressed(soul::app::KeyboardKey::T))
      {
        gui->open_popup("Add Entity"_str);
      }
      if (gui->begin_popup("Add Entity"_str))
      {
        gui->separator_text("Add Entity"_str);
        if (gui->menu_item("Empty"_str))
        {
          store_->create_entity();
        }
        if (gui->begin_menu("Light"_str))
        {
          if (gui->menu_item("Point"_str))
          {
            store_->create_light_entity(LightRadiationType::POINT);
          }
          if (gui->menu_item("Spot"_str))
          {
            store_->create_light_entity(LightRadiationType::SPOT);
          }
          if (gui->menu_item("Directional"_str))
          {
            store_->create_light_entity(LightRadiationType::DIRECTIONAL);
          }
          gui->end_menu();
        }
        gui->end_popup();
      }
    }
    gui->end_window();
  }
} // namespace renderlab
