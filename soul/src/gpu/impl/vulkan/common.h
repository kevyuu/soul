#include "core/config.h"

#include "memory/allocator.h"

#include "gpu/impl/vulkan/type.h"

namespace soul::gpu
{

  auto compute_as_geometry_info(
    const BlasBuildDesc& build_desc,
    RTBuildMode build_mode,
    VkAccelerationStructureGeometryKHR* as_geometries)
    -> VkAccelerationStructureBuildGeometryInfoKHR;

  auto compute_max_primitives_counts(
    const BlasBuildDesc& build_desc, memory::Allocator& allocator = *get_default_allocator())
    -> Vector<u32>;

} // namespace soul::gpu
