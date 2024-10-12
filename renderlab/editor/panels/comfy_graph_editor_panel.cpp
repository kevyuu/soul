#include "comfy_graph_editor_panel.h"

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
  ComfyGraphEditorPanel::ComfyGraphEditorPanel(NotNull<EditorStore*> store) : store_(store) {}

  void ComfyGraphEditorPanel::on_gui_render(NotNull<app::Gui*> gui)
  {
    if (gui->begin_window(
          LABEL,
          vec2f32(1400, 1040),
          vec2f32(20, 40),
          {
            
            
            app::Gui::WindowFlag::NO_SCROLLBAR,
          }))
    {
    }
    gui->end_window();
  }
} // namespace renderlab
