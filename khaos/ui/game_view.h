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
}

namespace khaos::ui
{

  class GameView
  {
  public:
    GameView() = default;
    void on_gui_render(NotNull<soul::app::Gui*> gui, NotNull<Store*> store);

  private:
    String user_input;

    void render_game_side_mode(NotNull<app::Gui*> gui, NotNull<Store*> store);
  };

} // namespace khaos::ui
