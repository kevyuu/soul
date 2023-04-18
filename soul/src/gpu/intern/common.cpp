#include "common.h"
#include "enum_mapping.h"

namespace soul::gpu
{

  auto compute_as_geometry_info(
    const BlasBuildDesc& build_desc,
    const RTBuildMode build_mode,
    VkAccelerationStructureGeometryKHR* as_geometries)
    -> VkAccelerationStructureBuildGeometryInfoKHR
  {
    std::ranges::transform(
      build_desc.geometry_descs,
      build_desc.geometry_descs + build_desc.geometry_count,
      as_geometries,
      [](const RTGeometryDesc& geometry_desc) -> VkAccelerationStructureGeometryKHR {
        const auto geometry_data =
          [](const RTGeometryDesc& desc) -> VkAccelerationStructureGeometryDataKHR {
          if (desc.type == RTGeometryType::TRIANGLE) {
            const auto& triangle_desc = desc.content.triangles;
            return {
              .triangles =
                {
                  .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                  .vertexFormat = vk_cast(triangle_desc.vertex_format),
                  .vertexData = {.deviceAddress = triangle_desc.vertex_data.id},
                  .vertexStride = triangle_desc.vertex_stride,
                  .maxVertex = triangle_desc.vertex_count,
                  .indexType = vk_cast(triangle_desc.index_type),
                  .indexData = {.deviceAddress = triangle_desc.index_data.id},
                  .transformData = {.deviceAddress = triangle_desc.transform_data.id},
                },
            };
          }
          SOUL_ASSERT(0, desc.type == RTGeometryType::AABB, "");
          const auto& aabb_desc = desc.content.aabbs;
          return {
            .aabbs =
              {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
                .data = {.deviceAddress = aabb_desc.data.id},
                .stride = aabb_desc.stride,
              },
          };
        }(geometry_desc);

        return {
          .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
          .geometryType = vk_cast(geometry_desc.type),
          .geometry = geometry_data,
          .flags = vk_cast(geometry_desc.flags),
        };
      });

    const VkAccelerationStructureBuildGeometryInfoKHR build_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      .flags = vk_cast(build_desc.flags),
      .mode = vk_cast(build_mode),
      .geometryCount = build_desc.geometry_count,
      .pGeometries = as_geometries,
    };

    return build_info;
  }

  auto compute_max_primitives_counts(const BlasBuildDesc& build_desc, uint32* max_primitives_counts)
    -> void
  {
    std::transform(
      build_desc.geometry_descs,
      build_desc.geometry_descs + build_desc.geometry_count,
      max_primitives_counts,
      [](const RTGeometryDesc& desc) -> uint32 {
        if (desc.type == RTGeometryType::TRIANGLE) {
          return desc.content.triangles.index_count / 3;
        }
        SOUL_ASSERT(0, desc.type == RTGeometryType::AABB, "");
        return desc.content.aabbs.count;
      });
  }

} // namespace soul::gpu
