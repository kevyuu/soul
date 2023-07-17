#include <volk.h>
#include <vulkan/vulkan_core.h>

#include "core/log.h"
#include "gpu/intern/common.h"
#include "gpu/intern/enum_mapping.h"
#include "gpu/intern/render_compiler.h"
#include "gpu/system.h"
#include "memory/util.h"
#include "runtime/scope_allocator.h"

namespace soul::gpu::impl
{

  auto RenderCompiler::bind_descriptor_sets(VkPipelineBindPoint pipeline_bind_point) -> void
  {
    const auto pipeline_layout = gpu_system_.get_bindless_pipeline_layout();
    const auto bindless_descriptor_sets = gpu_system_.get_bindless_descriptor_sets();
    vkCmdBindDescriptorSets(
      command_buffer_,
      pipeline_bind_point,
      pipeline_layout,
      0,
      BINDLESS_SET_COUNT,
      bindless_descriptor_sets.vk_handles,
      0,
      nullptr);
  }

  auto RenderCompiler::begin_render_pass(
    const VkRenderPassBeginInfo& render_pass_begin_info, const VkSubpassContents subpass_contents)
    -> void
  {
    vkCmdBeginRenderPass(command_buffer_, &render_pass_begin_info, subpass_contents);
  }

  auto RenderCompiler::end_render_pass() -> void { vkCmdEndRenderPass(command_buffer_); }

  auto RenderCompiler::execute_secondary_command_buffers(
    uint32_t count, const SecondaryCommandBuffer* secondary_command_buffers) -> void
  {
    static_assert(sizeof(SecondaryCommandBuffer) == sizeof(VkCommandBuffer));
    const auto* command_buffers =
      reinterpret_cast<const VkCommandBuffer*>(secondary_command_buffers);
    vkCmdExecuteCommands(command_buffer_, count, command_buffers);
  }

  auto RenderCompiler::compile_command(const RenderCommand& command) -> void
  {
    SOUL_PROFILE_ZONE();
#define COMPILE_PACKET(TYPE_STRUCT)                                                                \
  case TYPE_STRUCT::TYPE: compile_command(*static_cast<const TYPE_STRUCT*>(&command)); break

    switch (command.type) {
      COMPILE_PACKET(RenderCommandDraw);
      COMPILE_PACKET(RenderCommandDrawIndex);
      COMPILE_PACKET(RenderCommandUpdateTexture);
      COMPILE_PACKET(RenderCommandCopyTexture);
      COMPILE_PACKET(RenderCommandUpdateBuffer);
      COMPILE_PACKET(RenderCommandCopyBuffer);
      COMPILE_PACKET(RenderCommandDispatch);
    case RenderCommandType::COUNT: SOUL_NOT_IMPLEMENTED();
    }
  }

  auto RenderCompiler::compile_command(const RenderCommandDraw& command) -> void
  {
    SOUL_PROFILE_ZONE();
    apply_pipeline_state(command.pipeline_state_id);
    apply_push_constant(command.push_constant_data, command.push_constant_size);

    for (u32 vert_buf_idx = 0; vert_buf_idx < MAX_VERTEX_BINDING; vert_buf_idx++) {
      BufferID vert_buf_id = command.vertex_buffer_i_ds[vert_buf_idx];
      if (vert_buf_id.is_null()) {
        continue;
      }
      const Buffer& vertex_buffer = gpu_system_.get_buffer(vert_buf_id);
      SOUL_ASSERT(0, vertex_buffer.desc.usage_flags.test(BufferUsage::VERTEX), "");
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(command_buffer_, vert_buf_idx, 1, &vertex_buffer.vk_handle, offsets);
    }
    vkCmdDraw(
      command_buffer_,
      command.vertex_count,
      command.instance_count,
      command.first_vertex,
      command.first_instance);
  }

  auto RenderCompiler::compile_command(const RenderCommandDrawIndex& command) -> void
  {
    SOUL_PROFILE_ZONE();
    apply_pipeline_state(command.pipeline_state_id);
    apply_push_constant(command.push_constant_data, command.push_constant_size);

    for (u32 vert_buf_idx = 0; vert_buf_idx < MAX_VERTEX_BINDING; vert_buf_idx++) {
      BufferID vert_buf_id = command.vertex_buffer_ids[vert_buf_idx];
      if (vert_buf_id.is_null()) {
        continue;
      }
      const Buffer& vertex_buffer = gpu_system_.get_buffer(vert_buf_id);
      SOUL_ASSERT(0, vertex_buffer.desc.usage_flags.test(BufferUsage::VERTEX), "");
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(command_buffer_, vert_buf_idx, 1, &vertex_buffer.vk_handle, offsets);
    }

    const Buffer& index_buffer = gpu_system_.get_buffer(command.index_buffer_id);
    SOUL_ASSERT(0, index_buffer.desc.usage_flags.test(gpu::BufferUsage::INDEX), "");

    vkCmdBindIndexBuffer(
      command_buffer_, index_buffer.vk_handle, command.index_offset, vk_cast(command.index_type));
    vkCmdDrawIndexed(
      command_buffer_, command.index_count, 1, command.first_index, command.vertex_offsets[0], 1);
  }

