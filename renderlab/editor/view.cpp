
#include "editor/view.h"

#include "app/gui.h"
#include "editor/panels/comfy_graph_editor_panel.h"
#include "editor/panels/entity_property_panel.h"
#include "editor/panels/render_pipeline_panel.h"
#include "editor/panels/scene_hierarchy_panel.h"
#include "editor/panels/scene_setting_panel.h"
#include "editor/panels/viewport_panel.h"
#include "editor/store.h"

namespace renderlab
{

  void EditorView::render(NotNull<app::Gui*> gui)
  {
    menu_bar_.render(gui);

    gui->begin_dock_window();
    const auto dock_id = gui->get_id("Dock"_str);
    if (gui->dock_builder_init(dock_id))
    {
      const auto display_size         = gui->get_display_size();
      const auto is_ultrawide_display = (display_size.x / display_size.y) > 2;
      if (is_ultrawide_display)
      {
        const auto main_splits =
          gui->dock_builder_split_dock(dock_id, soul::app::Gui::Direction::LEFT, 0.7);

        const auto viewport_dock_id = main_splits.ref<0>();
        const auto tools_dock_id    = main_splits.ref<1>();

        const auto tools_docks =
          gui->dock_builder_split_dock(tools_dock_id, soul::app::Gui::Direction::LEFT, 0.5);
        const auto left_tool_docks =
          gui->dock_builder_split_dock(tools_docks.ref<0>(), soul::app::Gui::Direction::UP, 0.5);
        const auto left_top_dock_id    = left_tool_docks.ref<0>();
        const auto left_bottom_dock_id = left_tool_docks.ref<1>();
        const auto right_tool_dock_id  = tools_docks.ref<1>();

        gui->dock_builder_dock_window(ComfyGraphEditorPanel::LABEL, viewport_dock_id);
        gui->dock_builder_dock_window(ViewportPanel::LABEL, viewport_dock_id);
        gui->dock_builder_dock_window(SceneHierarchyPanel::LABEL, left_top_dock_id);
        gui->dock_builder_dock_window(EntityPropertyPanel::LABEL, left_bottom_dock_id);
        gui->dock_builder_dock_window(RenderPipelinePanel::LABEL, right_tool_dock_id);
        gui->dock_builder_dock_window(SceneSettingPanel::LABEL, right_tool_dock_id);
      } else
      {
        const auto main_splits =
          gui->dock_builder_split_dock(dock_id, soul::app::Gui::Direction::RIGHT, 0.82);

        const auto viewport_dock_id = main_splits.ref<0>();
        const auto left_dock_id     = main_splits.ref<1>();

        const auto left_dock_splits =
          gui->dock_builder_split_dock(left_dock_id, soul::app::Gui::Direction::UP, 0.35);
        const auto scene_hierarchy_dock_id = left_dock_splits.ref<0>();
        const auto entity_property_dock_id = left_dock_splits.ref<1>();

        gui->dock_builder_dock_window(ComfyGraphEditorPanel::LABEL, viewport_dock_id);
        gui->dock_builder_dock_window(ViewportPanel::LABEL, viewport_dock_id);
        gui->dock_builder_dock_window(SceneHierarchyPanel::LABEL, scene_hierarchy_dock_id);
        gui->dock_builder_dock_window(EntityPropertyPanel::LABEL, entity_property_dock_id);
        gui->dock_builder_dock_window(RenderPipelinePanel::LABEL, entity_property_dock_id);
        gui->dock_builder_dock_window(SceneSettingPanel::LABEL, entity_property_dock_id);
      }

      gui->dock_builder_finish(dock_id);
    }
    gui->dock_space(dock_id);
    gui->end_window();

    comfy_graph_editor_panel_.on_gui_render(gui);
    viewport_panel_.on_gui_render(gui);
    scene_hierarchy_panel_.on_gui_render(gui);
    entity_property_panel_.on_gui_render(gui);
    render_pipeline_panel_.on_gui_render(gui);
    scene_setting_panel_.on_gui_render(gui);
  }

} // namespace renderlab
