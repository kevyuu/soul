#include "gpu/type.h"

namespace soul::gpu
{
	VkAccelerationStructureBuildGeometryInfoKHR compute_as_geometry_info(const BlasBuildDesc& build_desc, RTBuildMode build_mode, VkAccelerationStructureGeometryKHR* as_geometries);
	void compute_max_primitives_counts(const BlasBuildDesc& build_desc, uint32* max_primitives_counts);
}