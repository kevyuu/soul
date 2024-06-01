#pragma once

#include "type.h"
#include "type.shared.hlsl"

using namespace soul;
namespace renderlab
{
  struct MeshPreprocessor {

    struct Result {
      Vector<StaticVertexData> vertexes;
      IndexData indexes;
    };

    [[nodiscard]]
    static auto GenerateVertexes(const MeshDesc& desc) -> Result;
  };
} // namespace renderlab
