#include "ui/chat_view.h"
#include "ui/dialog_text.h"

#include "store/store.h"

#include "app/gui.h"
#include "app/icons.h"

using namespace soul;

namespace khaos::ui
{

  void ChatView::on_gui_render(NotNull<app::Gui*> gui, Store* store)
  {

    if (!store->is_any_journey_active())
    {
      gui->text("No journey loaded"_str);
    } else
    {
      const vec2f32 available_region = gui->get_content_region_avail();
      const f32 background_height    = available_region.y;
      const f32 group_width          = gui->get_content_region_avail().x;
      gui->begin_group();
      gui->begin_child_window(
        "Dialog Box"_str, vec2f32(gui->get_content_region_avail().x, 0.9 * background_height));

      auto& textgen_system = store->textgen_system_ref();

      const auto messages = store->active_journey_cref().messages.cspan();
      for (i32 message_i = 0; message_i < messages.size(); message_i++)
      {
        const auto& message = messages[message_i];
        gui->push_id(message_i);
        gui->push_style_color(app::Gui::ColorVar::TEXT, vec4f32(1, 0.3, 0.3, 1.0));
        gui->align_text_to_frame_padding();
        gui->text(message.label.cview(), 22);
        gui->pop_style_color();
        gui->same_line(group_width - 2 * gui->get_frame_height_with_spacing());
        {
          gui->frameless_button(ICON_MD_EDIT);
          gui->same_line();
          gui->frameless_button(ICON_MD_DELETE);
          gui->new_line();
        }
        const auto streaming_view = textgen_system.streaming_buffer_cview();
        if (message_i == messages.size() - 1 && streaming_view != ""_str)
        {
          auto full_content = String::Format("{}{}", message.content.cview(), streaming_view);
          dialog_text(gui, full_content.cview());
        } else
        {
          dialog_text(gui, message.content.cview());
        }
        if (streaming_view.size() != 0)
        {
          gui->set_scroll_here_y(0);
        }
        gui->pop_id();
      }



      gui->button("Continue"_str);
      gui->same_line();
      gui->frameless_button(ICON_MD_ARROW_BACK);
      gui->same_line();
      gui->frameless_button(ICON_MD_ARROW_FORWARD);
      gui->end_child_window();
      f32 text_input_width = group_width - (2 * gui->get_frame_height_with_spacing());

      if (!store->textgen_system_ref().is_any_pending_response())
      {
        gui->input_text_multiline(
          "###user_input"_str,
          &user_input,
          vec2f32(text_input_width, 2 * gui->get_frame_height() + gui->get_item_spacing().y));
        gui->same_line();
        {
          gui->begin_group();
          if (gui->button(ICON_MD_SEND))
          {
            store->script_system_ref().on_user_text_input(user_input.cview());
            user_input.clear();
          }
          gui->button(ICON_MD_ARROW_FORWARD);
          gui->end_group();
          gui->same_line();
          gui->begin_group();
          gui->button(ICON_MD_FORMAT_LIST_BULLETED);
          gui->button(ICON_MD_PHOTO_CAMERA);
          gui->end_group();
        }
      }
      gui->end_group();
    }
  }
} // namespace khaos::ui
