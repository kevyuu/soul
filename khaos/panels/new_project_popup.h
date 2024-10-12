#pragma once

#include "core/not_null.h"
#include "core/string.h"

#include "type.h"

using namespace soul;

namespace soul::app
{
  class Gui;
};

namespace khaos
{
  class Store;

  class NewProjectPopup
  {
  public:
    NewProjectPopup() = default;
    void on_gui_render(NotNull<soul::app::Gui*> gui, Store* store);
    void open(NotNull<soul::app::Gui*> gui);

    auto get_id() -> CompStr
    {
      return "New Project"_str;
    }

  private:
    String name_                    = ""_str;
    String path_                    = ""_str;
    b8 show_empty_name_error_       = false;
    b8 show_directory_not_exists_   = false;
    b8 show_project_already_exists_ = false;

    void render_prompt_formatting_widget(NotNull<soul::app::Gui*> gui, Store* store);
    void render_sampler_setting(NotNull<soul::app::Gui*> gui, Store* store);
  };
} // namespace khaos
