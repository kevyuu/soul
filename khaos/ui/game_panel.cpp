#include "ui/game_panel.h"
#include "ui/dialog_text.h"

#include "store.h"

#include "app/gui.h"
#include "app/icons.h"
#include "type.h"

using namespace soul;

namespace khaos
{
  void GamePanel::render_game_side_mode(NotNull<app::Gui*> gui, NotNull<Store*> store)
  {
    const vec2f32 available_region = gui->get_content_region_avail();
    const f32 background_height    = available_region.y;
    const f32 background_width     = (19 * background_height) / 13;
    {
      gui->image(store->background_texture_id, vec2f32(background_width, background_height));
    }
    gui->same_line();
    {
      const f32 group_width = gui->get_content_region_avail().x;
      gui->begin_group();
      gui->begin_child_window(
        "Dialog Box"_str, vec2f32(gui->get_content_region_avail().x, 0.9 * background_height));
      const auto messages = store->active_journey_cref().messages.cspan();
      for (i32 message_i = 0; message_i < messages.size(); message_i++)
      {
        const auto& message = messages[message_i];
        gui->push_id(message_i);
        gui->push_style_color(app::Gui::ColorVar::TEXT, vec4f32(1, 0.3, 0.3, 1.0));
        gui->align_text_to_frame_padding();
        gui->text(ROLE_LABELS[message.role], 22);
        gui->pop_style_color();
        gui->same_line(group_width - 2 * gui->get_frame_height_with_spacing());
        {
          gui->frameless_button(ICON_MD_EDIT);
          gui->same_line();
          gui->frameless_button(ICON_MD_DELETE);
          gui->new_line();
        }
        dialog_text(gui, message.content.cspan());
        gui->pop_id();
      }
      gui->button("Continue"_str);
      gui->same_line();
      gui->frameless_button(ICON_MD_ARROW_BACK);
      gui->same_line();
      gui->frameless_button(ICON_MD_ARROW_FORWARD);
      gui->end_child_window();
      f32 text_input_width = group_width - (2 * gui->get_frame_height_with_spacing());

      if (store->get_game_state() == GameState::WAITING_USER_RESPONSE)
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
            store->add_message(Role::USER, user_input.cspan());
            user_input.clear();
            store->run_task_completion();
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

  void GamePanel::on_gui_render(NotNull<app::Gui*> gui, NotNull<Store*> store)
  {
    if (gui->begin_window("Game"_str, vec2f32(1920, 1080)))
    {
      const auto window_size             = gui->get_window_size();
      const auto user_input_frame_height = gui->get_frame_height();
      const auto dialog_box_height       = 200;
      const auto dialog_box_control_height =
        gui->get_frame_height(20) + (2 * gui->get_frame_padding().y);

      if (store->is_any_journey_active())
      {
        // const auto child_window_width = 0.6 * window_size.x;
        // gui->push_style_color(app::Gui::ColorVar::CHILD_BG, vec4f32(0, 0, 0, 0.6));
        // gui->push_style_var(app::Gui::StyleVar::WINDOW_PADDING, vec2f32(16, 8));
        //
        // gui->set_cursor_pos(vec2f32(
        //   0.2 * window_size.x,
        //   gui->get_window_size().y -
        //     (user_input_frame_height + dialog_box_height + dialog_box_control_height + 2)));
        // gui->begin_child_window(
        //   "Dialog Box Control"_str,
        //   vec2f32(child_window_width, dialog_box_control_height),
        //   {app::Gui::ChildWindowFlag::BORDERS});
        // const auto font_size = dialog_box_control_height - (2 * gui->get_frame_padding().y);
        // gui->text("Shopkeeper"_str, font_size);
        // gui->same_line(child_window_width - 140);
        // gui->button(ICON_MD_REFRESH);
        // gui->same_line();
        // gui->button(ICON_MD_ARROW_LEFT);
        // gui->same_line();
        // gui->button(ICON_MD_ARROW_RIGHT);
        // gui->end_child_window();
        //
        // gui->set_cursor_pos(vec2f32(
        //   0.2 * window_size.x,
        //   gui->get_window_size().y - (dialog_box_height + user_input_frame_height)));
        // gui->begin_child_window(
        //   "Dialog Box"_str,
        //   vec2f32(child_window_width, dialog_box_height),
        //   {app::Gui::ChildWindowFlag::BORDERS});
        // dialog_text(gui, store->active_journey_cref().messages.back().content.cspan());
        // // gui->text_wrapped(store->active_journey_cref().messages.back().content.cspan());
        // gui->end_child_window();
        //
        // gui->pop_style_var();
        // gui->pop_style_color();
        //
        // {
        //   gui->push_style_var(app::Gui::StyleVar::ITEM_SPACING, vec2f32(0, 0));
        //   gui->set_cursor_pos(
        //     vec2f32(0.2 * window_size.x, window_size.y - user_input_frame_height));
        //   gui->input_text_multiline(
        //     "###user_input"_str, &user_input, vec2f32(child_window_width,
        //     user_input_frame_height));
        //   gui->same_line();
        //   if (gui->button(ICON_MD_SEND))
        //   {
        //     store->add_message(Role::USER, user_input.cspan());
        //     store->run_task_completion();
        //   }
        //   gui->same_line();
        //   gui->button(ICON_MD_LIST);
        //   gui->same_line();
        //   gui->button(ICON_MD_CAMERA);
        //   gui->same_line();
        //   gui->button(ICON_MD_LOOKS);
        //   gui->same_line();
        //   gui->pop_style_var();
        // }

        render_game_side_mode(gui, store);
      } else
      {
        gui->set_cursor_pos(vec2f32(window_size.x / 2, window_size.y / 2));
        if (gui->button("New Project"_str))
        {
          store->create_new_journey();
        }
      }
    }
    gui->end_window();
  }
} // namespace khaos
