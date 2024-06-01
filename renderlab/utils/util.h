#pragma once

#include "core/comp_str.h"
#include "gpu/system.h"

using namespace soul;

namespace renderlab::util
{
  inline auto create_compute_program(NotNull<gpu::System*> gpu_system, const CompStr path_str)
    -> gpu::ProgramID
  {
    const auto search_path = Path::From("shaders"_str);
    const auto shader_source =
      gpu::ShaderSource::From(gpu::ShaderFile{.path = Path::From(path_str)});
    constexpr auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::COMPUTE, "cs_main"_str},
    };
    const gpu::ProgramDesc program_desc = {
      .search_paths = u32cspan(&search_path, 1),
      .sources      = u32cspan(&shader_source, 1),
      .entry_points = entry_points.cspan<u32>(),
    };
    auto result = gpu_system->create_program(program_desc);
    if (result.is_err())
    {
      SOUL_PANIC("Fail to create program");
    }
    return result.ok_ref();
  }

  inline auto create_raster_program(NotNull<gpu::System*> gpu_system, const CompStr path_str)
  {
    const auto search_path = Path::From("shaders"_str);
    const auto shader_source =
      gpu::ShaderSource::From(gpu::ShaderFile{.path = Path::From(path_str)});
    constexpr auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::VERTEX, "vs_main"_str},
      gpu::ShaderEntryPoint{gpu::ShaderStage::FRAGMENT, "ps_main"_str},
    };

    const gpu::ProgramDesc program_desc = {
      .search_paths = u32cspan(&search_path, 1),
      .sources      = u32cspan(&shader_source, 1),
      .entry_points = entry_points.cspan<u32>(),
    };
    auto result = gpu_system->create_program(program_desc);
    if (result.is_err())
    {
      SOUL_PANIC("Fail to create program");
    }
    return result.ok_ref();
  }
} // namespace renderlab::util
