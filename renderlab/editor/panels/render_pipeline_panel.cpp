#include "render_pipeline_panel.h"

#include "core/flag_map.h"
#include "ecs.h"
#include "editor/store.h"

#include "app/gui.h"
#include "math/math.h"
#include "math/matrix.h"
#include "math/quaternion.h"
#include "type.shared.hlsl"

namespace renderlab
{
  RenderPipelinePanel::RenderPipelinePanel(NotNull<EditorStore*> store) : store_(store) {}

  void RenderPipelinePanel::on_gui_render(NotNull<app::Gui*> gui)
  {
    if (gui->begin_window(
          LABEL,
          vec2f32(1400, 1040),
          vec2f32(20, 40),
          {
            app::Gui::WindowFlag::NO_SCROLLBAR,
          }))
    {
      store_->active_render_pipeline_ref().on_gui_render(gui);
    }
    gui->end_window();
  }
} // namespace renderlab
