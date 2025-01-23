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
};

namespace khaos::ui
{

  class ChatView
  {
  public:
    ChatView() = default;
    void on_gui_render(NotNull<soul::app::Gui*> gui, Store* store);

  private:
    String user_input;
  };
} // namespace khaos::ui
