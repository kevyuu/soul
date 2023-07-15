#include "core/config.h"
#include "gpu/type.h"
#include "memory/allocator.h"

namespace soul::gpu
{

  auto compute_as_geometry_info(
    const BlasBuildDesc& build_desc,
    RTBuildMode build_mode,
    VkAccelerationStructureGeometryKHR* as_geometries)
    -> VkAccelerationStructureBuildGeometryInfoKHR;

  auto compute_max_primitives_counts(
    const BlasBuildDesc& build_desc, memory::Allocator& allocator = *get_default_allocator())
    -> Vector<ui32>;

} // namespace soul::gpu