  auto RenderCompiler::compile_command(const RenderCommandUpdateTexture& command) -> void
  {
    runtime::ScopeAllocator scope_allocator("compile_command::RenderCommandUpdateTexture");
    const Texture& dst_texture = gpu_system_.get_texture(command.dst_texture);

    const auto gpu_allocator = gpu_system_.get_gpu_allocator();
    const BufferID staging_buffer_id = gpu_system_.create_staging_buffer(command.data_size);
    const Buffer& staging_buffer = gpu_system_.get_buffer(staging_buffer_id);
    void* mapped_data;
    vmaMapMemory(gpu_allocator, staging_buffer.allocation, &mapped_data);
    memcpy(mapped_data, command.data, command.data_size);
    vmaUnmapMemory(gpu_allocator, staging_buffer.allocation);

    auto get_buffer_image_copy =
      [format = dst_texture.desc.format](const TextureRegionUpdate& region) -> VkBufferImageCopy {
      // SOUL_ASSERT(0, region.extent.z == 1, "3D texture is not supported yet");
      return {
        .bufferOffset = region.buffer_offset,
        .bufferRowLength = region.buffer_row_length,
        .bufferImageHeight = region.buffer_image_height,
        .imageSubresource =
          {.aspectMask = vk_cast_format_to_aspect_flags(format),
           .mipLevel = region.subresource.mip_level,
           .baseArrayLayer = region.subresource.base_array_layer,
           .layerCount = region.subresource.layer_count},
        .imageOffset =
          {
            .x = region.offset.x,
            .y = region.offset.y,
            .z = region.offset.z,
          },
        .imageExtent = {
          .width = region.extent.x,
          .height = region.extent.y,
          .depth = region.extent.z,
        }};
    };

    const auto buffer_image_copies = Vector<VkBufferImageCopy>::transform(
      std::span(command.regions, command.region_count), get_buffer_image_copy, scope_allocator);

    vkCmdCopyBufferToImage(
      command_buffer_,
      staging_buffer.vk_handle,
      dst_texture.vk_handle,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      soul::cast<u32>(buffer_image_copies.size()),
      buffer_image_copies.data());
  }

  auto RenderCompiler::compile_command(const RenderCommandCopyTexture& command) -> void
  {
    SOUL_PROFILE_ZONE();
    const auto& src_texture = gpu_system_.get_texture(command.src_texture);
    const auto& dst_texture = gpu_system_.get_texture(command.dst_texture);

    const auto src_aspect_mask = vk_cast_format_to_aspect_flags(src_texture.desc.format);
    const auto dst_aspect_mask = vk_cast_format_to_aspect_flags(dst_texture.desc.format);

    runtime::ScopeAllocator scope_allocator("compile_command copy texture");

    auto to_vk_image_copy = [src_aspect_mask,
                             dst_aspect_mask](const TextureRegionCopy& copy_region) -> VkImageCopy {
      return {
        .srcSubresource = get_vk_subresource_layers(copy_region.src_subresource, src_aspect_mask),
        .srcOffset = get_vk_offset_3d(copy_region.src_offset),
        .dstSubresource = get_vk_subresource_layers(copy_region.dst_subresource, dst_aspect_mask),
        .dstOffset = get_vk_offset_3d(copy_region.dst_offset),
        .extent = get_vk_extent_3d(copy_region.extent)};
    };
    const auto image_copies = Vector<VkImageCopy>::transform(
      std::span(command.regions, command.region_count), to_vk_image_copy, scope_allocator);

    vkCmdCopyImage(
      command_buffer_,
      src_texture.vk_handle,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      dst_texture.vk_handle,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      command.region_count,
      image_copies.data());
  }

