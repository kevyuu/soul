#include "core/comp_str.h"

#include "app/gui.h"

using namespace soul;

namespace khaos
{
  struct Popup
  {
    CompStr name;
    b8 is_open   = false;
    b8 will_open = false;

    explicit Popup(CompStr name) : name(name) {}

    void open(NotNull<app::Gui*> gui)
    {
      will_open = true;
    }

    template <typename RenderFn, typename CloseFn>
    void operator()(NotNull<app::Gui*> gui, RenderFn render_fn, CloseFn close_fn)
    {
      if (is_open)
      {
        if (gui->begin_popup_modal(name, &is_open, {app::Gui::WindowFlag::NO_MOVE}))
        {
          render_fn();
          gui->end_popup();
        } else
        {
          close_fn();
        }
      }
      if (will_open)
      {
        will_open = false;
        is_open   = true;
        gui->open_popup(name);
      }
    }
  };
} // namespace khaos
