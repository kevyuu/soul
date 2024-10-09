#pragma once

#include "panels/chatbot_setting_panel.h"
#include "panels/menu_bar.h"
#include "panels/project_selection_panel.h"

#include "core/not_null.h"

using namespace soul;

namespace soul::app
{
  class Gui;
};

namespace khaos
{
  class Store;

  class View
  {
  private:
    MenuBar menu_bar_;
    ChatbotSettingPanel connection_panel_;
    ProjectSelectionPanel project_selection_panel_;

  public:
    explicit View() : menu_bar_() {}

    void render(NotNull<app::Gui*> gui, NotNull<Store*> store);
  };
} // namespace khaos
