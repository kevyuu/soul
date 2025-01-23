#include "popup_new_item.h"

#include "app/gui.h"

namespace khaos::ui
{
  PopupNewItem::PopupNewItem(StringView label)
      : popup_name_(String::From(label)), item_name_(""_str)
  {
  }

  void PopupNewItem::on_gui_render(
    NotNull<app::Gui*> gui, Function<void(StringView label)> on_create_callback)
  {
    if (gui->begin_popup(popup_name_.cview()))
    {
      gui->input_text("Name"_str, &item_name_);
      if (gui->button("Create"_str, vec2f32(120, 0)))
      {
        on_create_callback(item_name_.cview());
        gui->close_current_popup();
      }
      gui->same_line();
      if (gui->button("Cancel"_str, vec2f32(120, 0)))
      {
        gui->close_current_popup();
      }
      gui->end_popup();
    }
  }

  void PopupNewItem::open(NotNull<app::Gui*> gui)
  {
    item_name_ = ""_str;
    gui->open_popup(popup_name_.cview());
  }
} // namespace khaos::ui
