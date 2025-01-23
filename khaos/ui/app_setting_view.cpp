#include "ui/app_setting_view.h"
#include "core/compiler.h"
#include "store/store.h"

#include "core/type.h"
#include "core/util.h"

#include "app/gui.h"
#include "app/icons.h"
#include "type.h"

using namespace soul;

namespace khaos::ui
{

  void AppSettingView::on_gui_render(NotNull<app::Gui*> gui, Store* store)
  {
    const auto& chatbot_setting = store->app_setting_cref().chatbot_setting;
    gui->begin_child_window("App Setting View"_str, vec2f32(1000, 800));
    {
      gui->begin_group();
      gui->push_style_var(app::Gui::StyleVar::FRAME_PADDING, vec2f32(16, 16));
      const vec2f32 tab_size = {5 * gui->get_frame_height(), 0};
      for (auto e : FlagIter<TabType>())
      {
        if (gui->selectable(TAB_LABEL[e], tab_type_ == e, {}, tab_size))
        {
          tab_type_ = e;
        }
      }
      gui->pop_style_var();
      gui->end_group();
    }

    gui->same_line();

    {
      const auto avail       = gui->get_content_region_avail();
      const auto view_width  = avail.x;
      const auto view_height = avail.y - gui->get_frame_height_with_spacing();
      gui->begin_child_window("App Setting View Panel"_str, vec2f32(view_width, view_height));
      switch (tab_type_)
      {
      case TabType::CHATBOT_SETTING:
      {
        chatbot_setting_view_.on_gui_render(gui, store);
        break;
      }
      case TabType::IMAGE_GEN_SETTING:
      {
        break;
      }
      default: unreachable();
      }
      gui->end_child_window();
    }
    gui->end_child_window();
  }
} // namespace khaos::ui
