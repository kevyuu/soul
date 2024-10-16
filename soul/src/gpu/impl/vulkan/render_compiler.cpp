#include <volk.h>

#include "core/log.h"
#include "core/profile.h"

#include "memory/util.h"
#include "runtime/scope_allocator.h"

#include "gpu/system.h"

#include "gpu/impl/vulkan/common.h"
#include "gpu/impl/vulkan/enum_mapping.h"
#include "gpu/impl/vulkan/render_compiler.h"
#include "gpu/impl/vulkan/type.h"

namespace soul::gpu::impl
{

  auto RenderCompiler::bind_descriptor_sets(VkPipelineBindPoint pipeline_bind_point) -> void
  {
    const auto pipeline_layout          = gpu_system_->get_bindless_pipeline_layout();
    const auto bindless_descriptor_sets = gpu_system_->get_bindless_descriptor_sets();
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
    const VkRenderPassBeginInfo& render_pass_begin_info,
    const VkSubpassContents subpass_contents) -> void
  {
    vkCmdBeginRenderPass(command_buffer_, &render_pass_begin_info, subpass_contents);
  }

  auto RenderCompiler::end_render_pass() -> void
  {
    vkCmdEndRenderPass(command_buffer_);
  }

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

    // NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast)
    switch (command.type)
    {
      COMPILE_PACKET(RenderCommandDraw);
      COMPILE_PACKET(RenderCommandDrawIndex);
      COMPILE_PACKET(RenderCommandDrawIndexedIndirect);
      COMPILE_PACKET(RenderCommandUpdateTexture);
      COMPILE_PACKET(RenderCommandCopyTexture);
      COMPILE_PACKET(RenderCommandClearTexture);
      COMPILE_PACKET(RenderCommandUpdateBuffer);
      COMPILE_PACKET(RenderCommandCopyBuffer);
      COMPILE_PACKET(RenderCommandDispatch);
      COMPILE_PACKET(RenderCommandBuildBlas);
      COMPILE_PACKET(RenderCommandBatchBuildBlas);
      COMPILE_PACKET(RenderCommandBuildTlas);
      COMPILE_PACKET(RenderCommandRayTrace);
    case RenderCommandType::COUNT: SOUL_NOT_IMPLEMENTED();
    }
    // NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast)
  }

  auto RenderCompiler::compile_command(const RenderCommandDraw& command) -> void
  {
    SOUL_PROFILE_ZONE();
    apply_pipeline_state(command.pipeline_state_id);
    apply_push_constant(command.push_constant_data, command.push_constant_size);

    for (u32 vert_buf_idx = 0; vert_buf_idx < MAX_VERTEX_BINDING; vert_buf_idx++)
    {
      BufferID vert_buf_id = command.vertex_buffer_i_ds[vert_buf_idx];
      if (vert_buf_id.is_null())
      {
        continue;
      }
      const Buffer& vertex_buffer = gpu_system_->buffer_ref(vert_buf_id);
      SOUL_ASSERT(0, vertex_buffer.desc.usage_flags.test(BufferUsage::VERTEX));
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

    for (u32 vert_buf_idx = 0; vert_buf_idx < MAX_VERTEX_BINDING; vert_buf_idx++)
    {
      BufferID vert_buf_id = command.vertex_buffer_ids[vert_buf_idx];
      if (vert_buf_id.is_null())
      {
        continue;
      }
      const Buffer& vertex_buffer = gpu_system_->buffer_ref(vert_buf_id);
      SOUL_ASSERT(0, vertex_buffer.desc.usage_flags.test(BufferUsage::VERTEX));
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(command_buffer_, vert_buf_idx, 1, &vertex_buffer.vk_handle, offsets);
    }

    const Buffer& index_buffer = gpu_system_->buffer_ref(command.index_buffer_id);
    SOUL_ASSERT(0, index_buffer.desc.usage_flags.test(gpu::BufferUsage::INDEX));

    vkCmdBindIndexBuffer(
      command_buffer_, index_buffer.vk_handle, command.index_offset, vk_cast(command.index_type));
    vkCmdDrawIndexed(
      command_buffer_,
      command.index_count,
      command.instance_count,
      command.first_index,
      command.vertex_offsets[0],
      command.first_instance);
  }

  void RenderCompiler::compile_command(const RenderCommandDrawIndexedIndirect& command)
  {
    SOUL_PROFILE_ZONE();
    apply_pipeline_state(command.pipeline_state_id);
    apply_push_constant(command.push_constant_data, command.push_constant_size);

    for (u32 vert_buf_idx = 0; vert_buf_idx < MAX_VERTEX_BINDING; vert_buf_idx++)
    {
      BufferID vert_buf_id = command.vertex_buffer_ids[vert_buf_idx];
      if (vert_buf_id.is_null())
      {
        continue;
      }
      const Buffer& vertex_buffer = gpu_system_->buffer_ref(vert_buf_id);
      SOUL_ASSERT(0, vertex_buffer.desc.usage_flags.test(BufferUsage::VERTEX));
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(command_buffer_, vert_buf_idx, 1, &vertex_buffer.vk_handle, offsets);
    }

    const Buffer& index_buffer = gpu_system_->buffer_ref(command.index_buffer_id);
    SOUL_ASSERT(0, index_buffer.desc.usage_flags.test(gpu::BufferUsage::INDEX));

    vkCmdBindIndexBuffer(
      command_buffer_, index_buffer.vk_handle, command.index_offset, vk_cast(command.index_type));

    const Buffer& buffer = gpu_system_->buffer_ref(command.buffer_id);
    vkCmdDrawIndexedIndirect(
      command_buffer_, buffer.vk_handle, command.offset, command.draw_count, command.stride);
  }

  auto RenderCompiler::compile_command(const RenderCommandUpdateTexture& command) -> void
  {
    runtime::ScopeAllocator scope_allocator("compile_command::RenderCommandUpdateTexture"_str);
    const Texture& dst_texture = gpu_system_->texture_ref(command.dst_texture);

    const auto gpu_allocator         = gpu_system_->get_gpu_allocator();
    const BufferID staging_buffer_id = gpu_system_->create_staging_buffer(command.data_size);
    const Buffer& staging_buffer     = gpu_system_->buffer_ref(staging_buffer_id);
    void* mapped_data; // NOLINT
    vmaMapMemory(gpu_allocator, staging_buffer.allocation, &mapped_data);
    memcpy(mapped_data, command.data, command.data_size);
    vmaUnmapMemory(gpu_allocator, staging_buffer.allocation);

    auto get_buffer_image_copy =
      [format = dst_texture.desc.format](const TextureRegionUpdate& region) -> VkBufferImageCopy
    {
      // SOUL_ASSERT(0, region.extent.z == 1, "3D texture is not supported yet");
      return {
        .bufferOffset      = region.buffer_offset,
        .bufferRowLength   = region.buffer_row_length,
        .bufferImageHeight = region.buffer_image_height,
        .imageSubresource =
          {.aspectMask     = vk_cast_format_to_aspect_flags(format),
           .mipLevel       = region.subresource.mip_level,
           .baseArrayLayer = region.subresource.base_array_layer,
           .layerCount     = region.subresource.layer_count},
        .imageOffset =
          {
            .x = region.offset.x,
            .y = region.offset.y,
            .z = region.offset.z,
          },
        .imageExtent = {
          .width  = region.extent.x,
          .height = region.extent.y,
          .depth  = region.extent.z,
        }};
    };

    const auto buffer_image_copies =
      Vector<VkBufferImageCopy>::Transform(command.regions, get_buffer_image_copy, scope_allocator);

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
    const auto& src_texture = gpu_system_->texture_ref(command.src_texture);
    const auto& dst_texture = gpu_system_->texture_ref(command.dst_texture);

    const auto src_aspect_mask = vk_cast_format_to_aspect_flags(src_texture.desc.format);
    const auto dst_aspect_mask = vk_cast_format_to_aspect_flags(dst_texture.desc.format);

    runtime::ScopeAllocator scope_allocator("compile_command copy texture"_str);

    auto to_vk_image_copy = [src_aspect_mask,
                             dst_aspect_mask](const TextureRegionCopy& copy_region) -> VkImageCopy
    {
      return {
        .srcSubresource = get_vk_subresource_layers(copy_region.src_subresource, src_aspect_mask),
        .srcOffset      = get_vk_offset_3d(copy_region.src_offset),
        .dstSubresource = get_vk_subresource_layers(copy_region.dst_subresource, dst_aspect_mask),
        .dstOffset      = get_vk_offset_3d(copy_region.dst_offset),
        .extent         = get_vk_extent_3d(copy_region.extent)};
    };
    const auto image_copies =
      Vector<VkImageCopy>::Transform(command.regions, to_vk_image_copy, scope_allocator);

    vkCmdCopyImage(
      command_buffer_,
      src_texture.vk_handle,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      dst_texture.vk_handle,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      command.regions.size(),
      image_copies.data());
  }

  void RenderCompiler::compile_command(const RenderCommandClearTexture& command)
  {
    SOUL_PROFILE_ZONE();
    const auto& dst_texture    = gpu_system_->texture_ref(command.dst_texture);
    const auto dst_aspect_mask = vk_cast_format_to_aspect_flags(dst_texture.desc.format);

    const VkImageSubresourceRange subresource_range = [&]
    {
      if (command.subresource_range.is_some())
      {
        const auto range = command.subresource_range.some_ref();
        return VkImageSubresourceRange{
          .aspectMask     = dst_aspect_mask,
          .baseMipLevel   = range.base_mip_level,
          .levelCount     = range.level_count,
          .baseArrayLayer = range.base_array_layer,
          .layerCount     = range.layer_count,
        };
      } else
      {
        return VkImageSubresourceRange{
          .aspectMask     = dst_aspect_mask,
          .baseMipLevel   = 0,
          .levelCount     = VK_REMAINING_MIP_LEVELS,
          .baseArrayLayer = 0,
          .layerCount     = VK_REMAINING_ARRAY_LAYERS,
        };
      }
    }();

    if ((dst_aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT) != 0u)
    {
      const VkClearDepthStencilValue vk_clear_value = {
        command.clear_value.depth_stencil.depth,
        command.clear_value.depth_stencil.stencil,
      };
      vkCmdClearDepthStencilImage(
        command_buffer_,
        dst_texture.vk_handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &vk_clear_value,
        1,
        &subresource_range);
    } else
    {
      VkClearColorValue vk_clear_value;
      memcpy(&vk_clear_value, &command.clear_value, sizeof(vk_clear_value));
      vkCmdClearColorImage(
        command_buffer_,
        dst_texture.vk_handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &vk_clear_value,
        1,
        &subresource_range);
    }
  }

  auto RenderCompiler::compile_command(const RenderCommandUpdateBuffer& command) -> void
  {
    const auto gpu_allocator = gpu_system_->get_gpu_allocator();
    const auto& dst_buffer   = gpu_system_->buffer_ref(command.dst_buffer);
    if ((dst_buffer.memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0u)
    {
      SOUL_ASSERT(0, dst_buffer.memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
      void* mapped_data; // NOLINT
      vmaMapMemory(gpu_allocator, dst_buffer.allocation, &mapped_data);
      for (usize region_idx = 0; region_idx < command.regions.size(); region_idx++)
      {
        const auto& region_load = command.regions[region_idx];
        memcpy(
          memory::util::pointer_add(mapped_data, region_load.dst_offset),
          memory::util::pointer_add(command.data, region_load.src_offset),
          region_load.size);
      }
      vmaUnmapMemory(gpu_allocator, dst_buffer.allocation);
    } else
    {
      for (const auto& region_load : command.regions)
      {
        const BufferID staging_buffer_id = gpu_system_->create_staging_buffer(region_load.size);
        const Buffer& staging_buffer     = gpu_system_->buffer_ref(staging_buffer_id);
        void* mapped_data; // NOLINT
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
    runtime::ScopeAllocator<> scope_allocator("compile_command::RenderCommandCopyBuffer"_str);
    const auto& src_buffer = gpu_system_->buffer_ref(command.src_buffer);
    const auto& dst_buffer = gpu_system_->buffer_ref(command.dst_buffer);

    const auto region_copies = Vector<VkBufferCopy>::Transform(
      command.regions,
      [](BufferRegionCopy region_copy) -> VkBufferCopy
      {
        return {
          .srcOffset = region_copy.src_offset,
          .dstOffset = region_copy.dst_offset,
          .size      = region_copy.size};
      },
      scope_allocator);

    vkCmdCopyBuffer(
      command_buffer_,
      src_buffer.vk_handle,
      dst_buffer.vk_handle,
      command.regions.size(),
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

  auto RenderCompiler::compile_command(const RenderCommandDispatchIndirect& command) -> void
  {
    SOUL_PROFILE_ZONE();
    apply_push_constant(command.push_constant);
    apply_pipeline_state(command.pipeline_state_id);
    vkCmdDispatchIndirect(
      command_buffer_, gpu_system_->buffer_ref(command.buffer).vk_handle, command.offset);
  }

  auto RenderCompiler::compile_command(const RenderCommandRayTrace& command) -> void
  {
    SOUL_PROFILE_ZONE();
    apply_push_constant(command.push_constant_data, command.push_constant_size);
    const auto& shader_table = gpu_system_->shader_table_ref(command.shader_table_id);
    apply_pipeline_state(shader_table.pipeline, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);
    vkCmdTraceRaysKHR(
      command_buffer_,
      &shader_table.vk_regions[ShaderGroupKind::RAYGEN],
      &shader_table.vk_regions[ShaderGroupKind::MISS],
      &shader_table.vk_regions[ShaderGroupKind::HIT],
      &shader_table.vk_regions[ShaderGroupKind::CALLABLE],
      command.dimension.x,
      command.dimension.y,
      command.dimension.z);
  }

  auto RenderCompiler::compile_command(const RenderCommandBuildTlas& command) -> void
  {
    const auto& tlas       = gpu_system_->tlas_cref(command.tlas_id);
    const auto& build_desc = command.build_desc;

    const auto size_info = gpu_system_->get_as_build_size_info(command.build_desc);

    const BufferDesc scratch_buffer_desc = {
      .size        = size_info.buildScratchSize,
      .usage_flags = {BufferUsage::AS_SCRATCH_BUFFER, BufferUsage::STORAGE},
      .queue_flags = {QueueType::COMPUTE},
      .memory_option =
        MemoryOption{
          .required = {MemoryProperty::DEVICE_LOCAL},
        },
    };
    const auto scratch_buffer_id =
      gpu_system_->create_transient_buffer("Build Tlas Scratch Buffer"_str, scratch_buffer_desc);
    const auto scratch_buffer_address = gpu_system_->get_gpu_address(scratch_buffer_id);

    const VkAccelerationStructureGeometryInstancesDataKHR as_instance = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
      .data  = {.deviceAddress = build_desc.instance_data.id},
    };

    const VkAccelerationStructureGeometryKHR as_geometry = {
      .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
      .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
      .geometry     = {.instances = as_instance},
      .flags        = vk_cast(build_desc.geometry_flags),
    };

    const VkAccelerationStructureBuildGeometryInfoKHR build_info = {
      .sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      .flags                    = vk_cast(build_desc.build_flags),
      .mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
      .dstAccelerationStructure = tlas.vk_handle,
      .geometryCount            = 1,
      .pGeometries              = &as_geometry,
      .scratchData =
        {
          .deviceAddress = scratch_buffer_address.id,
        },
    };
    const VkAccelerationStructureBuildRangeInfoKHR build_offset_info{
      build_desc.instance_count, build_desc.instance_offset, 0, 0};
    const VkAccelerationStructureBuildRangeInfoKHR* p_build_offset_info = &build_offset_info;
    vkCmdBuildAccelerationStructuresKHR(command_buffer_, 1, &build_info, &p_build_offset_info);
  }

  auto RenderCompiler::compile_command(const RenderCommandBuildBlas& command) -> void
  {
    runtime::ScopeAllocator<> scope_allocator("compile_command(const RenderCommandBuildBlas&)"_str);

    const auto& dst_blas   = gpu_system_->blas_ref(command.dst_blas_id);
    const auto& build_desc = command.build_desc;

    auto as_geometries = Vector<VkAccelerationStructureGeometryKHR>::WithSize(
      build_desc.geometry_count, scope_allocator);
    auto build_info =
      compute_as_geometry_info(build_desc, command.build_mode, as_geometries.data());

    const auto max_primitives_counts = compute_max_primitives_counts(build_desc, scope_allocator);

    const auto size_info =
      gpu_system_->get_as_build_size_info(build_info, max_primitives_counts.data());
    String as_scratch_buffer_name(&scope_allocator);
    as_scratch_buffer_name.appendf("{}_scratch_buffer", dst_blas.name);

    const BufferDesc scratch_buffer_desc = {
      .size          = size_info.buildScratchSize,
      .usage_flags   = {BufferUsage::AS_SCRATCH_BUFFER},
      .queue_flags   = {QueueType::COMPUTE},
      .memory_option = MemoryOption{.required = {MemoryProperty::DEVICE_LOCAL}},
    };
    const auto scratch_buffer_id = gpu_system_->create_transient_buffer(
      String::Format("{}_scratch_buffer", dst_blas.name), scratch_buffer_desc);
    const auto scratch_buffer_address = gpu_system_->get_gpu_address(scratch_buffer_id);

    if (!command.src_blas_id.is_null())
    {
      const auto& src_blas                = gpu_system_->blas_ref(command.src_blas_id);
      build_info.srcAccelerationStructure = src_blas.vk_handle;
    }
    build_info.dstAccelerationStructure  = dst_blas.vk_handle;
    build_info.scratchData.deviceAddress = scratch_buffer_address.id;

    const auto build_ranges = Vector<VkAccelerationStructureBuildRangeInfoKHR>::Transform(
      max_primitives_counts,
      [](u32 count) -> VkAccelerationStructureBuildRangeInfoKHR
      {
        return {.primitiveCount = count};
      },
      scope_allocator);
    const VkAccelerationStructureBuildRangeInfoKHR* build_range_info = build_ranges.data();
    vkCmdBuildAccelerationStructuresKHR(command_buffer_, 1, &build_info, &build_range_info);
  }

  auto RenderCompiler::compile_command(const RenderCommandBatchBuildBlas& command) -> void
  {
    runtime::ScopeAllocator<> scope_allocator(
      "compile_command(const RenderCommandBatchBuildBlas&)"_str);
    Vector<VkAccelerationStructureBuildGeometryInfoKHR> build_infos(&scope_allocator);
    build_infos.reserve(command.builds.size());

    using ASGeometryList = SBOVector<VkAccelerationStructureGeometryKHR, 1>;
    Vector<ASGeometryList> as_geometry_list_vec(&scope_allocator);
    as_geometry_list_vec.reserve(command.builds.size());

    Vector<VkAccelerationStructureBuildRangeInfoKHR*> build_range_list_vec(&scope_allocator);
    build_range_list_vec.reserve(command.builds.size());

    Vector<usize> build_scratch_sizes(&scope_allocator);
    build_scratch_sizes.reserve(command.builds.size());

    auto total_size = 0u;
    for (const auto& blas_build : command.builds)
    {
      const auto& build_desc = blas_build.build_desc;
      as_geometry_list_vec.generate_back(
        [&]
        {
          return ASGeometryList::WithSize(build_desc.geometry_count, scope_allocator);
        });
      auto build_info = compute_as_geometry_info(
        build_desc, blas_build.build_mode, as_geometry_list_vec.back().data());

      const auto max_primitives_counts = compute_max_primitives_counts(build_desc, scope_allocator);

      const auto& dst_blas                = gpu_system_->blas_ref(blas_build.dst_blas_id);
      build_info.dstAccelerationStructure = dst_blas.vk_handle;
      if (!blas_build.src_blas_id.is_null())
      {
        const auto& src_blas                = gpu_system_->blas_ref(blas_build.src_blas_id);
        build_info.srcAccelerationStructure = src_blas.vk_handle;
      }
      build_infos.push_back(build_info);

      build_range_list_vec.push_back(
        scope_allocator.allocate_array<VkAccelerationStructureBuildRangeInfoKHR>(
          build_desc.geometry_count));
      std::ranges::transform(
        max_primitives_counts,
        build_range_list_vec.back(),
        [](u32 count) -> VkAccelerationStructureBuildRangeInfoKHR
        {
          return {.primitiveCount = count};
        });

      const auto size_info =
        gpu_system_->get_as_build_size_info(build_info, max_primitives_counts.data());
      const auto scratch_size = blas_build.build_mode == RTBuildMode::REBUILD
                                  ? size_info.buildScratchSize
                                  : size_info.updateScratchSize;
      SOUL_ASSERT(
        scratch_size < command.max_build_memory_size, "Scratch size exeeds max_build_memory_size");
      build_scratch_sizes.push_back(scratch_size);
      total_size += scratch_size;
    }

    const auto scratch_buffer_size = std::min<usize>(command.max_build_memory_size, total_size);
    const BufferDesc scratch_buffer_desc = {
      .size          = scratch_buffer_size,
      .usage_flags   = {BufferUsage::AS_SCRATCH_BUFFER},
      .queue_flags   = {QueueType::COMPUTE},
      .memory_option = MemoryOption{.required = {MemoryProperty::DEVICE_LOCAL}},
    };
    const auto scratch_buffer = gpu_system_->create_transient_buffer(
      "Batch Blas Build Scratch Buffer"_str, scratch_buffer_desc);
    const auto scratch_buffer_addr = gpu_system_->get_gpu_address(scratch_buffer).id;

    u32 current_build_base_idx       = 0;
    u32 current_build_count          = 0;
    usize current_build_scratch_size = 0;

    for (u32 build_idx = 0; build_idx < command.builds.size(); build_idx++)
    {
      auto current_scratch_size = build_scratch_sizes[build_idx];
      if (current_build_scratch_size + current_scratch_size > command.max_build_memory_size)
      {
        vkCmdBuildAccelerationStructuresKHR(
          command_buffer_,
          current_build_count,
          build_infos.data() + current_build_base_idx,
          build_range_list_vec.data() + current_build_base_idx);
        current_build_count        = 0;
        current_build_base_idx     = build_idx;
        current_build_scratch_size = 0;

        const VkBufferMemoryBarrier mem_barrier = {
          .sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
          .srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |
                           VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
          .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |
                           VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .buffer              = gpu_system_->buffer_ref(scratch_buffer).vk_handle,
          .offset              = 0,
          .size                = VK_WHOLE_SIZE,
        };
        vkCmdPipelineBarrier(
          command_buffer_,
          VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
          VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
          0,
          0,
          nullptr,
          1,
          &mem_barrier,
          0,
          nullptr);
      }
      build_infos[build_idx].scratchData.deviceAddress =
        scratch_buffer_addr + current_build_scratch_size;
      current_build_count++;
      current_build_scratch_size += current_scratch_size;
    }

    if (current_build_count != 0)
    {
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
    SOUL_ASSERT(0, !pipeline_state_id.is_null());
    const PipelineState& pipeline_state = gpu_system_->get_pipeline_state(pipeline_state_id);
    apply_pipeline_state(pipeline_state.vk_handle, pipeline_state.bind_point);
  }

  auto RenderCompiler::apply_pipeline_state(
    VkPipeline pipeline, VkPipelineBindPoint pipeline_bind_point) -> void
  {
    if (pipeline != current_pipeline_)
    {
      vkCmdBindPipeline(command_buffer_, pipeline_bind_point, pipeline);
      current_pipeline_ = pipeline;
    }
  }

  auto RenderCompiler::apply_push_constant(const void* push_constant_data, u32 push_constant_size)
    -> void
  {
    SOUL_ASSERT(0, push_constant_size <= PUSH_CONSTANT_SIZE, "");
    if (push_constant_data == nullptr)
    {
      return;
    }
    SOUL_PROFILE_ZONE();
    vkCmdPushConstants(
      command_buffer_,
      gpu_system_->get_bindless_pipeline_layout(),
      VK_SHADER_STAGE_ALL,
      0,
      push_constant_size,
      push_constant_data);
  }

  void RenderCompiler::apply_push_constant(Span<const byte*> push_constant)
  {
    SOUL_ASSERT(0, push_constant.size_in_bytes() <= PUSH_CONSTANT_SIZE, "");
    if (push_constant.data() == nullptr)
    {
      return;
    }
    SOUL_PROFILE_ZONE();
    vkCmdPushConstants(
      command_buffer_,
      gpu_system_->get_bindless_pipeline_layout(),
      VK_SHADER_STAGE_ALL,
      0,
      push_constant.size(),
      push_constant.data());
  }
} // namespace soul::gpu::impl
