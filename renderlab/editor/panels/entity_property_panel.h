#pragma once

#include "core/comp_str.h"
#include "editor/panel.h"

namespace renderlab
{
  class EditorStore;

  class EntityPropertyPanel : public EditorPanel
  {
  private:
    NotNull<EditorStore*> store_;

  public:
    static constexpr CompStr LABEL = "Entity Property"_str;

    explicit EntityPropertyPanel(NotNull<EditorStore*> store);

    void on_gui_render(NotNull<soul::app::Gui*> gui) override;

    [[nodiscard]]
    auto get_title() const -> CompStr override
    {
      return LABEL;
    }
  };
} // namespace renderlab
