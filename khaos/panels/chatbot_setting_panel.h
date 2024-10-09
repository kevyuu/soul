#pragma once

#include "core/not_null.h"
#include "core/string.h"

#include "type.h"

using namespace soul;

namespace soul::app
{
  class Gui;
};

namespace khaos
{
  class Store;

  class ChatbotSettingPanel
  {
  public:
    ChatbotSettingPanel() = default;
    void on_gui_render(NotNull<soul::app::Gui*> gui, Store* store);

  private:
    String api_url_;
    PromptFormat edit_prompt_format_;
    String new_prompt_format_name_;
    Sampler edit_sampler_;
    String new_sampler_name_;

    void render_prompt_formatting_widget(NotNull<soul::app::Gui*> gui, Store* store);
    void render_sampler_setting(NotNull<soul::app::Gui*> gui, Store* store);
  };
} // namespace khaos
