#pragma once

#include "camera_controller.h"
#include "editor/panel.h"

#include "app/gui.h"

#include "core/not_null.h"
#include "core/path.h"

#include "render_node.h"

using namespace soul;

namespace soul::app
{
  class Gui;
};

namespace renderlab
{
  class EditorStore;

  enum class TransformMode
  {
    TRANSLATE,
    ROTATE,
    SCALE,
    COUNT
  };

  class ViewportPanel : public EditorPanel
  {
  private:
    NotNull<EditorStore*> store_;
    soul::app::Gui::GizmoOp gizmo_op_;
    TransformMode transform_mode_;

    OrbitCameraController::Config orbit_config_ = {
      .zoom_speed  = 1.0f,
      .orbit_speed = 1.0f,
      .up_axis     = vec3f32(0, 1, 0),
    };

    FlightCameraController::Config flight_config_ = {
      .zoom_speed = 1.0f,
      .pan_speed  = 2.5f,
      .up_axis    = vec3f32(0, 1, 0),
    };

    MapCameraController::Config map_config_ = {
      .zoom_speed = 1.0f,
      .pan_speed  = 2.0f,
    };

  public:
    static constexpr CompStr LABEL = "Viewport"_str;

    explicit ViewportPanel(NotNull<EditorStore*> store);

    void on_gui_render(NotNull<soul::app::Gui*> gui) override;

    [[nodiscard]]
    auto get_title() const -> CompStr override
    {
      return LABEL;
    }
  };
} // namespace renderlab
