#include "view.h"
#include "store/store.h"

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
      menu_bar_.render(gui, store);

      gui->begin_dock_window();
      const auto dock_id = gui->get_id("Dock"_str);
      if (gui->dock_builder_init(dock_id))
      {
        gui->dock_builder_finish(dock_id);
      }
      gui->dock_space(dock_id);
      gui->end_window();

      gui->window_scope(
        [this, store](NotNull<app::Gui*> gui)
        {
          game_view_.on_gui_render(gui, store);
        },
        "Game View"_str,
        vec2f32(1920, 1080));

      gui->window_scope(
        [this, store](NotNull<app::Gui*> gui)
        {
          chat_view_.on_gui_render(gui, store);
        },
        "Chat View"_str,
        vec2f32(512, 1080));

      // gui->show_demo_window();
    }
  }

} // namespace khaos
