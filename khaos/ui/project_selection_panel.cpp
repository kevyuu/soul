#include "ui/project_selection_panel.h"
#include "store/store.h"

#include "app/gui.h"

#include "type.h"

using namespace soul;

namespace khaos
{
  void ProjectSelectionPanel::on_gui_render(NotNull<soul::app::Gui*> gui, NotNull<Store*> store)
  {
    if (gui->begin_window(
          "Project Launcer"_str,
          gui->get_display_size(),
          vec2f32(0, 0),
          {
            app::Gui::WindowFlag::NO_TITLE_BAR,
            app::Gui::WindowFlag::NO_RESIZE,
            app::Gui::WindowFlag::NO_MOVE,
            app::Gui::WindowFlag::NO_SCROLLBAR,
          }))
    {
      new_project_popup.on_gui_render(gui, store);

      gui->text("Khaos"_str, 32.0f);
      if (gui->button("New"_str))
      {
        new_project_popup.open(gui);
      }
      gui->same_line();
      if (gui->button("Import"_str))
      {
        const auto result =
          gui->open_file_dialog(""_str, Path::From(""_str), "File"_str, ".kosmos"_str);
        if (result.is_some())
        {
        }
      }

      const auto project_metadatas = store->project_metadatas_cspan();
      if (gui->begin_table(
            "Projects"_str, 2, {app::Gui::TableFlag::ROW_BG, app::Gui::TableFlag::SCROLL_Y}))
      {
        gui->table_setup_column(
          "Project"_str, {app::Gui::TableColumnFlag::WIDTH_FIXED}, gui->get_window_size().x - 200);
        gui->table_setup_column("Action"_str, {app::Gui::TableColumnFlag::WIDTH_FIXED}, 200);
        gui->table_headers_row();
        for (i32 project_i = 0; project_i < project_metadatas.size(); project_i++)
        {
          const auto& project_metadata = project_metadatas[project_i];

          gui->table_next_row();
          gui->table_next_column();
          gui->begin_group();
          gui->text(project_metadata.name.cview());
          gui->text_colored(
            StringView(project_metadata.path.string().c_str()), vec4f32(0.6, 0.6, 0.6, 1.0), 14);
          gui->end_group();

          gui->table_next_column();
          gui->push_id(project_i);
          if (gui->button("Load"_str))
          {
            store->load_project(project_metadata.path);
          }
          gui->pop_id();
        }
        gui->end_table();
      }
    }

    gui->end_window();
  }

} // namespace khaos
