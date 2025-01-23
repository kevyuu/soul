#pragma once

#include "store/store.h"

#include "app/gui.h"

namespace khaos::ui
{
  class PromptFormatPresetsGroup
  {
  public:
    void on_gui_render(NotNull<app::Gui*> gui, NotNull<Store*> store);

  private:
    u32 selected_index_;
  };
} // namespace khaos::ui
