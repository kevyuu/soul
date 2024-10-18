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
  class Store;

  class GameSettingPanel
  {
  public:
    GameSettingPanel() = default;
    void on_gui_render(NotNull<soul::app::Gui*> gui, Store* store);

  private:
    String header_prompt_;
    String first_message_;
  };
} // namespace khaos
