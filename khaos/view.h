#pragma once

#include "panels/chatbot_setting_panel.h"
#include "panels/game_panel.h"
#include "panels/game_setting_panel.h"
#include "panels/journey_list_panel.h"
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
    GamePanel game_panel_;
    GameSettingPanel game_setting_panel_;
    ProjectSelectionPanel project_selection_panel_;
    JourneyListPanel journey_list_panel_;

  public:
    explicit View() = default;

    void render(NotNull<app::Gui*> gui, NotNull<Store*> store);
  };
} // namespace khaos
