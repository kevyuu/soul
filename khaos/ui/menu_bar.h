#pragma once

#include "core/not_null.h"
#include "core/path.h"

using namespace soul;

namespace soul::app
{
  class Gui;
}

namespace khaos
{
  class MenuBar
  {
  public:
    explicit MenuBar();
    void render(NotNull<app::Gui*> gui);

  private:
    String gltf_file_path_;
  };
} // namespace khaos
