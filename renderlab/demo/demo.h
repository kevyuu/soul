#pragma once

#include "scene.h"

namespace renderlab
{
  class Demo
  {
  public:
    Demo()                               = default;
    Demo(const Demo&)                    = default;
    Demo(Demo&&)                         = default;
    auto operator=(const Demo&) -> Demo& = default;
    auto operator=(Demo&&) -> Demo&      = default;

  private:
    virtual void load_scene(NotNull<Scene*> scene) = 0;
  };
} // namespace renderlab
