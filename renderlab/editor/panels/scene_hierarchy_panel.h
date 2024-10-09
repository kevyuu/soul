#pragma once

#include "core/comp_str.h"
#include "core/string.h"

#include "editor/panel.h"

namespace renderlab
{
  class EditorStore;

  class SceneHierarchyPanel : public EditorPanel
  {
  private:
    NotNull<EditorStore*> store_;
    String search_text_;

  public:
    static constexpr CompStr LABEL = "Scene Hierarchy"_str;

    explicit SceneHierarchyPanel(NotNull<EditorStore*> store);

    void on_gui_render(NotNull<soul::app::Gui*> gui) override;

    [[nodiscard]]
    auto get_title() const -> CompStr override
    {
      return LABEL;
    }
  };
} // namespace renderlab
