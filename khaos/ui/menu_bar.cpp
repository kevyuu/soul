#include "ui/menu_bar.h"
#include "core/type.h"

#include "core/panic.h"

#include "app/gui.h"

namespace khaos::ui
{
  MenuBar::MenuBar() {}

  void MenuBar::render(NotNull<app::Gui*> gui, Store* store)
  {

    app_setting_popup(
      gui,
      [this, gui, store]()
      {
        app_setting_view_.on_gui_render(gui, store);
      },
      [store]()
      {
        SOUL_LOG_INFO("Save App Settings");
        store->save_app_settings();
      });

    if (gui->begin_main_menu_bar())
    {
      if (gui->begin_menu("Menu"_str))
      {
        if (gui->menu_item("Open Project"_str))
        {
        }
        if (gui->menu_item("Edit App Setting"_str))
        {
          app_setting_popup.open(gui);
        }
        gui->end_menu();
      }
      gui->end_main_menu_bar();
    }
  }
} // namespace khaos::ui
