#include "scene_setting_panel.h"

#include "app/gui.h"

#include "editor/store.h"
#include "type.h"

namespace renderlab
{

  SceneSettingPanel::SceneSettingPanel(NotNull<EditorStore*> store) : store_(store) {}

  void SceneSettingPanel::on_gui_render(NotNull<app::Gui*> gui)
  {
    if (gui->begin_window(
          LABEL,
          vec2f32(1400, 1040),
          vec2f32(20, 40),
          {
            app::Gui::WindowFlag::SHOW_TITLE_BAR,
            app::Gui::WindowFlag::ALLOW_MOVE,
          }))
    {
      if (gui->begin_tab_bar("Scene Settings Tab Bar"_str))
      {
        if (gui->begin_tab_item("EnvMap"_str))
        {

          EnvMapSetting setting = store_->get_env_map_setting();

          b8 is_env_map_setting_changed = false;
          is_env_map_setting_changed |= gui->input_f32("Intensity"_str, &setting.intensity);
          is_env_map_setting_changed |= gui->color_edit3("Tint"_str, &setting.tint);

          if (is_env_map_setting_changed)
          {
            store_->set_env_map_setting(setting);
          }

          gui->end_tab_item();
        }
        if (gui->begin_tab_item("Render"_str))
        {
          RenderSetting setting = store_->get_render_setting();
          if (gui->checkbox("Enable jitter"_str, &setting.enable_jitter))
          {
            store_->set_render_setting(setting);
          }
          gui->end_tab_item();
        }

        gui->end_tab_bar();
      }
    }
    gui->end_window();
  }
} // namespace renderlab
