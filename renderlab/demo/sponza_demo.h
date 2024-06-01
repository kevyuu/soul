#pragma once

#include "demo.h"

namespace renderlab
{
  class SponzaDemo : public Demo
  {
  public:
    SponzaDemo()                                     = default;
    SponzaDemo(const SponzaDemo&)                    = default;
    SponzaDemo(SponzaDemo&&)                         = default;
    auto operator=(const SponzaDemo&) -> SponzaDemo& = default;
    auto operator=(SponzaDemo&&) -> SponzaDemo&      = default;
    void load_scene(NotNull<Scene*> scene) override;

  private:
    EntityId light_entity_id_ = EntityId::Null();
  };
} // namespace renderlab
