#include "ui/game_view.h"

#include "store/store.h"

#include "app/gui.h"

using namespace soul;

namespace khaos::ui
{
  void GameView::render_game_side_mode(NotNull<app::Gui*> gui, NotNull<Store*> store) {}

  void GameView::on_gui_render(NotNull<app::Gui*> gui, NotNull<Store*> store)
  {
    const auto window_size             = gui->get_window_size();
    const auto user_input_frame_height = gui->get_frame_height();
    const auto dialog_box_height       = 200;
    const auto dialog_box_control_height =
      gui->get_frame_height(20) + (2 * gui->get_frame_padding().y);

    if (store->is_any_journey_active())
    {
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
} // namespace khaos::ui
