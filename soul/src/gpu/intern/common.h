#include "gpu/type.h"

namespace soul::gpu
{

  auto compute_as_geometry_info(
    const BlasBuildDesc& build_desc,
    RTBuildMode build_mode,
    VkAccelerationStructureGeometryKHR* as_geometries)
    -> VkAccelerationStructureBuildGeometryInfoKHR;

  auto compute_max_primitives_counts(const BlasBuildDesc& build_desc, uint32* max_primitives_counts)
    -> void;

} // namespace soul::gpu
