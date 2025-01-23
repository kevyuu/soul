#include "editor/panels/menu_bar.h"
#include "core/type.h"
#include "editor/store.h"

#include "core/panic.h"

#include "app/gui.h"
#include <portable-file-dialogs.h>

namespace renderlab
{
  MenuBar::MenuBar(NotNull<EditorStore*> store)
      : store_(store), gltf_file_path_(String::From(""_str))
  {
  }

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

    if (gui->begin_popup_modal("Import GLTF"_str))
    {

      const b8 browse_gltf_file = gui->button("Browse##gltf"_str);
      gui->same_line();
      gui->input_text("GLTF File"_str, &gltf_file_path_);

      if (browse_gltf_file)
      {
        auto result     = pfd::open_file("Select a file").result();
        gltf_file_path_ = String::From(StringView{result[0].c_str(), result[0].size()});
      }

      if (gui->button("Ok"_str, vec2f32(120, 0)))
      {
        store_->import(Path::From(gltf_file_path_.cview()));
        gui->close_current_popup();
      }

      gui->same_line();

      if (gui->button("Cancel"_str, vec2f32(120, 0)))
      {
        gui->close_current_popup();
      }

      gui->set_item_default_focus();
      gui->same_line();
      gui->end_popup();
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
} // namespace renderlab
