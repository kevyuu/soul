#pragma once

#include "ui/new_project_popup.h"

#include "core/not_null.h"

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
  private:
    NewProjectPopup new_project_popup;

  public:
    ProjectSelectionPanel() = default;
    void on_gui_render(NotNull<soul::app::Gui*> gui, NotNull<Store*> store);
  };
} // namespace khaos