  auto RenderCompiler::compile_command(const RenderCommandUpdateBuffer& command) -> void
  {
    const auto gpu_allocator = gpu_system_.get_gpu_allocator();
    const auto& dst_buffer = gpu_system_.get_buffer(command.dst_buffer);
    if (dst_buffer.memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
      SOUL_ASSERT(0, dst_buffer.memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "");
      void* mapped_data;
      vmaMapMemory(gpu_allocator, dst_buffer.allocation, &mapped_data);
      for (usize region_idx = 0; region_idx < command.region_count; region_idx++) {
        const auto& region_load = command.regions[region_idx];
        memcpy(
          memory::util::pointer_add(mapped_data, region_load.dst_offset),
          memory::util::pointer_add(command.data, region_load.src_offset),
          region_load.size);
      }
      vmaUnmapMemory(gpu_allocator, dst_buffer.allocation);
    } else {
      for (usize region_idx = 0; region_idx < command.region_count; region_idx++) {
        const auto& region_load = command.regions[region_idx];
        const BufferID staging_buffer_id = gpu_system_.create_staging_buffer(region_load.size);
        const Buffer& staging_buffer = gpu_system_.get_buffer(staging_buffer_id);
        void* mapped_data;
        vmaMapMemory(gpu_allocator, staging_buffer.allocation, &mapped_data);
        memcpy(
          mapped_data,
          memory::util::pointer_add(command.data, region_load.src_offset),
          region_load.size);
        vmaUnmapMemory(gpu_allocator, staging_buffer.allocation);
        const VkBufferCopy copy_region = {
          .srcOffset = 0, .dstOffset = region_load.dst_offset, .size = region_load.size};
        vkCmdCopyBuffer(
          command_buffer_, staging_buffer.vk_handle, dst_buffer.vk_handle, 1, &copy_region);
      }
    }
  }

  auto RenderCompiler::compile_command(const RenderCommandCopyBuffer& command) -> void
  {
    runtime::ScopeAllocator<> scope_allocator("compile_command::RenderCommandCopyBuffer");
    const auto src_buffer = gpu_system_.get_buffer(command.src_buffer);
    const auto dst_buffer = gpu_system_.get_buffer(command.dst_buffer);

    const auto region_copies = Vector<VkBufferCopy>::transform(
      std::span(command.regions, command.region_count),
      [](BufferRegionCopy region_copy) -> VkBufferCopy {
        return {
          .srcOffset = region_copy.src_offset,
          .dstOffset = region_copy.dst_offset,
          .size = region_copy.size};
      },
      scope_allocator);

    vkCmdCopyBuffer(
      command_buffer_,
      src_buffer.vk_handle,
      dst_buffer.vk_handle,
      command.region_count,
      region_copies.data());
  }

  auto RenderCompiler::compile_command(const RenderCommandDispatch& command) -> void
  {
    SOUL_PROFILE_ZONE();
    apply_push_constant(command.push_constant_data, command.push_constant_size);
    apply_pipeline_state(command.pipeline_state_id);
    vkCmdDispatch(
      command_buffer_, command.group_count.x, command.group_count.y, command.group_count.z);
  }

  auto RenderCompiler::compile_command(const RenderCommandRayTrace& command) -> void
  {
    SOUL_PROFILE_ZONE();
    apply_push_constant(command.push_constant_data, command.push_constant_size);
    const auto& shader_table = gpu_system_.get_shader_table(command.shader_table_id);
    apply_pipeline_state(shader_table.pipeline, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);
    vkCmdTraceRaysKHR(
      command_buffer_,
      &shader_table.vk_regions[ShaderGroup::RAYGEN],
      &shader_table.vk_regions[ShaderGroup::MISS],
      &shader_table.vk_regions[ShaderGroup::HIT],
      &shader_table.vk_regions[ShaderGroup::CALLABLE],
      command.dimension.x,
      command.dimension.y,
      command.dimension.z);
  }

  auto RenderCompiler::compile_command(const RenderCommandBuildTlas& command) -> void
  {
    const auto& tlas = gpu_system_.get_tlas(command.tlas_id);
    const auto& build_desc = command.build_desc;

    const auto size_info = gpu_system_.get_as_build_size_info(command.build_desc);

    const BufferDesc scratch_buffer_desc = {
      .size = size_info.buildScratchSize,
      .usage_flags = {BufferUsage::AS_SCRATCH_BUFFER},
      .queue_flags = {QueueType::COMPUTE},
      .memory_option =
        MemoryOption{
          .required = {MemoryProperty::DEVICE_LOCAL},
        },
    };
    const auto scratch_buffer_id = gpu_system_.create_transient_buffer(scratch_buffer_desc);
    const auto scratch_buffer_address = gpu_system_.get_gpu_address(scratch_buffer_id);

    const VkAccelerationStructureGeometryInstancesDataKHR as_instance = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
      .data = {.deviceAddress = build_desc.instance_data.id},
    };

