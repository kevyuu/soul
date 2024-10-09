#include "panels/project_selection_panel.h"
#include "core/util.h"
#include "store.h"

#include "core/type.h"

#include "app/gui.h"
#include "app/icons.h"

#include "misc/json.h"

#include "httplib.h"

using namespace soul;

namespace khaos
{
  void ProjectSelectionPanel::on_gui_render(NotNull<soul::app::Gui*> gui, NotNull<Store*> store)
  {
    // if (gui->begin_window(
    //       "Project Launcer"_str,
    //       gui->get_display_size(),
    //       vec2f32(8, 8),
    //       {
    //         app::Gui::WindowFlag::NO_RESIZE,
    //         app::Gui::WindowFlag::NO_SCROLLBAR,
    //       }))
    // {
    //   gui->text("Projects"_str);
    //   gui->button("New"_str);
    //   gui->same_line();
    //   gui->button("Open"_str);
    // }
    // gui->end_window();
    gui->show_demo_window();
  }

} // namespace khaos
