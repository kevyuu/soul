#pragma once

#include "editor/panel.h"
#include "editor/panels/comfy_graph_editor_panel.h"
#include "editor/panels/entity_property_panel.h"
#include "editor/panels/menu_bar.h"
#include "editor/panels/render_pipeline_panel.h"
#include "editor/panels/scene_hierarchy_panel.h"
#include "editor/panels/scene_setting_panel.h"
#include "editor/panels/viewport_panel.h"

#include "core/not_null.h"
#include "core/vector.h"

using namespace soul;

namespace soul::app
{
  class Gui;
};

namespace renderlab
{
  class EditorStore;

  class EditorView
  {
  private:
    NotNull<EditorStore*> store_;
    MenuBar menu_bar_;
    ViewportPanel viewport_panel_;
    ComfyGraphEditorPanel comfy_graph_editor_panel_;
    SceneHierarchyPanel scene_hierarchy_panel_;
    EntityPropertyPanel entity_property_panel_;
    RenderPipelinePanel render_pipeline_panel_;
    SceneSettingPanel scene_setting_panel_;

  public:
    explicit EditorView(NotNull<EditorStore*> store)
        : store_(store),
          menu_bar_(store),
          viewport_panel_(store),
          comfy_graph_editor_panel_(store),
          scene_hierarchy_panel_(store),
          entity_property_panel_(store),
          render_pipeline_panel_(store),
          scene_setting_panel_(store)
    {
    }

    void render(NotNull<app::Gui*> gui);
  };
} // namespace renderlab
