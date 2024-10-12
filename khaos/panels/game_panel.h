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

  class GamePanel
  {
  public:
    GamePanel() = default;
    void on_gui_render(NotNull<soul::app::Gui*> gui, Store* store);

  private:
    String user_input;
    String test_input;
  };

} // namespace khaos
