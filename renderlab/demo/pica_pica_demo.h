#pragma once

#include "demo.h"

namespace renderlab
{
  class PicaPicaDemo : public Demo
  {
  public:
    PicaPicaDemo()                                       = default;
    PicaPicaDemo(const PicaPicaDemo&)                    = default;
    PicaPicaDemo(PicaPicaDemo&&)                         = default;
    auto operator=(const PicaPicaDemo&) -> PicaPicaDemo& = default;
    auto operator=(PicaPicaDemo&&) -> PicaPicaDemo&      = default;
    void load_scene(NotNull<Scene*> scene) override;

  private:
    EntityId light_entity_id_ = EntityId::Null();
  };
} // namespace renderlab
