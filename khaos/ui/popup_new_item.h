#pragma once

#include "core/function.h"
#include "core/string.h"

using namespace soul;

namespace soul::app
{
  class Gui;
};

namespace khaos::ui
{
  class PopupNewItem
  {
  public:
    explicit PopupNewItem(StringView label);
    void on_gui_render(NotNull<app::Gui*> gui, Function<void(StringView label)> on_create);
    void open(NotNull<app::Gui*> gui);

  private:
    String popup_name_;
    String item_name_;
  };
} // namespace khaos::ui
