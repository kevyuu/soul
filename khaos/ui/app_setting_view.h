#pragma once

#include "core/not_null.h"

#include "chatbot_setting_view.h"
#include "popup_new_item.h"

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

  class AppSettingView
  {
  public:
    AppSettingView() = default;
    void on_gui_render(NotNull<soul::app::Gui*> gui, Store* store);

  private:
    enum class TabType : u8
    {
      CHATBOT_SETTING,
      IMAGE_GEN_SETTING,
      COUNT,
    };
    static constexpr auto TAB_LABEL = FlagMap<TabType, CompStr>{
      "Chatbot"_str,
      "Image Generation"_str,
    };

    ChatbotSettingView chatbot_setting_view_;
    TabType tab_type_ = TabType::CHATBOT_SETTING;
  };
} // namespace khaos::ui
