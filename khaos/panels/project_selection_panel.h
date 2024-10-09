#pragma once

#include "type.h"

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

  class ProjectSelectionPanel
  {
  public:
    ProjectSelectionPanel() = default;
    void on_gui_render(NotNull<soul::app::Gui*> gui, NotNull<Store*> store);
  };
} // namespace khaos
