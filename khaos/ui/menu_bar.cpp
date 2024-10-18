#include "ui/menu_bar.h"
#include "core/type.h"

#include "core/panic.h"

#include "app/gui.h"

namespace khaos
{
  MenuBar::MenuBar() {}

  void MenuBar::render(NotNull<app::Gui*> gui)
  {
    enum Action
    {
      ACTION_NONE,
      ACTION_IMPORT_GLTF,

      ACTION_EDIT_UI_STYLE,

      ACTION_COUNT
    };

    Action action = ACTION_NONE;

    if (gui->begin_main_menu_bar())
    {
      if (gui->begin_menu("File"_str))
      {
        if (gui->begin_menu("Import"_str))
        {
          if (gui->menu_item("Import GLTF"_str))
          {
            action = ACTION_IMPORT_GLTF;
          }
          gui->end_menu();
        }
        gui->end_menu();
      } else if (gui->begin_menu("Setting"_str))
      {
        if (gui->menu_item("Edit UI Style"_str))
        {
          action = ACTION_EDIT_UI_STYLE;
        }
        gui->end_menu();
      }
      gui->end_main_menu_bar();
    }

    if (gui->begin_popup_modal("Edit UI Style"_str))
    {
      gui->show_style_editor();
      if (gui->button("Close"_str, vec2f32(120, 0)))
      {
        gui->close_current_popup();
      }
      gui->end_popup();
    }

    switch (action)
    {
    case ACTION_NONE: break;
    case ACTION_IMPORT_GLTF: gui->open_popup("Import GLTF"_str); break;
    case ACTION_EDIT_UI_STYLE: gui->open_popup("Edit UI Style"_str); break;
    default: SOUL_ASSERT(0, false, "Invalid menu bar action"); break;
    }
  }
} // namespace khaos
