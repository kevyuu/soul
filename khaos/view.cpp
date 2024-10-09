#include "view.h"
#include "store.h"

#include "app/gui.h"

namespace khaos
{

  void View::render(NotNull<app::Gui*> gui, NotNull<Store*> store)
  {
    if (!store->is_any_project_active())
    {
      project_selection_panel_.on_gui_render(gui, store);
    } else
    {
      menu_bar_.render(gui);

      gui->begin_dock_window();
      const auto dock_id = gui->get_id("Dock"_str);
      if (gui->dock_builder_init(dock_id))
      {
        gui->dock_builder_finish(dock_id);
      }
      gui->dock_space(dock_id);
      gui->end_window();
      connection_panel_.on_gui_render(gui, store);
    }
  }

} // namespace khaos
