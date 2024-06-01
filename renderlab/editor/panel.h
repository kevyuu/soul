#pragma once

#include "core/comp_str.h"
#include "core/not_null.h"

using namespace soul;

namespace soul::app
{
  class Gui;
};

namespace renderlab
{

  class EditorPanel
  {
  public:
    EditorPanel() = default;

    EditorPanel(const EditorPanel&) = default;

    EditorPanel(EditorPanel&&) = default;

    auto operator=(const EditorPanel&) -> EditorPanel& = default;

    auto operator=(EditorPanel&&) -> EditorPanel& = default;

    virtual ~EditorPanel() = default;

    virtual void on_gui_render(NotNull<app::Gui*> gui) = 0;

    [[nodiscard]]
    virtual auto get_title() const -> CompStr = 0;
  };
} // namespace renderlab
