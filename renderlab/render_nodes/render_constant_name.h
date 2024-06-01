#pragma once

#include "core/comp_str.h"

using namespace soul;

namespace renderlab
{
  struct RenderConstantName
  {
    static constexpr CompStr SOBOL_TEXTURE    = "sobol"_str;
    static constexpr CompStr SCRAMBLE_TEXTURE = "scramble"_str;
    static constexpr CompStr BRDF_LUT_TEXTURE = "brdf_lut"_str;

    static constexpr CompStr QUAD_VERTEX_BUFFER = "quad_vertex_buffer"_str;
    static constexpr CompStr QUAD_INDEX_BUFFER  = "quad_index_buffer"_str;

    static constexpr CompStr UNIT_CUBE_VERTEX_BUFFER = "unit_cube_vertex_buffer"_str;
    static constexpr CompStr UNIT_CUBE_INDEX_BUFFER  = "unit_cube_index_buffer"_str;
  };
} // namespace renderlab
