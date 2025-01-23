#pragma once

#include "ui/app_setting_view.h"
#include "ui/chat_view.h"
#include "ui/game_view.h"
#include "ui/journey_list_panel.h"
#include "ui/menu_bar.h"
#include "ui/project_selection_panel.h"

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
    ui::MenuBar menu_bar_;
    ui::GameView game_view_;
    ui::ChatView chat_view_;
    ProjectSelectionPanel project_selection_panel_;
    JourneyListPanel journey_list_panel_;

  public:
    explicit View() = default;

    void render(NotNull<app::Gui*> gui, NotNull<Store*> store);
  };
} // namespace khaos
