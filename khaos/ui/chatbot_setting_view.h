#pragma once

#include "core/string.h"

#include "app/gui.h"

#include "store/store.h"

#include "popup_new_item.h"

namespace khaos::ui
{
  class ChatbotSettingView
  {
  public:
    ChatbotSettingView() = default;
    void on_gui_render(NotNull<app::Gui*> gui, NotNull<Store*> store);

  private:
    String api_url_;
    PromptFormat edit_prompt_format_;
    Sampler edit_sampler_;

    PopupNewItem new_sampler_popup_       = PopupNewItem("New Sampler"_str);
    PopupNewItem new_prompt_format_popup_ = PopupNewItem("New Prompt Format"_str);
  };
} // namespace khaos::ui
