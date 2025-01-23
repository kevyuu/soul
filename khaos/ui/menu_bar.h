#pragma once

#include "core/not_null.h"

#include "store/store.h"

#include "ui/app_setting_view.h"
#include "ui/popup.h"

using namespace soul;

namespace soul::app
{
  class Gui;
}

namespace khaos::ui
{
  class MenuBar
  {
  public:
    explicit MenuBar();
    void render(NotNull<app::Gui*> gui, Store* store);

  private:
    String gltf_file_path_;
    AppSettingView app_setting_view_;
    b8 is_app_setting_view_open_ = false;

    Popup app_setting_popup = Popup("Edit App Setting"_str);
  };
} // namespace khaos::ui
