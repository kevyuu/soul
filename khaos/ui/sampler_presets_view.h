#pragma once

#include "app/gui.h"

#include "store/store.h"

#include "ui/popup_new_item.h"

namespace khaos::ui
{
  class SamplerPresetsView
  {
  public:
    void on_gui_render(NotNull<app::Gui*> gui, NotNull<Store*> store);

  private:
    u32 selected_index_ = 0;
    Sampler edit_sampler_;
    PopupNewItem new_sampler_popup_ = PopupNewItem("New Sampler"_str);

    void render_sampler_preset_list(NotNull<app::Gui*> gui, NotNull<Store*> store);
    void render_edit_sampler_view(NotNull<app::Gui*> gui, NotNull<Store*> store);
  };
} // namespace khaos::ui
