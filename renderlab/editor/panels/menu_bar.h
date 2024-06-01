#pragma once

#include "core/not_null.h"
#include "core/path.h"

using namespace soul;

namespace soul::app
{
  class Gui;
}

namespace renderlab
{
  class EditorStore;

  class MenuBar
  {
  public:
    explicit MenuBar(NotNull<EditorStore*> store);
    void render(NotNull<app::Gui*> gui);

  private:
    NotNull<EditorStore*> store_;
    String gltf_file_path_;
  };
} // namespace renderlab
