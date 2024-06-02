#pragma once

#include "core/not_null.h"
#include "core/path.h"

using namespace soul;

namespace renderlab
{
  class Scene;

  class AssimpImporter
  {
  public:
    AssimpImporter()                                               = default;
    AssimpImporter(const AssimpImporter& other)                    = delete;
    AssimpImporter(AssimpImporter&& other)                         = delete;
    auto operator=(const AssimpImporter& other) -> AssimpImporter& = delete;
    auto operator=(AssimpImporter&& other) -> AssimpImporter&      = delete;
    ~AssimpImporter()                                              = default;

    void import(const Path& path, NotNull<Scene*> scene);
    void cleanup();
  };
} // namespace renderlab
