#include "ui/game_setting_panel.h"
#include "store.h"

#include "app/gui.h"

using namespace soul;

namespace khaos
{

  void GameSettingPanel::on_gui_render(NotNull<app::Gui*> gui, Store* store)
  {
    if (gui->begin_window("Game Setting"_str, vec2f32(1400, 1040), vec2f32(20, 40)))
    {
      if (gui->is_window_appearing())
      {
        header_prompt_.assign(store->header_prompt_cspan());
        first_message_.assign(store->first_message_cspan());
      }

      gui->text("Header Prompt"_str);
      gui->input_text_multiline("###Header Prompt"_str, &header_prompt_, vec2f32(0, 300));
      if (gui->is_item_deactivated_after_edit())
      {
        store->set_header_prompt(header_prompt_.cspan());
      }

      gui->text("First Message"_str);
      gui->input_text_multiline("###First Message"_str, &first_message_, vec2f32(0, 300));
      if (gui->is_item_deactivated_after_edit())
      {
        store->set_first_message(first_message_.cspan());
      }
    }
    gui->end_window();
  }
} // namespace khaos
