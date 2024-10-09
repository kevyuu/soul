#pragma once

#include "core/not_null.h"
#include "core/string.h"

using namespace soul;

namespace soul::app
{
  class Gui;
};

namespace khaos
{
  class ChatbotSettingPanel
  {
  public:
    ChatbotSettingPanel() = default;
    void on_gui_render(NotNull<soul::app::Gui*> gui);

  private:
    String api_url_;
  };
} // namespace khaos