    const VkAccelerationStructureGeometryKHR as_geometry = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
      .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
      .geometry = {.instances = as_instance},
      .flags = vk_cast(build_desc.geometry_flags),
    };

    const VkAccelerationStructureBuildGeometryInfoKHR build_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      .flags = vk_cast(build_desc.build_flags),
      .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
      .dstAccelerationStructure = tlas.vk_handle,
      .geometryCount = 1,
      .pGeometries = &as_geometry,
      .scratchData = {.deviceAddress = scratch_buffer_address.id},
    };
    const VkAccelerationStructureBuildRangeInfoKHR build_offset_info{
      build_desc.instance_count, build_desc.instance_offset, 0, 0};
    const VkAccelerationStructureBuildRangeInfoKHR* p_build_offset_info = &build_offset_info;
    vkCmdBuildAccelerationStructuresKHR(command_buffer_, 1, &build_info, &p_build_offset_info);
  }

  auto RenderCompiler::compile_command(const RenderCommandBuildBlas& command) -> void
  {
    runtime::ScopeAllocator<> scope_allocator("compile_command(const RenderCommandBuildBlas&)");

    const auto& dst_blas = gpu_system_.get_blas(command.dst_blas_id);
    const auto& build_desc = command.build_desc;

    auto as_geometries = Vector<VkAccelerationStructureGeometryKHR>::with_size(
      build_desc.geometry_count, scope_allocator);
    auto build_info =
      compute_as_geometry_info(build_desc, command.build_mode, as_geometries.data());

    const auto max_primitives_counts = compute_max_primitives_counts(build_desc, scope_allocator);

    const auto size_info =
      gpu_system_.get_as_build_size_info(build_info, max_primitives_counts.data());
    CString as_scratch_buffer_name(&scope_allocator);
    if (dst_blas.desc.name != nullptr) {
      as_scratch_buffer_name.appendf("{}_scratch_buffer", dst_blas.desc.name);
    }

    const BufferDesc scratch_buffer_desc = {
      .size = size_info.buildScratchSize,
      .usage_flags = {BufferUsage::AS_SCRATCH_BUFFER},
      .queue_flags = {QueueType::COMPUTE},
      .memory_option = MemoryOption{.required = {MemoryProperty::DEVICE_LOCAL}},
      .name = as_scratch_buffer_name.data(),
    };
    const auto scratch_buffer_id = gpu_system_.create_transient_buffer(scratch_buffer_desc);
    const auto scratch_buffer_address = gpu_system_.get_gpu_address(scratch_buffer_id);

    if (command.src_blas_id.is_valid()) {
      const auto& src_blas = gpu_system_.get_blas(command.src_blas_id);
      build_info.srcAccelerationStructure = src_blas.vk_handle;
    }
    build_info.dstAccelerationStructure = dst_blas.vk_handle;
    build_info.scratchData.deviceAddress = scratch_buffer_address.id;

    const auto build_ranges = Vector<VkAccelerationStructureBuildRangeInfoKHR>::transform(
      max_primitives_counts,
      [](u32 count) -> VkAccelerationStructureBuildRangeInfoKHR {
        return {.primitiveCount = count};
      },
      scope_allocator);
    const VkAccelerationStructureBuildRangeInfoKHR* build_range_info = build_ranges.data();
    vkCmdBuildAccelerationStructuresKHR(command_buffer_, 1, &build_info, &build_range_info);
  }

  auto RenderCompiler::compile_command(const RenderCommandBatchBuildBlas& command) -> void
  {
    runtime::ScopeAllocator<> scope_allocator(
      "compile_command(const RenderCommandBatchBuildBlas&)");
    Vector<VkAccelerationStructureBuildGeometryInfoKHR> build_infos(&scope_allocator);
    build_infos.reserve(command.build_count);

    using ASGeometryList = SBOVector<VkAccelerationStructureGeometryKHR, 1>;
    Vector<ASGeometryList> as_geometry_list_vec(&scope_allocator);
    as_geometry_list_vec.reserve(command.build_count);

    Vector<VkAccelerationStructureBuildRangeInfoKHR*> build_range_list_vec(&scope_allocator);
    build_range_list_vec.reserve(command.build_count);

    Vector<usize> build_scratch_sizes(&scope_allocator);
    build_scratch_sizes.reserve(command.build_count);

    auto total_size = 0u;
    for (const auto& blas_build : std::span(command.builds, command.build_count)) {
      const auto& build_desc = blas_build.build_desc;
      as_geometry_list_vec.generate_back(
        [&] { return ASGeometryList::with_size(build_desc.geometry_count, scope_allocator); });
      auto build_info = compute_as_geometry_info(
        build_desc, blas_build.build_mode, as_geometry_list_vec.back().data());

      const auto max_primitives_counts = compute_max_primitives_counts(build_desc, scope_allocator);

      const auto& dst_blas = gpu_system_.get_blas(blas_build.dst_blas_id);
      if (blas_build.src_blas_id.is_valid()) {
        const auto& src_blas = gpu_system_.get_blas(blas_build.src_blas_id);
        build_info.srcAccelerationStructure = src_blas.vk_handle;
      }
      build_info.dstAccelerationStructure = dst_blas.vk_handle;
      build_infos.push_back(build_info);

      build_range_list_vec.push_back(
        scope_allocator.allocate_array<VkAccelerationStructureBuildRangeInfoKHR>(
          build_desc.geometry_count));
      std::ranges::transform(
        max_primitives_counts,
        build_range_list_vec.back(),
        [](u32 count) -> VkAccelerationStructureBuildRangeInfoKHR {
          return {.primitiveCount = count};
        });

      const auto size_info =
        gpu_system_.get_as_build_size_info(build_info, max_primitives_counts.data());
      const auto scratch_size = blas_build.build_mode == RTBuildMode::REBUILD
                                  ? size_info.buildScratchSize
                                  : size_info.updateScratchSize;
      build_scratch_sizes.push_back(scratch_size);
      SOUL_LOG_INFO("Scratch size = {}", scratch_size);
      total_size += scratch_size;
    }

    const auto scratch_buffer_size = std::min<usize>(command.max_build_memory_size, total_size);
    const BufferDesc scratch_buffer_desc = {
      .size = scratch_buffer_size,
      .usage_flags = {BufferUsage::AS_SCRATCH_BUFFER},
      .queue_flags = {QueueType::COMPUTE},
      .memory_option = MemoryOption{.required = {MemoryProperty::DEVICE_LOCAL}},
      .name = "Batch blas scratch buffer",
    };
    const auto scratch_buffer = gpu_system_.create_transient_buffer(scratch_buffer_desc);
    const auto scratch_buffer_addr = gpu_system_.get_gpu_address(scratch_buffer).id;

    u32 current_build_base_idx = 0;
    u32 current_build_count = 0;
    usize current_build_scratch_size = 0;

    for (u32 build_idx = 0; build_idx < command.build_count; build_idx++) {
      auto current_scratch_size = build_scratch_sizes[build_idx];
      if (current_build_scratch_size + current_scratch_size > command.max_build_memory_size) {
        vkCmdBuildAccelerationStructuresKHR(
          command_buffer_,
          current_build_count,
          build_infos.data() + current_build_base_idx,
          build_range_list_vec.data() + current_build_base_idx);
        current_build_count = 0;
        current_build_base_idx = build_idx;
        current_build_scratch_size = 0;
      }
      build_infos[build_idx].scratchData.deviceAddress =
        scratch_buffer_addr + current_build_scratch_size;
      current_build_count++;
      current_build_scratch_size += current_scratch_size;
    }

    if (current_build_count != 0) {
      vkCmdBuildAccelerationStructuresKHR(
        command_buffer_,
        current_build_count,
        build_infos.data() + current_build_base_idx,
        build_range_list_vec.data() + current_build_base_idx);
    }
  }

  auto RenderCompiler::apply_pipeline_state(PipelineStateID pipeline_state_id) -> void
  {
    SOUL_PROFILE_ZONE();
    SOUL_ASSERT(0, pipeline_state_id.is_valid(), "");
    const PipelineState& pipeline_state = gpu_system_.get_pipeline_state(pipeline_state_id);
    apply_pipeline_state(pipeline_state.vk_handle, pipeline_state.bind_point);
  }

  auto RenderCompiler::apply_pipeline_state(
    VkPipeline pipeline, VkPipelineBindPoint pipeline_bind_point) -> void
  {
    if (pipeline != current_pipeline_) {
      vkCmdBindPipeline(command_buffer_, pipeline_bind_point, pipeline);
      current_pipeline_ = pipeline;
    }
  }

  auto RenderCompiler::apply_push_constant(const void* push_constant_data, u32 push_constant_size)
    -> void
  {
    if (push_constant_data == nullptr) {
      return;
    }
    SOUL_PROFILE_ZONE();
    vkCmdPushConstants(
      command_buffer_,
      gpu_system_.get_bindless_pipeline_layout(),
      VK_SHADER_STAGE_ALL,
      0,
      push_constant_size,
      push_constant_data);
  }

} // namespace soul::gpu::impl
