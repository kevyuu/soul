#include "panels/game_panel.h"
#include "app/icons.h"
#include "store.h"

#include "app/gui.h"

using namespace soul;

namespace khaos
{

  void GamePanel::on_gui_render(NotNull<app::Gui*> gui, Store* store)
  {
    if (gui->begin_window("Game"_str, vec2f32(1920, 1080)))
    {
      const auto window_size             = gui->get_window_size();
      const auto user_input_frame_height = gui->get_frame_height();
      const auto dialog_box_height       = 200;
      const auto dialog_box_control_height =
        gui->get_frame_height(20) + (2 * gui->get_frame_padding().y);

      gui->image(store->background_texture_id, window_size);

      if (store->is_any_journey_active())
      {
        const auto child_window_width = 0.6 * window_size.x;
        gui->push_style_color(app::Gui::ColorVar::CHILD_BG, vec4f32(0, 0, 0, 0.6));
        gui->push_style_var(app::Gui::StyleVar::WINDOW_PADDING, vec2f32(16, 8));

        gui->set_cursor_pos(vec2f32(
          0.2 * window_size.x,
          gui->get_window_size().y -
            (user_input_frame_height + dialog_box_height + dialog_box_control_height + 2)));
        gui->begin_child_window(
          "Dialog Box Control"_str,
          vec2f32(child_window_width, dialog_box_control_height),
          {app::Gui::ChildWindowFlag::BORDERS});
        const auto font_size = dialog_box_control_height - (2 * gui->get_frame_padding().y);
        gui->text("Shopkeeper"_str, font_size);
        gui->same_line(child_window_width - 140);
        gui->button(ICON_MD_REFRESH);
        gui->same_line();
        gui->button(ICON_MD_ARROW_LEFT);
        gui->same_line();
        gui->button(ICON_MD_ARROW_RIGHT);
        gui->end_child_window();

        gui->set_cursor_pos(vec2f32(
          0.2 * window_size.x,
          gui->get_window_size().y - (dialog_box_height + user_input_frame_height)));
        gui->begin_child_window(
          "Dialog Box"_str,
          vec2f32(child_window_width, dialog_box_height),
          {app::Gui::ChildWindowFlag::BORDERS});
        gui->text_wrapped(store->active_journey_cref().messages.back().content.cspan());
        gui->end_child_window();

        gui->pop_style_var();
        gui->pop_style_color();

        {
          gui->push_style_var(app::Gui::StyleVar::ITEM_SPACING, vec2f32(0, 0));
          gui->set_cursor_pos(
            vec2f32(0.2 * window_size.x, window_size.y - user_input_frame_height));
          gui->input_text_multiline(
            "###user_input"_str, &user_input, vec2f32(child_window_width, user_input_frame_height));
          gui->same_line();
          gui->button(ICON_MD_SEND);
          gui->same_line();
          gui->button(ICON_MD_LIST);
          gui->same_line();
          gui->button(ICON_MD_CAMERA);
          gui->same_line();
          gui->button(ICON_MD_LOOKS);
          gui->same_line();
          gui->pop_style_var();
        }
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
