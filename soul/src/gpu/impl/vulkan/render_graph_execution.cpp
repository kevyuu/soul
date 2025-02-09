#include <numeric>
#include <volk.h>

#include "core/compiler.h"

#include "runtime/runtime.h"
#include "runtime/scope_allocator.h"

#include "gpu/render_graph.h"
#include "gpu/render_graph_registry.h"
#include "gpu/system.h"
#include "gpu/type.h"

#include "gpu/impl/vulkan/enum_mapping.h"
#include "gpu/impl/vulkan/render_compiler.h"
#include "gpu/impl/vulkan/render_graph_execution.h"
#include "gpu/impl/vulkan/type.h"
#include "gpu/impl/vulkan/vk_str.h"

#if defined(RENDER_GRAPH_EXECUTION_LOG_ENABLE)
#  include "core/log.h"
#  define SOUL_LOG_RG_EXEC(...) soul::impl::log_info(__FILE__, __LINE__, ##__VA_ARGS__)
#else
#  define SOUL_LOG_RG_EXEC(...) SOUL_NOOP
#endif

namespace soul::gpu::impl
{

  constexpr auto SHADER_BUFFER_READ_USAGE_MAP =
    FlagMap<ShaderBufferReadUsage, BufferUsageFlags>::FromValues(
      {BufferUsageFlags({BufferUsage::UNIFORM}), {BufferUsage::STORAGE}});

  constexpr auto get_buffer_usage_flags(ShaderBufferReadUsage usage) -> BufferUsageFlags
  {
    return SHADER_BUFFER_READ_USAGE_MAP[usage];
  }

  using ShaderBufferWriteMap = FlagMap<ShaderBufferWriteUsage, BufferUsageFlags>;
  constexpr auto SHADER_BUFFER_WRITE_USAGE_MAP =
    ShaderBufferWriteMap::FromValues({{BufferUsage::UNIFORM}, {BufferUsage::STORAGE}});

  constexpr auto get_buffer_usage_flags(ShaderBufferWriteUsage usage) -> BufferUsageFlags
  {
    return SHADER_BUFFER_WRITE_USAGE_MAP[usage];
  }

  constexpr auto SHADER_TEXTURE_READ_USAGE_MAP =
    FlagMap<ShaderTextureReadUsage, TextureUsageFlags>::FromValues(
      {{TextureUsage::SAMPLED}, {TextureUsage::STORAGE}});

  constexpr auto get_texture_usage_flags(ShaderTextureReadUsage usage) -> TextureUsageFlags
  {
    return SHADER_TEXTURE_READ_USAGE_MAP[usage];
  }

  using ShaderTextureWriteMap = FlagMap<ShaderTextureWriteUsage, TextureUsageFlags>;
  constexpr auto SHADER_TEXTURE_WRITE_USAGE_MAP =
    ShaderTextureWriteMap::FromValues({{TextureUsage::STORAGE}});

  constexpr auto get_texture_usage_flags(ShaderTextureWriteUsage usage) -> TextureUsageFlags
  {
    return SHADER_TEXTURE_WRITE_USAGE_MAP[usage];
  }

  void update_buffer_info(
    const QueueType queue_type,
    const BufferUsageFlags usage_flags,
    const PassNodeID pass_id,
    BufferExecInfo* buffer_info)
  {
    buffer_info->usage_flags |= usage_flags;
    buffer_info->queue_flags |= {queue_type};
    if (buffer_info->first_pass.is_null())
    {
      buffer_info->first_pass = pass_id;
    }
    buffer_info->last_pass = pass_id;
    buffer_info->passes.push_back(pass_id);
  };

  void update_texture_info(
    const QueueType queue_type,
    const TextureUsageFlags usage_flags,
    const PassNodeID pass_id,
    const SubresourceIndexRange view_index_range,
    TextureExecInfo* texture_info)
  {
    texture_info->usage_flags |= usage_flags;
    texture_info->queue_flags |= {queue_type};
    if (texture_info->first_pass.is_null())
    {
      texture_info->first_pass = pass_id;
    }
    texture_info->last_pass = pass_id;

    std::ranges::for_each(
      view_index_range,
      [pass_id, texture_info](SubresourceIndex view_index)
      {
        texture_info->get_view(view_index)->passes.push_back(pass_id);
      });
    SOUL_ASSERT(0, !texture_info->view->passes.empty());
  };

  void update_resource_info(
    const QueueType queue_type, const PassNodeID pass_id, ResourceExecInfo* resource_info)
  {
    resource_info->queue_flags |= {queue_type};
    if (resource_info->first_pass.is_null())
    {
      resource_info->first_pass = pass_id;
    }
    resource_info->last_pass = pass_id;
    resource_info->passes.push_back(pass_id);
  }

  PassDependencyGraph::PassDependencyGraph(
    usize pass_node_count, std::span<const ResourceNode> resource_nodes)
      : pass_node_count_(pass_node_count),
        dependencies_(Vector<NodeList>::WithSize(pass_node_count)),
        dependants_(Vector<NodeList>::WithSize(pass_node_count)),
        dependency_levels_(Vector<usize>::WithSize(pass_node_count))
  {
    for (auto& dependency_matrix : dependency_matrixes_)
    {
      dependency_matrix.resize(pass_node_count * pass_node_count);
    }

    for (const auto& resource_node : resource_nodes)
    {

      for (const auto pass_node_id : resource_node.readers)
      {
        set_dependency(resource_node.creator, pass_node_id, DependencyType::READ_AFTER_WRITE);
      }

      if (resource_node.writer.is_valid())
      {
        set_dependency(
          resource_node.creator, resource_node.writer, DependencyType::WRITE_AFTER_WRITE);

        for (const auto pass_node_id : resource_node.readers)
        {
          set_dependency(pass_node_id, resource_node.writer, DependencyType::WRITE_AFTER_READ);
        }
      }
    }

    std::ranges::fill(dependency_levels_, UNINITIALIZED_DEPENDENCY_LEVEL);
    for (usize pass_index = 0; pass_index < pass_node_count; pass_index++)
    {
      calculate_dependency_level(PassNodeID(pass_index));
    }
  }

  auto PassDependencyGraph::get_dependency_flags(
    const PassNodeID src_node_id, const PassNodeID dst_node_id) const -> DependencyFlags
  {
    DependencyFlags dependency_flags;
    const auto matrix_index = get_dependency_matrix_index(src_node_id, dst_node_id);
    for (const auto dependency_type : FlagIter<DependencyType>())
    {
      if (dependency_matrixes_[dependency_type][matrix_index])
      {
        dependency_flags.set(dependency_type);
      }
    }

    return dependency_flags;
  }

  void PassDependencyGraph::set_dependency(
    const PassNodeID src_node_id,
    const PassNodeID dst_node_id,
    const DependencyType dependency_type)
  {
    if (src_node_id.is_null() || dst_node_id.is_null())
    {
      return;
    }

    if (get_dependency_flags(src_node_id, dst_node_id).none())
    {
      dependencies_[dst_node_id.id].push_back(src_node_id);
      dependants_[src_node_id.id].push_back(dst_node_id);
    }
    dependency_matrixes_[dependency_type].set(
      get_dependency_matrix_index(src_node_id, dst_node_id));
  }

  auto PassDependencyGraph::get_pass_node_count() const -> usize
  {
    return pass_node_count_;
  }

  auto PassDependencyGraph::get_dependency_matrix_index(
    const PassNodeID src_node_id, const PassNodeID dst_node_id) const -> usize
  {
    return src_node_id.id * get_pass_node_count() + dst_node_id.id;
  }

  auto PassDependencyGraph::calculate_dependency_level(PassNodeID pass_node_id) -> usize
  {
    if (dependency_levels_[pass_node_id.id] == UNINITIALIZED_DEPENDENCY_LEVEL)
    {
      usize dependency_level = 0u;
      for (const auto dependency_node_id : dependencies_[pass_node_id.id])
      {
        dependency_level =
          std::max(dependency_level, 1 + calculate_dependency_level(dependency_node_id));
      }
      dependency_levels_[pass_node_id.id] = dependency_level;
    }
    return dependency_levels_[pass_node_id.id];
  }

  void RenderGraphExecution::init()
  {
    SOUL_ASSERT_MAIN_THREAD();
    SOUL_PROFILE_ZONE_WITH_NAME("Render Graph Execution Init");

    SOUL_LOG_RG_EXEC("Resource Node Info :");
    SOUL_LOG_RG_EXEC("=========================================");
    for (u32 resource_i = 0; resource_i < render_graph_->get_resource_nodes().size(); resource_i++)
    {
      const auto& resource_node = render_graph_->get_resource_nodes()[resource_i];
      SOUL_LOG_RG_EXEC("- {}", resource_i);
      if (resource_node.writer.is_valid())
      {
        SOUL_LOG_RG_EXEC(
          ">> >> Writer : {}",
          render_graph_->get_pass_nodes()[resource_node.writer.id]->get_name());
      } else
      {
        SOUL_LOG_RG_EXEC(">> >> Writer : None");
      }
      SOUL_LOG_RG_EXEC(">> >> Reader :");
      for (const auto reader_pass_node_id : resource_node.readers)
      {
        SOUL_ASSERT(0, reader_pass_node_id.is_valid(), "Must be valid pass node");
        SOUL_LOG_RG_EXEC(
          "---- {}", render_graph_->get_pass_nodes()[reader_pass_node_id.id]->get_name());
      }
    }

    SOUL_LOG_RG_EXEC(">> Dependency Level");
    SOUL_LOG_RG_EXEC("=========================================");
    for (u32 pass_i = 0; pass_i < render_graph_->get_pass_nodes().size(); pass_i++)
    {
      const auto pass_node_id = PassNodeID(pass_i);
      SOUL_LOG_RG_EXEC(
        "- {} : {}",
        render_graph_->get_pass_nodes()[pass_i]->get_name(),
        pass_dependency_graph_.get_dependency_level(pass_node_id));
      SOUL_LOG_RG_EXEC(">> >> Dependencies :");
      for (const auto dependency_node_id : pass_dependency_graph_.get_dependencies(pass_node_id))
      {
        SOUL_LOG_RG_EXEC(
          "---- {}", render_graph_->get_pass_nodes()[dependency_node_id.id]->get_name());
      }
    }

    compute_active_passes();
    compute_pass_order();

    pass_infos_.resize(render_graph_->get_pass_nodes().size());

    const auto internal_buffer_count = render_graph_->get_internal_buffers().size();
    const auto external_buffer_count = render_graph_->get_external_buffers().size();
    const auto buffer_count          = internal_buffer_count + external_buffer_count;
    buffer_infos_.resize(buffer_count);
    internal_buffer_infos_ = u64span(buffer_infos_.begin(), internal_buffer_count);
    external_buffer_infos_ =
      u64span(buffer_infos_.begin() + internal_buffer_count, external_buffer_count);

    const auto& internal_textures = render_graph_->get_internal_textures();
    const auto& external_textures = render_graph_->get_external_textures();
    texture_infos_.resize(internal_textures.size() + external_textures.size());
    internal_texture_infos_ = u64span(texture_infos_.begin(), internal_textures.size());
    external_texture_infos_ =
      u64span(texture_infos_.begin() + internal_textures.size(), external_textures.size());

    const auto external_tlas_count       = render_graph_->get_external_tlas_list().size();
    const auto external_blas_group_count = render_graph_->get_external_blas_group_list().size();
    resource_infos_.resize(external_tlas_count + external_blas_group_count);
    external_tlas_resource_infos_ = u64span(resource_infos_.begin(), external_tlas_count);
    external_blas_group_resource_infos_ =
      u64span(resource_infos_.begin() + external_tlas_count, external_blas_group_count);

    constexpr auto fold_internal_texture_view_count =
      [](usize count, const RGInternalTexture& internal_texture)
    {
      return count + internal_texture.get_view_count();
    };
    auto fold_external_texture_view_count =
      [gpu_system = this->gpu_system_](const usize count, const RGExternalTexture& external_texture)
    {
      const TextureDesc& desc = gpu_system->texture_ref(external_texture.texture_id).desc;
      return count + desc.get_view_count();
    };

    const usize internal_view_count = std::accumulate(
      internal_textures.begin(),
      internal_textures.end(),
      usize(0),
      fold_internal_texture_view_count);
    const usize external_view_count = std::accumulate(
      external_textures.begin(),
      external_textures.end(),
      usize(0),
      fold_external_texture_view_count);
    texture_view_infos_.resize(internal_view_count + external_view_count);

    for (usize texture_info_idx = 0, view_offset = 0;
         texture_info_idx < internal_texture_infos_.size();
         texture_info_idx++)
    {
      TextureExecInfo& texture_info = internal_texture_infos_[texture_info_idx];
      texture_info.view             = texture_view_infos_.data() + view_offset;
      texture_info.mip_levels       = internal_textures[texture_info_idx].mip_levels;
      texture_info.layers           = internal_textures[texture_info_idx].layer_count;
      texture_info.name             = internal_textures[texture_info_idx].name.cview();
      view_offset += texture_info.get_view_count();
    }

    for (usize texture_info_idx = 0, view_offset = internal_view_count;
         texture_info_idx < external_texture_infos_.size();
         texture_info_idx++)
    {
      TextureExecInfo& texture_info = external_texture_infos_[texture_info_idx];
      const auto texture_id         = external_textures[texture_info_idx].texture_id;
      const TextureDesc& desc       = gpu_system_->texture_ref(texture_id).desc;
      texture_info.view             = texture_view_infos_.data() + view_offset;
      texture_info.mip_levels       = desc.mip_levels;
      texture_info.layers           = desc.layer_count;
      texture_info.name             = gpu_system_->texture_name_view(texture_id);
      view_offset += texture_info.get_view_count();
    }

    for (const auto pass_node_id : pass_order_)
    {
      const auto pass_index      = pass_node_id.id;
      PassBaseNode& pass_node    = *render_graph_->get_pass_nodes()[pass_index];
      const auto pass_queue_type = pass_node.get_queue_type();
      PassExecInfo& pass_info    = pass_infos_[pass_index];
      pass_info.pass_node        = &pass_node;
      pass_info.name             = pass_node.name_view();

      init_shader_buffers(pass_node.get_buffer_read_accesses(), pass_node_id, pass_queue_type);
      init_shader_buffers(pass_node.get_buffer_write_accesses(), pass_node_id, pass_queue_type);
      init_shader_textures(pass_node.get_texture_read_accesses(), pass_node_id, pass_queue_type);
      init_shader_textures(pass_node.get_texture_write_accesses(), pass_node_id, pass_queue_type);
      init_shader_tlas_accesses(
        pass_node.get_shader_tlas_read_accesses(), pass_node_id, pass_queue_type);
      init_shader_blas_group_accesses(
        pass_node.get_shader_blas_group_read_accesses(), pass_node_id, pass_queue_type);

      for (const BufferNodeID node_id : pass_node.get_vertex_buffers())
      {
        SOUL_ASSERT(0, node_id.is_valid());

        auto buffer_info_id = get_buffer_info_index(node_id);

        pass_info.buffer_accesses.push_back(BufferAccess{
          .stage_flags     = {PipelineStage::VERTEX_INPUT},
          .access_flags    = {AccessType::VERTEX_ATTRIBUTE_READ},
          .buffer_info_idx = buffer_info_id,
        });

        update_buffer_info(
          pass_queue_type, {BufferUsage::VERTEX}, pass_node_id, &buffer_infos_[buffer_info_id]);
      }

      for (const BufferNodeID node_id : pass_node.get_index_buffers())
      {
        SOUL_ASSERT(0, node_id.is_valid());

        const auto buffer_info_id = get_buffer_info_index(node_id);

        pass_info.buffer_accesses.push_back(BufferAccess{
          .stage_flags     = {PipelineStage::VERTEX_INPUT},
          .access_flags    = {AccessType::INDEX_READ},
          .buffer_info_idx = buffer_info_id,
        });

        update_buffer_info(
          pass_queue_type, {BufferUsage::INDEX}, pass_node_id, &buffer_infos_[buffer_info_id]);
      }

      for (const BufferNodeID node_id : pass_node.get_indirect_command_buffers())
      {
        SOUL_ASSERT(0, node_id.is_valid());

        const auto buffer_info_id = get_buffer_info_index(node_id);

        pass_info.buffer_accesses.push_back(BufferAccess{
          .stage_flags     = {PipelineStage::DRAW_INDIRECT},
          .access_flags    = {AccessType::INDIRECT_COMMAND_READ},
          .buffer_info_idx = buffer_info_id,
        });

        update_buffer_info(
          pass_queue_type, {BufferUsage::INDIRECT}, pass_node_id, &buffer_infos_[buffer_info_id]);
      }

      const auto& [dimension, sampleCount, colorAttachments, resolveAttachments, depthStencilAttachment] =
        pass_node.get_render_target();

      for (const ColorAttachment& color_attachment : colorAttachments)
      {
        SOUL_ASSERT(0, color_attachment.out_node_id.id.is_valid());

        const auto texture_info_id = get_texture_info_index(color_attachment.out_node_id);
        update_texture_info(
          pass_queue_type,
          {TextureUsage::COLOR_ATTACHMENT},
          pass_node_id,
          {color_attachment.desc.view, 1, 1},
          &texture_infos_[texture_info_id]);

        pass_info.texture_accesses.push_back(TextureAccess{
          .stage_flags  = {PipelineStage::COLOR_ATTACHMENT_OUTPUT},
          .access_flags = {AccessType::COLOR_ATTACHMENT_READ, AccessType::COLOR_ATTACHMENT_WRITE},
          .layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .texture_info_idx = texture_info_id,
          .view             = color_attachment.desc.view,
        });
      }

      for (const ResolveAttachment& resolve_attachment : resolveAttachments)
      {
        const auto texture_info_id = get_texture_info_index(resolve_attachment.out_node_id);
        update_texture_info(
          pass_queue_type,
          {TextureUsage::COLOR_ATTACHMENT},
          pass_node_id,
          {resolve_attachment.desc.view, 1, 1},
          &texture_infos_[texture_info_id]);

        pass_info.texture_accesses.push_back(TextureAccess{
          .stage_flags  = {PipelineStage::COLOR_ATTACHMENT_OUTPUT},
          .access_flags = {AccessType::COLOR_ATTACHMENT_READ, AccessType::COLOR_ATTACHMENT_WRITE},
          .layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .texture_info_idx = texture_info_id,
          .view             = resolve_attachment.desc.view,
        });
      }

      if (depthStencilAttachment.out_node_id.id.is_valid())
      {
        const auto resource_info_index = get_texture_info_index(depthStencilAttachment.out_node_id);

        const auto texture_info_id = get_texture_info_index(depthStencilAttachment.out_node_id);
        update_texture_info(
          pass_queue_type,
          {TextureUsage::DEPTH_STENCIL_ATTACHMENT},
          pass_node_id,
          {depthStencilAttachment.desc.view, 1, 1},
          &texture_infos_[texture_info_id]);

        pass_info.texture_accesses.push_back(TextureAccess{
          .stage_flags = {PipelineStage::EARLY_FRAGMENT_TESTS, PipelineStage::LATE_FRAGMENT_TESTS},
          .access_flags =
            {AccessType::DEPTH_STENCIL_ATTACHMENT_READ, AccessType::DEPTH_STENCIL_ATTACHMENT_WRITE},
          .layout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
          .texture_info_idx = resource_info_index,
          .view             = depthStencilAttachment.desc.view,
        });
      }

      for (const auto& source_buffer : pass_node.get_source_buffers())
      {
        const auto resource_info_index = get_buffer_info_index(source_buffer.node_id);
        update_buffer_info(
          pass_queue_type,
          {BufferUsage::TRANSFER_SRC},
          pass_node_id,
          &buffer_infos_[resource_info_index]);

        pass_info.buffer_accesses.push_back(BufferAccess{
          .stage_flags     = {PipelineStage::TRANSFER},
          .access_flags    = {AccessType::TRANSFER_READ},
          .buffer_info_idx = resource_info_index,
        });
      }

      for (const auto& dst_buffer : pass_node.get_destination_buffers())
      {
        const auto resource_info_index = get_buffer_info_index(dst_buffer.output_node_id);
        update_buffer_info(
          pass_queue_type,
          {BufferUsage::TRANSFER_DST},
          pass_node_id,
          &buffer_infos_[resource_info_index]);

        pass_info.buffer_accesses.push_back(BufferAccess{
          .stage_flags     = {PipelineStage::TRANSFER},
          .access_flags    = {AccessType::TRANSFER_WRITE},
          .buffer_info_idx = resource_info_index,
        });
      }

      for (const auto& src_texture : pass_node.get_source_textures())
      {
        const auto resource_info_index = get_texture_info_index(src_texture.node_id);
        update_texture_info(
          pass_queue_type,
          {TextureUsage::TRANSFER_SRC},
          pass_node_id,
          src_texture.view_range,
          &texture_infos_[resource_info_index]);

        auto generate_invalidate_barrier =
          [resource_info_index](SubresourceIndex view_index) -> TextureAccess
        {
          return {
            .stage_flags      = {PipelineStage::TRANSFER},
            .access_flags     = {AccessType::TRANSFER_READ},
            .layout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .texture_info_idx = resource_info_index,
            .view             = view_index,
          };
        };
        std::ranges::transform(
          src_texture.view_range,
          std::back_inserter(pass_info.texture_accesses),
          generate_invalidate_barrier);
      }

      for (const auto& dst_texture : pass_node.get_destination_textures())
      {
        const auto resource_info_index = get_texture_info_index(dst_texture.output_node_id);
        update_texture_info(
          pass_queue_type,
          {TextureUsage::TRANSFER_DST},
          pass_node_id,
          dst_texture.view_range,
          &texture_infos_[resource_info_index]);

        auto generate_invalidate_barrier =
          [resource_info_index](SubresourceIndex /* view_index */) -> TextureAccess
        {
          return {
            .stage_flags      = {PipelineStage::TRANSFER},
            .access_flags     = {AccessType::TRANSFER_WRITE},
            .layout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .texture_info_idx = resource_info_index,
          };
        };
        std::ranges::transform(
          dst_texture.view_range,
          std::back_inserter(pass_info.texture_accesses),
          generate_invalidate_barrier);
      }

      for (const auto& buffer : pass_node.get_as_build_input_buffers())
      {
        const auto resource_info_index = get_buffer_info_index(buffer);
        update_buffer_info(
          pass_node.get_queue_type(),
          {BufferUsage::AS_BUILD_INPUT},
          pass_node_id,
          &buffer_infos_[resource_info_index]);

        pass_info.buffer_accesses.push_back(BufferAccess{
          .stage_flags     = {PipelineStage::AS_BUILD},
          .access_flags    = {AccessType::SHADER_READ},
          .buffer_info_idx = resource_info_index,
        });
      }

      for (const auto& blas_group : pass_node.get_as_build_input_blas_groups())
      {
        const auto resource_info_index = get_blas_group_resource_info_index(blas_group);
        update_resource_info(
          pass_node.get_queue_type(), pass_node_id, &resource_infos_[resource_info_index]);

        pass_info.resource_accesses.push_back(ResourceAccess{
          .stage_flags       = {PipelineStage::AS_BUILD},
          .access_flags      = {AccessType::AS_READ},
          .resource_info_idx = resource_info_index,
        });
      }

      for (const auto& dst_tlas : pass_node.get_as_build_destination_tlas_list())
      {
        const auto resource_info_index = get_tlas_resource_info_index(dst_tlas.output_node_id);
        update_resource_info(
          pass_node.get_queue_type(), pass_node_id, &resource_infos_[resource_info_index]);

        pass_info.resource_accesses.push_back(ResourceAccess{
          .stage_flags       = {PipelineStage::AS_BUILD},
          .access_flags      = {AccessType::AS_READ, AccessType::AS_WRITE},
          .resource_info_idx = resource_info_index,
        });
      }

      for (const auto& dst_blas_group : pass_node.get_as_build_destination_blas_group_list())
      {
        const auto resource_info_index =
          get_blas_group_resource_info_index(dst_blas_group.output_node_id);
        update_resource_info(
          pass_node.get_queue_type(), pass_node_id, &resource_infos_[resource_info_index]);

        pass_info.resource_accesses.push_back(ResourceAccess{
          .stage_flags       = {PipelineStage::AS_BUILD},
          .access_flags      = {AccessType::AS_READ, AccessType::AS_WRITE},
          .resource_info_idx = resource_info_index,
        });
      }
    }

    for (usize i = 0; i < render_graph_->get_internal_buffers().size(); i++)
    {
      const RGInternalBuffer& rg_buffer = render_graph_->get_internal_buffers()[i];
      BufferExecInfo& buffer_info       = buffer_infos_[i];

      if (buffer_info.usage_flags.none())
      {
        SOUL_ASSERT(
          0,
          buffer_info.first_pass.is_null() && buffer_info.last_pass.is_null() &&
            buffer_info.queue_flags.none() && buffer_info.passes.empty(),
          "");
        continue;
      }

      buffer_info.buffer_id = gpu_system_->create_transient_buffer(
        rg_buffer.name.clone(),
        {
          .size        = rg_buffer.size,
          .usage_flags = buffer_info.usage_flags,
          .queue_flags = buffer_info.queue_flags,
        });
    }

    for (usize i = 0; i < external_buffer_infos_.size(); i++)
    {
      BufferExecInfo& buffer_info = external_buffer_infos_[i];
      if (buffer_info.passes.empty())
      {
        continue;
      }
      buffer_info.buffer_id = render_graph_->get_external_buffers()[i].buffer_id;
    }

    for (usize i = 0; i < render_graph_->get_internal_textures().size(); i++)
    {
      const RGInternalTexture& rg_texture = render_graph_->get_internal_textures()[i];
      TextureExecInfo& texture_info       = texture_infos_[i];

      if (texture_info.usage_flags.none())
      {
        SOUL_ASSERT(
          0,
          texture_info.first_pass.is_null() && texture_info.last_pass.is_null() &&
            texture_info.queue_flags.none(),
          "");
        continue;
      }

      TextureDesc desc = {
        .type         = rg_texture.type,
        .format       = rg_texture.format,
        .extent       = rg_texture.extent,
        .mip_levels   = rg_texture.mip_levels,
        .sample_count = rg_texture.sample_count,
        .usage_flags  = texture_info.usage_flags,
        .queue_flags  = texture_info.queue_flags,
      };
      if (!rg_texture.clear)
      {
        texture_info.texture_id = gpu_system_->create_texture(rg_texture.name.clone(), desc);
      } else
      {
        desc.usage_flags |= {TextureUsage::SAMPLED};
        texture_info.texture_id =
          gpu_system_->create_texture(rg_texture.name.clone(), desc, rg_texture.clear_value);
      }
    }

    for (usize i = 0; i < external_texture_infos_.size(); i++)
    {
      TextureExecInfo& texture_info = external_texture_infos_[i];
      texture_info.texture_id       = render_graph_->get_external_textures()[i].texture_id;
    }
  }

  void traverse_recursive(
    BitVector<>& pass_node_bits, const PassNodeID pass_node_id, const PassDependencyGraph& adj_list)
  {
    if (pass_node_bits[pass_node_id.id])
    {
      return;
    }
    pass_node_bits.set(pass_node_id.id);
    for (const auto dependency_node_id : adj_list.get_dependencies(pass_node_id))
    {
      const auto dependency_flags = adj_list.get_dependency_flags(dependency_node_id, pass_node_id);
      if ((dependency_flags & PassDependencyGraph::OP_AFTER_WRITE_DEPENDENCY).any())
      {
        traverse_recursive(pass_node_bits, dependency_node_id, adj_list);
      }
    }
  }

  void RenderGraphExecution::compute_active_passes()
  {
    const auto& pass_nodes = render_graph_->get_pass_nodes();

    active_passes_.resize(pass_nodes.size());
    for (const auto& resource_node : render_graph_->get_resource_nodes())
    {
      if (resource_node.creator.is_valid() && resource_node.resource_id.is_external())
      {
        traverse_recursive(active_passes_, resource_node.creator, pass_dependency_graph_);
      }
    }
  }

  void RenderGraphExecution::compute_pass_order()
  {
    const auto& pass_nodes = render_graph_->get_pass_nodes();

    pass_order_.reserve(pass_nodes.size());
    for (auto pass_index = 0u; pass_index < pass_nodes.size(); pass_index++)
    {
      if (active_passes_[pass_index])
      {
        pass_order_.push_back(PassNodeID(pass_index));
      }
    }

    const auto pass_compare_func = [this](const PassNodeID node1, const PassNodeID node2) -> b8
    {
      const auto node1_dependency_level = pass_dependency_graph_.get_dependency_level(node1);
      const auto node2_dependency_level = pass_dependency_graph_.get_dependency_level(node2);
      if (node1_dependency_level == node2_dependency_level)
      {
        const auto node1_dependent_count = pass_dependency_graph_.get_dependants(node1).size();
        const auto node2_dependent_count = pass_dependency_graph_.get_dependants(node2).size();
        return node1_dependent_count < node2_dependent_count;
      }
      return node1_dependency_level < node2_dependency_level;
    };
    std::ranges::sort(pass_order_, pass_compare_func);

    SOUL_LOG_RG_EXEC(">> Pass Order: ");
    SOUL_LOG_RG_EXEC("=========================================");

    for (const auto pass_node_id : pass_order_)
    {
      SOUL_LOG_RG_EXEC("- {}", render_graph_->get_pass_nodes()[pass_node_id.id]->get_name());
    }
  }

  auto RenderGraphExecution::create_render_pass(const u32 pass_index) -> VkRenderPass
  {
    SOUL_PROFILE_ZONE();
    SOUL_ASSERT_MAIN_THREAD();

    const auto pass_node                = render_graph_->get_pass_nodes()[pass_index];
    RenderPassKey render_pass_key       = {};
    const RGRenderTarget& render_target = pass_node->get_render_target();

    auto get_render_pass_attachment = [this, pass_index](const auto& attachment) -> Attachment
    {
      const u32 texture_info_idx          = get_texture_info_index(attachment.out_node_id);
      const TextureExecInfo& texture_info = texture_infos_[texture_info_idx];
      const auto& texture                 = gpu_system_->texture_ref(texture_info.texture_id);

      Attachment render_pass_attachment  = {};
      render_pass_attachment.format      = texture.desc.format;
      render_pass_attachment.sampleCount = texture.desc.sample_count;
      if (texture_info.first_pass.id == pass_index)
      {
        render_pass_attachment.flags |= ATTACHMENT_FIRST_PASS_BIT;
      }
      if (texture_info.last_pass.id == pass_index)
      {
        render_pass_attachment.flags |= ATTACHMENT_LAST_PASS_BIT;
      }
      if (attachment.desc.clear)
      {
        render_pass_attachment.flags |= ATTACHMENT_CLEAR_BIT;
      }
      if (is_external(texture_info))
      {
        render_pass_attachment.flags |= ATTACHMENT_EXTERNAL_BIT;
      }
      render_pass_attachment.flags |= ATTACHMENT_ACTIVE_BIT;
      return render_pass_attachment;
    };

    std::ranges::transform(
      render_target.color_attachments,
      render_pass_key.color_attachments.begin(),
      get_render_pass_attachment);
    std::ranges::transform(
      render_target.resolve_attachments,
      render_pass_key.resolve_attachments.begin(),
      get_render_pass_attachment);

    if (render_target.depth_stencil_attachment.out_node_id.id.is_valid())
    {
      const DepthStencilAttachment& attachment = render_target.depth_stencil_attachment;
      render_pass_key.depth_attachment         = get_render_pass_attachment(attachment);
    }

    return gpu_system_->request_render_pass(render_pass_key);
  }

  auto RenderGraphExecution::create_framebuffer(u32 pass_index, VkRenderPass render_pass)
    -> VkFramebuffer
  {
    SOUL_ASSERT_MAIN_THREAD();

    const auto pass_node = render_graph_->get_pass_nodes()[pass_index];

    VkImageView image_views[2 * MAX_COLOR_ATTACHMENT_PER_SHADER + 1];
    u32 image_view_count                = 0;
    const RGRenderTarget& render_target = pass_node->get_render_target();

    for (const ColorAttachment& attachment : render_target.color_attachments)
    {
      const auto texture_id = get_texture_id(attachment.out_node_id);
      image_views[image_view_count++] =
        gpu_system_->get_texture_view(texture_id, attachment.desc.view).vk_handle;
    }

    for (const ResolveAttachment& attachment : render_target.resolve_attachments)
    {
      const auto texture_id = get_texture_id(attachment.out_node_id);
      image_views[image_view_count++] =
        gpu_system_->get_texture_view(texture_id, attachment.desc.view).vk_handle;
    }

    if (render_target.depth_stencil_attachment.out_node_id.id.is_valid())
    {
      const auto info_idx =
        get_texture_info_index(render_target.depth_stencil_attachment.out_node_id);
      const TextureExecInfo& texture_info = texture_infos_[info_idx];
      const auto depth_attachment_desc    = render_target.depth_stencil_attachment.desc;

      image_views[image_view_count++] =
        gpu_system_->get_texture_view(texture_info.texture_id, depth_attachment_desc.view)
          .vk_handle;
    }

    const VkFramebufferCreateInfo framebuffer_info = {
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass      = render_pass,
      .attachmentCount = image_view_count,
      .pAttachments    = image_views,
      .width           = render_target.dimension.x,
      .height          = render_target.dimension.y,
      .layers          = 1};

    return gpu_system_->create_framebuffer(framebuffer_info);
  }

  void RenderGraphExecution::sync_external()
  {
    for (auto& event : external_event_idxs_)
    {
      event = nilopt;
    }

    for (auto& texture_info : texture_infos_)
    {
      if (texture_info.usage_flags.none())
      {
        continue;
      }

      const auto& texture = gpu_system_->texture_ref(texture_info.texture_id);

      std::for_each(
        texture_info.view,
        texture_info.view + texture_info.get_view_count(),
        [this,
         &texture,
         texture_id          = texture_info.texture_id,
         external_queue_type = texture.cache_state.queue_owner](TextureViewExecInfo& view_info)
        {
          view_info.layout = texture.layout;
          if (!view_info.passes.empty())
          {
            const auto first_queue_type =
              render_graph_->get_pass_nodes()[view_info.passes[0].id]->get_queue_type();
            view_info.cache_state = texture.cache_state;
            if (gpu_system_->is_owned_by_presentation_engine(texture_id))
            {
              view_info.pending_event_idx = nilopt;
              view_info.pending_semaphore.assign(
                &gpu_system_->get_frame_context().image_available_semaphore);
            } else if (external_queue_type == first_queue_type)
            {
              if (
                texture.cache_state.unavailable_pipeline_stages.none() &&
                texture.cache_state.unavailable_accesses.none())
              {
                return;
              }
              Option<u32>& external_event = external_event_idxs_[first_queue_type];
              if (!external_event.is_some() && external_queue_type != QueueType::TRANSFER)
              {
                external_event = event_infos_.size();
                event_infos_.push_back(EventInfo{gpu_system_->create_event(), {}});
              }
              view_info.pending_event_idx = external_event;
              view_info.pending_semaphore.assign(TimelineSemaphore::null());
              external_events_stage_flags_[first_queue_type] |=
                texture.cache_state.unavailable_pipeline_stages;
            } else if (external_queue_type != QueueType::NONE)
            {
              view_info.pending_event_idx = nilopt;
              view_info.pending_semaphore.assign(
                command_queues_->ref(external_queue_type).get_timeline_semaphore());
            }
          }
        });
    }

    std::ranges::for_each(
      texture_infos_,
      [this](const TextureExecInfo& texture_info)
      {
        if (texture_info.usage_flags.none())
        {
          return;
        }
        const Texture& texture = gpu_system_->texture_ref(texture_info.texture_id);
        std::for_each(
          texture_info.view,
          texture_info.view + texture_info.get_view_count(),
          [layout = texture.layout](TextureViewExecInfo& view_info)
          {
            view_info.layout = layout;
          });
      });

    auto compute_non_texture_sync_info =
      [this](const ResourceCacheState& external_cache_state, auto& resource_exec_info)
    {
      const auto external_queue_type = external_cache_state.queue_owner;
      const auto first_queue_type =
        render_graph_->get_pass_nodes()[resource_exec_info.first_pass.id]->get_queue_type();

      resource_exec_info.cache_state = external_cache_state;
      if (external_queue_type == first_queue_type)
      {
        if (
          external_cache_state.unavailable_pipeline_stages.none() &&
          external_cache_state.unavailable_accesses.none())
        {
          return;
        }
        auto& external_event_idx = external_event_idxs_[first_queue_type];
        if (external_event_idx == nilopt && external_queue_type != QueueType::TRANSFER)
        {
          external_event_idx = event_infos_.size();
          event_infos_.push_back(EventInfo{gpu_system_->create_event(), {}});
        }
        resource_exec_info.pending_event_idx = external_event_idx;
        external_events_stage_flags_[first_queue_type] |=
          external_cache_state.unavailable_pipeline_stages;
        resource_exec_info.pending_semaphore.assign(TimelineSemaphore::null());
      } else if (external_queue_type != QueueType::COUNT)
      {
        resource_exec_info.pending_event_idx = nilopt;
        resource_exec_info.pending_semaphore.assign(
          command_queues_->ref(external_queue_type).get_timeline_semaphore());
      }
    };

    for (auto& buffer_info : external_buffer_infos_)
    {
      if (buffer_info.passes.empty())
      {
        continue;
      }
      const auto& buffer = gpu_system_->buffer_ref(buffer_info.buffer_id);
      compute_non_texture_sync_info(buffer.cache_state, buffer_info);
    }

    for (usize tlas_idx = 0; tlas_idx < external_tlas_resource_infos_.size(); tlas_idx++)
    {
      auto& resource_exec_info     = external_tlas_resource_infos_[tlas_idx];
      const auto& rg_external_tlas = render_graph_->get_external_tlas_list()[tlas_idx];
      const auto& tlas             = gpu_system_->tlas_cref(rg_external_tlas.tlas_id);
      compute_non_texture_sync_info(tlas.cache_state, resource_exec_info);
    }

    for (usize blas_group_idx = 0; blas_group_idx < external_blas_group_resource_infos_.size();
         blas_group_idx++)
    {
      auto& resource_exec_info = external_blas_group_resource_infos_[blas_group_idx];
      const auto& rg_external_blas_group =
        render_graph_->get_external_blas_group_list()[blas_group_idx];
      const auto& blas_group = gpu_system_->blas_group_ref(rg_external_blas_group.blas_group_id);
      compute_non_texture_sync_info(blas_group.cache_state, resource_exec_info);
    }

    // Sync events
    for (const auto queue_type : FlagIter<QueueType>())
    {
      if (external_event_idxs_[queue_type] != nilopt)
      {
        const auto sync_event_command_buffer = command_pools_->request_command_buffer(queue_type);
        set_event(
          sync_event_command_buffer,
          external_event_idxs_[queue_type].some_ref(),
          external_events_stage_flags_[queue_type]);
        command_queues_->ref(queue_type).submit(sync_event_command_buffer);
      }
    }
  }

  void RenderGraphExecution::execute_pass(const u32 pass_index, PrimaryCommandBuffer command_buffer)
  {
    SOUL_PROFILE_ZONE();
    const auto& pass_node               = *render_graph_->get_pass_nodes()[pass_index];
    const auto pipeline_flags           = pass_node.get_pipeline_flags();
    VkRenderPass render_pass            = VK_NULL_HANDLE;
    const RGRenderTarget& render_target = pass_node.get_render_target();

    VkClearValue clear_values[2 * MAX_COLOR_ATTACHMENT_PER_SHADER + 1];
    VkRenderPassBeginInfo render_pass_begin_info = {};

    if (pipeline_flags.test(PipelineType::RASTER))
    {
      u32 clear_count           = 0;
      render_pass               = create_render_pass(pass_index);
      VkFramebuffer framebuffer = create_framebuffer(pass_index, render_pass);

      for (const ColorAttachment& attachment : render_target.color_attachments)
      {
        ClearValue clear_value = attachment.desc.clear_value;
        memcpy(&clear_values[clear_count++], &clear_value, sizeof(VkClearValue));
      }

      for (const ResolveAttachment& attachment : render_target.resolve_attachments)
      {
        ClearValue clear_value = attachment.desc.clear_value;
        memcpy(&clear_values[clear_count++], &clear_value, sizeof(VkClearValue));
      }

      if (render_target.depth_stencil_attachment.out_node_id.id.is_valid())
      {
        const RGDepthStencilAttachmentDesc& desc = render_target.depth_stencil_attachment.desc;
        ClearValue clear_value                   = desc.clear_value;
        clear_values[clear_count++].depthStencil = {
          clear_value.depth_stencil.depth, clear_value.depth_stencil.stencil};
      }

      render_pass_begin_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass      = render_pass,
        .framebuffer     = framebuffer,
        .renderArea      = {.extent = {render_target.dimension.x, render_target.dimension.y}},
        .clearValueCount = clear_count,
        .pClearValues    = clear_values,
      };
    }
    auto registry =
      RenderGraphRegistry::New(gpu_system_, this, render_pass, render_target.sample_count);

    auto render_compiler = RenderCompiler::New(gpu_system_, command_buffer.get_vk_handle());

    pass_node.get_pipeline_flags().for_each(
      [&render_compiler](PipelineType pipeline_type)
      {
        constexpr auto MAPPING = FlagMap<PipelineType, VkPipelineBindPoint>::FromValues({
          VK_PIPELINE_BIND_POINT_MAX_ENUM,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          VK_PIPELINE_BIND_POINT_COMPUTE,
          VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        });
        const auto bind_point  = MAPPING[pipeline_type];
        if (bind_point != VK_PIPELINE_BIND_POINT_MAX_ENUM)
        {
          render_compiler.bind_descriptor_sets(bind_point);
        }
      });
    pass_node.execute(
      &registry, &render_compiler, &render_pass_begin_info, command_pools_, gpu_system_);
  }

  void RenderGraphExecution::run()
  {
    SOUL_ASSERT_MAIN_THREAD();
    SOUL_PROFILE_ZONE();
    SOUL_LOG_RG_EXEC("Run Render Graph\n\n\n");

    sync_external();

    Vector<VkEvent> garbage_events;

    Vector<VkMemoryBarrier> pipeline_barriers;
    Vector<VkBufferMemoryBarrier> pipeline_buffer_barriers;
    Vector<VkImageMemoryBarrier> pipeline_image_barriers;
    Vector<VkMemoryBarrier> event_barriers;
    Vector<VkBufferMemoryBarrier> event_buffer_barriers;
    Vector<VkImageMemoryBarrier> event_image_barriers;

    Vector<VkImageMemoryBarrier> semaphore_layout_barriers;
    Vector<VkEvent> events;

    for (const auto pass_node_id : pass_order_)
    {
      SOUL_PROFILE_ZONE_WITH_NAME("Pass command buffer submission");
      SOUL_PROFILE_ZONE_TEXT(pass_infos_[pass_node_id.id].name);
      SOUL_LOG_RG_EXEC(">> Evaluate pass : {}", pass_infos_[pass_node_id.id].name);
      SOUL_LOG_RG_EXEC("=========================================");
      SCOPE_EXIT(SOUL_LOG_RG_EXEC("=========================================\n"));

      const auto pass_index = pass_node_id.id;
      runtime::ScopeAllocator passNodeScopeAllocator(
        "Pass Node Scope Allocator"_str, runtime::get_temp_allocator());
      PassBaseNode* pass_node = render_graph_->get_pass_nodes()[pass_index];
      auto current_queue_type = pass_node->get_queue_type();
      auto& command_queue     = command_queues_->ref(current_queue_type);
      auto& pass_info         = pass_infos_[pass_index];

      const auto cmd_buffer = command_pools_->request_command_buffer(current_queue_type);

      vec3f32 color = util::get_random_color();
      SOUL_ASSERT(0, pass_node->name_view().is_null_terminated(), "");
      const VkDebugUtilsLabelEXT passLabel = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, // sType
        nullptr,                                 // pNext
        pass_node->name_view().data(),           // pLabelName
        {color.x, color.y, color.z, 1.0f},       // color
      };
      vkCmdBeginDebugUtilsLabelEXT(cmd_buffer.get_vk_handle(), &passLabel);

      pipeline_barriers.clear();
      pipeline_buffer_barriers.clear();
      pipeline_image_barriers.clear();
      event_barriers.clear();
      event_buffer_barriers.clear();
      event_image_barriers.clear();
      semaphore_layout_barriers.clear();
      events.clear();

      PipelineStageFlags pipeline_src_stage_flags;
      PipelineStageFlags pipeline_dst_stage_flags;
      PipelineStageFlags event_src_stage_flags;
      PipelineStageFlags event_dst_stage_flags;
      PipelineStageFlags semaphore_dst_stage_flags;

      for (const auto& barrier : pass_info.resource_accesses)
      {
        ResourceExecInfo& resource_info = resource_infos_[barrier.resource_info_idx];

        if (resource_info.cache_state.unavailable_accesses.any())
        {
          for (auto& access_flags : resource_info.cache_state.visible_access_matrix)
          {
            access_flags = AccessFlags{};
          }
        }

        const auto queue_owner = resource_info.cache_state.queue_owner;
        const auto unavailable_pipeline_stages =
          resource_info.cache_state.unavailable_pipeline_stages;
        const auto unavailable_accesses = resource_info.cache_state.unavailable_accesses;

        if (
          is_semaphore_null(resource_info.pending_semaphore) && unavailable_accesses.none() &&
          !resource_info.cache_state.need_invalidate(barrier.stage_flags, barrier.access_flags))
        {
          resource_info.cache_state.commit_access(
            current_queue_type, barrier.stage_flags, barrier.access_flags);
          continue;
        }

        if (is_semaphore_valid(resource_info.pending_semaphore))
        {
          command_queues_->ref(current_queue_type)
            .wait(resource_info.pending_semaphore, vk_cast(barrier.stage_flags));
          resource_info.pending_semaphore.assign(TimelineSemaphore::null());
          resource_info.cache_state.commit_wait_semaphore(
            queue_owner, current_queue_type, barrier.stage_flags);
        } else
        {
          if (
            unavailable_pipeline_stages.none() ||
            unavailable_pipeline_stages == PipelineStageFlags{PipelineStage::TOP_OF_PIPE})
          {
            SOUL_ASSERT(0, resource_info.cache_state.unavailable_accesses.none());
          }
          VkMemoryBarrier mem_barrier = {
            .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .srcAccessMask = vk_cast(unavailable_accesses),
            .dstAccessMask = vk_cast(barrier.access_flags),
          };

          if (resource_info.pending_event_idx != nilopt)
          {
            event_barriers.push_back(mem_barrier);
            wait_event(
              events,
              event_src_stage_flags,
              resource_info.pending_event_idx.some_ref(),
              pass_node_id);
            event_dst_stage_flags |= barrier.stage_flags;
            resource_info.pending_event_idx = nilopt;
          } else
          {
            pipeline_barriers.push_back(mem_barrier);
            pipeline_src_stage_flags |= unavailable_pipeline_stages;
            pipeline_dst_stage_flags |= barrier.stage_flags;
          }
          resource_info.cache_state.commit_wait_event_or_barrier(
            current_queue_type,
            unavailable_pipeline_stages,
            unavailable_accesses,
            barrier.stage_flags,
            barrier.access_flags);
        }
        resource_info.cache_state.commit_access(
          current_queue_type, barrier.stage_flags, barrier.access_flags);
      }

      for (const BufferAccess& barrier : pass_info.buffer_accesses)
      {
        BufferExecInfo& buffer_info = buffer_infos_[barrier.buffer_info_idx];

        if (
          is_semaphore_null(buffer_info.pending_semaphore) &&
          buffer_info.cache_state.unavailable_accesses.none() &&
          !buffer_info.cache_state.need_invalidate(barrier.stage_flags, barrier.access_flags))
        {
          buffer_info.cache_state.commit_access(
            current_queue_type, barrier.stage_flags, barrier.access_flags);
          continue;
        }

        if (is_semaphore_valid(buffer_info.pending_semaphore))
        {
          command_queues_->ref(current_queue_type)
            .wait(buffer_info.pending_semaphore, vk_cast(barrier.stage_flags));
          buffer_info.pending_semaphore.assign(TimelineSemaphore::null());
          buffer_info.cache_state.commit_wait_semaphore(
            buffer_info.cache_state.queue_owner, current_queue_type, barrier.stage_flags);
        } else
        {
          if (
            buffer_info.cache_state.unavailable_pipeline_stages.none() ||
            buffer_info.cache_state.unavailable_pipeline_stages ==
              PipelineStageFlags{PipelineStage::TOP_OF_PIPE})
          {
            SOUL_ASSERT(0, buffer_info.cache_state.unavailable_accesses.none());
          }
          SOUL_ASSERT(0, !buffer_info.cache_state.unavailable_accesses.test(AccessType::AS_WRITE));
          VkBufferMemoryBarrier mem_barrier = {
            .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask       = vk_cast(buffer_info.cache_state.unavailable_accesses),
            .dstAccessMask       = vk_cast(barrier.access_flags),
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer              = gpu_system_->buffer_ref(buffer_info.buffer_id).vk_handle,
            .offset              = 0,
            .size                = VK_WHOLE_SIZE,
          };

          if (buffer_info.pending_event_idx != nilopt)
          {
            event_buffer_barriers.push_back(mem_barrier);
            const EventInfo& event_info = event_infos_[buffer_info.pending_event_idx.some_ref()];
            wait_event(
              events,
              event_src_stage_flags,
              buffer_info.pending_event_idx.some_ref(),
              pass_node_id);
            event_dst_stage_flags |= barrier.stage_flags;
            buffer_info.pending_event_idx = nilopt;
          } else
          {
            pipeline_buffer_barriers.push_back(mem_barrier);
            pipeline_src_stage_flags |= buffer_info.cache_state.unavailable_pipeline_stages;
            pipeline_dst_stage_flags |= barrier.stage_flags;
          }

          buffer_info.cache_state.commit_wait_event_or_barrier(
            current_queue_type,
            buffer_info.cache_state.unavailable_pipeline_stages,
            buffer_info.cache_state.unavailable_accesses,
            barrier.stage_flags,
            barrier.access_flags);
        }
        buffer_info.cache_state.commit_access(
          current_queue_type, barrier.stage_flags, barrier.access_flags);
      }

      for (const TextureAccess& barrier : pass_info.texture_accesses)
      {
        SOUL_LOG_RG_EXEC(
          "Texture Access Barrier, Name : {}", texture_infos_[barrier.texture_info_idx].name);

        TextureExecInfo& texture_info  = texture_infos_[barrier.texture_info_idx];
        auto& texture                  = gpu_system_->texture_ref(texture_info.texture_id);
        TextureViewExecInfo& view_info = *texture_info.get_view(barrier.view);

        const auto layout_change = view_info.layout != barrier.layout;

        const auto queue_owner                 = view_info.cache_state.queue_owner;
        const auto unavailable_pipeline_stages = view_info.cache_state.unavailable_pipeline_stages;
        const auto unavailable_accesses        = view_info.cache_state.unavailable_accesses;

        if (
          is_semaphore_null(view_info.pending_semaphore) && !layout_change &&
          !view_info.cache_state.need_invalidate(barrier.stage_flags, barrier.access_flags) &&
          unavailable_accesses.none())
        {
          view_info.cache_state.commit_access(
            queue_owner, barrier.stage_flags, barrier.access_flags);
          continue;
        }

        VkImageMemoryBarrier mem_barrier = {
          .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
          .oldLayout           = view_info.layout,
          .newLayout           = barrier.layout,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image               = texture.vk_handle,
          .subresourceRange =
            {
              .aspectMask     = vk_cast_format_to_aspect_flags(texture.desc.format),
              .baseMipLevel   = barrier.view.get_level(),
              .levelCount     = 1,
              .baseArrayLayer = barrier.view.get_layer(),
              .layerCount     = 1,
            },
        };

        if (is_semaphore_valid(view_info.pending_semaphore))
        {
          command_queue.wait(view_info.pending_semaphore, vk_cast(barrier.stage_flags));
          view_info.pending_semaphore.assign(TimelineSemaphore::null());
          view_info.cache_state.commit_wait_semaphore(
            queue_owner, current_queue_type, barrier.stage_flags);

          if (layout_change)
          {
            semaphore_dst_stage_flags |= barrier.stage_flags;
            mem_barrier.srcAccessMask = 0;
            mem_barrier.dstAccessMask = vk_cast(barrier.access_flags);
            semaphore_layout_barriers.push_back(mem_barrier);
            SOUL_LOG_RG_EXEC(
              "Semaphore Layout Barrier for : {:#x} From : {}, To : {}",
              u64(mem_barrier.image),
              to_string(mem_barrier.oldLayout),
              to_string(mem_barrier.newLayout));

            view_info.cache_state.commit_wait_event_or_barrier(
              current_queue_type,
              barrier.stage_flags,
              {},
              barrier.stage_flags,
              barrier.access_flags,
              layout_change);
          }
        } else
        {
          const auto dst_access_flags = barrier.access_flags;

          AccessFlags src_access = unavailable_accesses;
          if (unavailable_accesses == AccessFlags{AccessType::SHADER_WRITE})
          {
            src_access.set(AccessType::SHADER_READ);
          }
          mem_barrier.srcAccessMask = vk_cast(unavailable_accesses);
          mem_barrier.dstAccessMask = vk_cast(dst_access_flags);

          SOUL_ASSERT(0, !unavailable_accesses.test(AccessType::AS_WRITE));
          if (view_info.pending_event_idx != nilopt)
          {
            event_image_barriers.push_back(mem_barrier);

            const EventInfo& event_info = event_infos_[view_info.pending_event_idx.some_ref()];
            wait_event(
              events, event_src_stage_flags, view_info.pending_event_idx.some_ref(), pass_node_id);
            event_dst_stage_flags |= barrier.stage_flags;
            view_info.pending_event_idx = nilopt;
            SOUL_LOG_RG_EXEC(
              "Event Barrier for : {:#x} From : {}, To : {}, Src Access : {:#x}, Dst Access : "
              "{:#x}",
              u64(mem_barrier.image),
              to_string(mem_barrier.oldLayout),
              to_string(mem_barrier.newLayout),
              u64(mem_barrier.srcAccessMask),
              u64(mem_barrier.dstAccessMask));
          } else
          {
            pipeline_image_barriers.push_back(mem_barrier);
            SOUL_LOG_RG_EXEC(
              "Pipeline Image Barrier : {:#x} From : {}, To : {}",
              u64(mem_barrier.image),
              to_string(mem_barrier.oldLayout),
              to_string(mem_barrier.newLayout));

            pipeline_src_stage_flags |= unavailable_pipeline_stages;
            pipeline_dst_stage_flags |= barrier.stage_flags;
          }

          view_info.cache_state.commit_wait_event_or_barrier(
            current_queue_type,
            unavailable_pipeline_stages,
            unavailable_accesses,
            barrier.stage_flags,
            barrier.access_flags,
            layout_change);
        }

        view_info.layout = barrier.layout;
        view_info.cache_state.commit_access(queue_owner, barrier.stage_flags, barrier.access_flags);
      }

      if (!semaphore_layout_barriers.empty())
      {
        SOUL_LOG_RG_EXEC("\n");
        SOUL_LOG_RG_EXEC(">>> Semaphore Layout Barrier:");
        SOUL_LOG_RG_EXEC("Src Stage : {}", u64(vk_cast(semaphore_dst_stage_flags)));
        SOUL_LOG_RG_EXEC("Dst Stage : {}", u64(vk_cast(semaphore_dst_stage_flags)));
        SOUL_LOG_RG_EXEC("Semaphore Layout Barrier List : ");
        vkCmdPipelineBarrier(
          cmd_buffer.get_vk_handle(),
          vk_cast(semaphore_dst_stage_flags),
          vk_cast(semaphore_dst_stage_flags),
          0,
          0,
          nullptr,
          0,
          nullptr,
          soul::cast<u32>(semaphore_layout_barriers.size()),
          semaphore_layout_barriers.data());

        SOUL_LOG_RG_EXEC(
          "Semaphore Layout Barrier For Pass : {}, Size : {}, src stage : {}, dst stage : {}",
          pass_info.name,
          semaphore_layout_barriers.size(),
          u64(vk_cast(semaphore_dst_stage_flags)),
          u64(vk_cast(semaphore_dst_stage_flags)));
      }

      if (!pipeline_buffer_barriers.empty() || !pipeline_image_barriers.empty())
      {
        if (pipeline_src_stage_flags.none())
        {
          pipeline_src_stage_flags = {PipelineStage::TOP_OF_PIPE};
        }
        SOUL_ASSERT(0, pipeline_dst_stage_flags.any());
        vkCmdPipelineBarrier(
          cmd_buffer.get_vk_handle(),
          vk_cast(pipeline_src_stage_flags),
          vk_cast(pipeline_dst_stage_flags),
          0,
          soul::cast<u32>(pipeline_barriers.size()),
          pipeline_barriers.data(),
          soul::cast<u32>(pipeline_buffer_barriers.size()),
          pipeline_buffer_barriers.data(),
          soul::cast<u32>(pipeline_image_barriers.size()),
          pipeline_image_barriers.data());
      }

      if (!events.empty())
      {
        SOUL_LOG_RG_EXEC("\n");
        SOUL_LOG_RG_EXEC(">>> Events:");
        SOUL_LOG_RG_EXEC("Src Stage : {}", u64(vk_cast(event_src_stage_flags)));
        SOUL_LOG_RG_EXEC("Dst Stage : {}", u64(vk_cast(event_dst_stage_flags)));
        SOUL_LOG_RG_EXEC("Event List : ");

        for (auto event : events)
        {
          SOUL_LOG_RG_EXEC("{:#x}", u64(event));
        }

        SOUL_LOG_RG_EXEC(
          "Wait Events For Pass : {}, Size : {}, src stage : {}, dst stage : {}",
          pass_info.name,
          events.size(),
          u64(vk_cast(event_src_stage_flags)),
          u64(vk_cast(event_dst_stage_flags)));

        vkCmdWaitEvents(
          cmd_buffer.get_vk_handle(),
          soul::cast<u32>(events.size()),
          events.data(),
          vk_cast(event_src_stage_flags),
          vk_cast(event_dst_stage_flags),
          soul::cast<u32>(event_barriers.size()),
          event_barriers.data(),
          soul::cast<u32>(event_buffer_barriers.size()),
          event_buffer_barriers.data(),
          soul::cast<u32>(event_image_barriers.size()),
          event_image_barriers.data());
      }

      execute_pass(pass_index, cmd_buffer);

      auto is_queue_type_dependent = FlagMap<QueueType, b8>::Fill(false);
      for (const BufferAccess& access : pass_info.buffer_accesses)
      {
        const BufferExecInfo& buffer_info = buffer_infos_[access.buffer_info_idx];
        if (buffer_info.pass_counter != buffer_info.passes.size() - 1)
        {
          u32 next_pass_idx = buffer_info.passes[buffer_info.pass_counter + 1].id;
          const auto next_queue_type =
            render_graph_->get_pass_nodes()[next_pass_idx]->get_queue_type();
          is_queue_type_dependent[next_queue_type] = true;
        }
      }

      for (const TextureAccess& access : pass_info.texture_accesses)
      {
        const TextureExecInfo& texture_info = texture_infos_[access.texture_info_idx];
        if (texture_info.first_pass.is_null())
        {
          continue;
        }
        const TextureViewExecInfo& texture_view_info = *texture_info.get_view(access.view);
        if (texture_view_info.pass_counter != texture_view_info.passes.size() - 1)
        {
          u32 next_pass_idx = texture_view_info.passes[texture_view_info.pass_counter + 1].id;
          const auto next_queue_type =
            render_graph_->get_pass_nodes()[next_pass_idx]->get_queue_type();
          is_queue_type_dependent[next_queue_type] = true;
        }
      }

      for (const ResourceAccess& access : pass_info.resource_accesses)
      {
        const ResourceExecInfo& resource_info = resource_infos_[access.resource_info_idx];
        if (resource_info.pass_counter != resource_info.passes.size() - 1)
        {
          u32 next_pass_idx = resource_info.passes[resource_info.pass_counter + 1].id;
          const auto next_queue_type =
            render_graph_->get_pass_nodes()[next_pass_idx]->get_queue_type();
          is_queue_type_dependent[next_queue_type] = true;
        }
      }

      Option<u32> event_idx = nilopt;
      PipelineStageFlags unsync_write_stage_flags;

      for (QueueType queue_type : FlagIter<QueueType>())
      {
        if (
          is_queue_type_dependent[queue_type] && queue_type == pass_node->get_queue_type() &&
          queue_type != QueueType::TRANSFER)
        {
          event_idx = event_infos_.size();
          event_infos_.push_back(EventInfo{gpu_system_->create_event(), {}});
        }
      }

      SBOVector<Semaphore*> pending_semaphores;

      for (const BufferAccess& barrier : pass_info.buffer_accesses)
      {
        BufferExecInfo& buffer_info = buffer_infos_[barrier.buffer_info_idx];
        if (buffer_info.pass_counter != buffer_info.passes.size() - 1)
        {
          const auto next_pass_idx = buffer_info.passes[buffer_info.pass_counter + 1].id;
          const auto next_queue_type =
            render_graph_->get_pass_nodes()[next_pass_idx]->get_queue_type();

          if (current_queue_type != next_queue_type)
          {
            pending_semaphores.push_back(&buffer_info.pending_semaphore);
          } else
          {
            buffer_info.pending_event_idx = event_idx;
            unsync_write_stage_flags |= barrier.stage_flags;
          }
        }
      }

      for (const TextureAccess& barrier : pass_info.texture_accesses)
      {
        TextureExecInfo& texture_info          = texture_infos_[barrier.texture_info_idx];
        TextureViewExecInfo& texture_view_info = *texture_info.get_view(barrier.view);
        if (texture_view_info.pass_counter != texture_view_info.passes.size() - 1)
        {
          const auto next_pass_idx =
            texture_view_info.passes[texture_view_info.pass_counter + 1].id;
          const auto next_queue_type =
            render_graph_->get_pass_nodes()[next_pass_idx]->get_queue_type();
          if (current_queue_type != next_queue_type)
          {
            pending_semaphores.push_back(&texture_view_info.pending_semaphore);
          } else
          {
            texture_view_info.pending_event_idx = event_idx;
            unsync_write_stage_flags |= barrier.stage_flags;
          }
        }
        texture_view_info.layout = barrier.layout;
      }

      for (const ResourceAccess& access : pass_info.resource_accesses)
      {
        ResourceExecInfo& resource_info = resource_infos_[access.resource_info_idx];
        if (resource_info.pass_counter != resource_info.passes.size() - 1)
        {
          const auto next_pass_idx = resource_info.passes[resource_info.pass_counter + 1].id;
          const auto next_queue_type =
            render_graph_->get_pass_nodes()[next_pass_idx]->get_queue_type();

          if (current_queue_type != next_queue_type)
          {
            pending_semaphores.push_back(&resource_info.pending_semaphore);
          } else
          {
            resource_info.pending_event_idx = event_idx;
            unsync_write_stage_flags |= access.stage_flags;
          }
        }
      }

      if (event_idx != nilopt)
      {
        set_event(cmd_buffer, event_idx.some_ref(), unsync_write_stage_flags);
        SOUL_LOG_RG_EXEC("Set Event : {:#x}", u64(event_infos_[event_idx.some_ref()].vk_handle));
      }

      vkCmdEndDebugUtilsLabelEXT(cmd_buffer.get_vk_handle());
      command_queue.submit(cmd_buffer);

      for (Semaphore* pending_semaphore : pending_semaphores)
      {
        pending_semaphore->assign(command_queue.get_timeline_semaphore());
      }

      // Update unsync stage
      for (const BufferAccess& barrier : pass_info.buffer_accesses)
      {
        BufferExecInfo& buffer_info = buffer_infos_[barrier.buffer_info_idx];
        buffer_info.pass_counter += 1;
      }

      for (const TextureAccess& barrier : pass_info.texture_accesses)
      {
        TextureExecInfo& texture_info          = texture_infos_[barrier.texture_info_idx];
        TextureViewExecInfo& texture_view_info = *texture_info.get_view(barrier.view);
        texture_view_info.pass_counter += 1;
      }

      for (const auto& access : pass_info.resource_accesses)
      {
        ResourceExecInfo& resource_info = resource_infos_[access.resource_info_idx];
        resource_info.pass_counter += 1;
      }
    }

    for (const TextureExecInfo& texture_info : external_texture_infos_)
    {
      auto& texture = gpu_system_->texture_ref(texture_info.texture_id);

      VkImageLayout layout = texture_info.view->layout;
      SOUL_ASSERT(
        0,
        std::all_of(
          texture_info.view,
          texture_info.view + texture_info.get_view_count(),
          [layout](const TextureViewExecInfo& view_info)
          {
            return view_info.layout == layout;
          }),
        "");

      if (texture_info.view->passes.empty())
      {
        continue;
      }
      u64 last_pass_idx = texture_info.view->passes.back().id;
      SOUL_ASSERT(
        0,
        std::all_of(
          texture_info.view,
          texture_info.view + texture_info.get_view_count(),
          [last_pass_idx](const TextureViewExecInfo& view_info)
          {
            return view_info.passes.back().id == last_pass_idx;
          }),
        "");

      texture.layout = layout;
      SOUL_ASSERT(0, texture_info.get_view_count() > 0);
      texture.cache_state = texture_info.view[0].cache_state;
      for (auto view_idx = 1u; view_idx < texture_info.get_view_count(); view_idx++)
      {
        texture.cache_state.join(texture_info.view[view_idx].cache_state);
      }
    }

    for (const BufferExecInfo& buffer_info : buffer_infos_)
    {
      if (buffer_info.passes.empty())
      {
        continue;
      }
      auto& buffer       = gpu_system_->buffer_ref(buffer_info.buffer_id);
      buffer.cache_state = buffer_info.cache_state;
    }

    const auto external_tlas_list = render_graph_->get_external_tlas_list();
    for (auto external_tlas_idx = 0u; external_tlas_idx < external_tlas_list.size();
         external_tlas_idx++)
    {
      auto& tlas = gpu_system_->tlas_ref(external_tlas_list[external_tlas_idx].tlas_id);
      const auto& resource_info = external_tlas_resource_infos_[external_tlas_idx];
      tlas.cache_state          = resource_info.cache_state;
    }

    for (VkEvent event : garbage_events)
    {
      gpu_system_->destroy_event(event);
    }
  }

  auto RenderGraphExecution::is_external(const BufferExecInfo& info) const -> b8
  {
    return soul::cast<u32>(&info - buffer_infos_.data()) >=
           render_graph_->get_internal_buffers().size();
  }

  auto RenderGraphExecution::is_external(const TextureExecInfo& info) const -> b8
  {
    return soul::cast<u32>(&info - texture_infos_.data()) >=
           render_graph_->get_internal_textures().size();
  }

  auto RenderGraphExecution::get_buffer_id(const BufferNodeID node_id) const -> BufferID
  {
    const auto info_idx = get_buffer_info_index(node_id);
    return buffer_infos_[info_idx].buffer_id;
  }

  auto RenderGraphExecution::get_texture_id(const TextureNodeID node_id) const -> TextureID
  {
    const auto info_idx = get_texture_info_index(node_id);
    return texture_infos_[info_idx].texture_id;
  }

  auto RenderGraphExecution::get_tlas_id(TlasNodeID node_id) const -> TlasID
  {
    const auto& node = render_graph_->get_resource_node(node_id);
    SOUL_ASSERT(0, node.resource_id.is_external());
    return render_graph_->get_external_tlas_list()[node.resource_id.get_index()].tlas_id;
  }

  auto RenderGraphExecution::get_buffer(const BufferNodeID node_id) const -> Buffer&
  {
    return gpu_system_->buffer_ref(get_buffer_id(node_id));
  }

  auto RenderGraphExecution::get_texture(const TextureNodeID node_id) const -> Texture&
  {
    return gpu_system_->texture_ref(get_texture_id(node_id));
  }

  auto RenderGraphExecution::get_buffer_info_index(const BufferNodeID node_id) const -> u32
  {
    const auto& node = render_graph_->get_resource_node(node_id);
    if (node.resource_id.is_external())
    {
      return soul::cast<u32>(render_graph_->get_internal_buffers().size()) +
             node.resource_id.get_index();
    }
    return node.resource_id.get_index();
  }

  auto RenderGraphExecution::get_texture_info_index(TextureNodeID nodeID) const -> u32
  {
    const auto& node = render_graph_->get_resource_node(nodeID);
    if (node.resource_id.is_external())
    {
      return soul::cast<u32>(render_graph_->get_internal_textures().size()) +
             node.resource_id.get_index();
    }
    return node.resource_id.get_index();
  }

  auto RenderGraphExecution::get_tlas_resource_info_index(TlasNodeID node_id) const -> u32
  {
    const auto& node = render_graph_->get_resource_node(node_id);
    SOUL_ASSERT(0, node.resource_id.is_external());
    return soul::cast<u32>(external_tlas_resource_infos_.begin() - resource_infos_.begin()) +
           node.resource_id.get_index();
  }

  auto RenderGraphExecution::get_blas_group_resource_info_index(BlasGroupNodeID node_id) const
    -> u32
  {
    const auto& node = render_graph_->get_resource_node(node_id);
    SOUL_ASSERT(0, node.resource_id.is_external());
    return soul::cast<u32>(external_blas_group_resource_infos_.begin() - resource_infos_.begin()) +
           node.resource_id.get_index();
  }

  void RenderGraphExecution::cleanup()
  {
    for (const auto& event_info : event_infos_)
    {
      gpu_system_->destroy_event(event_info.vk_handle);
    }

    for (const auto& texture_info : internal_texture_infos_)
    {
      if (texture_info.usage_flags.none())
      {
        continue;
      }
      gpu_system_->destroy_texture(texture_info.texture_id);
    }
  }

  void RenderGraphExecution::init_shader_buffers(
    const std::span<const ShaderBufferReadAccess> access_list,
    PassNodeID pass_node_id,
    const QueueType queue_type)
  {
    PassExecInfo& pass_info = pass_infos_[pass_node_id.id];
    for (const ShaderBufferReadAccess& shader_access : access_list)
    {
      SOUL_ASSERT(0, shader_access.node_id.is_valid());

      const auto buffer_info_id = get_buffer_info_index(shader_access.node_id);

      const auto stage_flags = cast_to_pipeline_stage_flags(shader_access.stage_flags);

      pass_info.buffer_accesses.push_back(BufferAccess{
        .stage_flags     = stage_flags,
        .access_flags    = {AccessType::SHADER_READ},
        .buffer_info_idx = buffer_info_id,
      });

      update_buffer_info(
        queue_type,
        get_buffer_usage_flags(shader_access.usage),
        pass_node_id,
        &buffer_infos_[buffer_info_id]);
    }
  }

  void RenderGraphExecution::init_shader_buffers(
    std::span<const ShaderBufferWriteAccess> access_list,
    PassNodeID pass_node_id,
    const QueueType queue_type)
  {
    PassExecInfo& pass_info = pass_infos_[pass_node_id.id];
    for (const auto& shader_access : access_list)
    {
      SOUL_ASSERT(0, shader_access.output_node_id.is_valid());

      const auto buffer_info_id = get_buffer_info_index(shader_access.output_node_id);

      const auto stage_flags = cast_to_pipeline_stage_flags(shader_access.stage_flags);

      pass_info.buffer_accesses.push_back(BufferAccess{
        .stage_flags     = stage_flags,
        .access_flags    = {AccessType::SHADER_READ, AccessType::SHADER_WRITE},
        .buffer_info_idx = buffer_info_id,
      });

      update_buffer_info(
        queue_type,
        get_buffer_usage_flags(shader_access.usage),
        pass_node_id,
        &buffer_infos_[buffer_info_id]);
    }
  }

  void RenderGraphExecution::init_shader_textures(
    std::span<const ShaderTextureReadAccess> access_list,
    PassNodeID pass_node_id,
    const QueueType queue_type)
  {
    PassExecInfo& pass_info = pass_infos_[pass_node_id.id];
    for (const auto& shader_access : access_list)
    {
      SOUL_ASSERT(0, shader_access.node_id.is_valid());

      const auto texture_info_id = get_texture_info_index(shader_access.node_id);
      const auto stage_flags     = cast_to_pipeline_stage_flags(shader_access.stage_flags);

      update_texture_info(
        queue_type,
        get_texture_usage_flags(shader_access.usage),
        PassNodeID(pass_node_id),
        shader_access.view_range,
        &texture_infos_[texture_info_id]);

      auto is_writable = [](const ShaderTextureReadUsage usage) -> b8
      {
        auto mapping = FlagMap<ShaderTextureReadUsage, b8>::FromValues({false, true});
        return mapping[usage];
      };

      auto image_layout = is_writable(shader_access.usage)
                            ? VK_IMAGE_LAYOUT_GENERAL
                            : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      std::ranges::transform(
        shader_access.view_range,
        std::back_inserter(pass_info.texture_accesses),
        [stage_flags, image_layout, texture_info_id](SubresourceIndex view_index) -> TextureAccess
        {
          return {
            .stage_flags      = stage_flags,
            .access_flags     = {AccessType::SHADER_READ},
            .layout           = image_layout,
            .texture_info_idx = texture_info_id,
            .view             = view_index,
          };
        });
    }
  }

  void RenderGraphExecution::init_shader_textures(
    std::span<const ShaderTextureWriteAccess> access_list,
    PassNodeID pass_node_id,
    const QueueType queue_type)
  {
    PassExecInfo& pass_info = pass_infos_[pass_node_id.id];
    for (const auto& shader_access : access_list)
    {
      SOUL_ASSERT(0, shader_access.output_node_id.is_valid());

      const auto texture_info_id = get_texture_info_index(shader_access.output_node_id);
      const auto stage_flags     = cast_to_pipeline_stage_flags(shader_access.stage_flags);

      update_texture_info(
        queue_type,
        get_texture_usage_flags(shader_access.usage),
        PassNodeID(pass_node_id),
        shader_access.view_range,
        &texture_infos_[texture_info_id]);

      std::ranges::transform(
        shader_access.view_range,
        std::back_inserter(pass_info.texture_accesses),
        [stage_flags, texture_info_id](const SubresourceIndex& view_index) -> TextureAccess
        {
          return {
            .stage_flags      = stage_flags,
            .access_flags     = {AccessType::SHADER_READ, AccessType::SHADER_WRITE},
            .layout           = VK_IMAGE_LAYOUT_GENERAL,
            .texture_info_idx = texture_info_id,
            .view             = view_index};
        });
    }
  }

  void RenderGraphExecution::init_shader_tlas_accesses(
    const std::span<const ShaderTlasReadAccess> access_list,
    PassNodeID pass_node_id,
    const QueueType queue_type)
  {
    PassExecInfo& pass_info = pass_infos_[pass_node_id.id];
    for (const ShaderTlasReadAccess& shader_access : access_list)
    {
      SOUL_ASSERT(0, shader_access.node_id.is_valid());

      const auto resource_info_id = get_tlas_resource_info_index(shader_access.node_id);

      const auto stage_flags = cast_to_pipeline_stage_flags(shader_access.stage_flags);

      pass_info.resource_accesses.push_back(ResourceAccess{
        .stage_flags       = stage_flags,
        .access_flags      = {AccessType::AS_READ},
        .resource_info_idx = resource_info_id,
      });

      update_resource_info(
        queue_type, PassNodeID(pass_node_id), &resource_infos_[resource_info_id]);
    }
  }

  void RenderGraphExecution::init_shader_blas_group_accesses(
    std::span<const ShaderBlasGroupReadAccess> access_list,
    PassNodeID pass_node_id,
    QueueType queue_type)
  {
    PassExecInfo& pass_info = pass_infos_[pass_node_id.id];
    for (const ShaderBlasGroupReadAccess& shader_access : access_list)
    {
      SOUL_ASSERT(0, shader_access.node_id.is_valid());

      const auto resource_info_id = get_blas_group_resource_info_index(shader_access.node_id);

      const auto stage_flags = cast_to_pipeline_stage_flags(shader_access.stage_flags);

      pass_info.resource_accesses.push_back(ResourceAccess{
        .stage_flags       = stage_flags,
        .access_flags      = {AccessType::AS_READ},
        .resource_info_idx = resource_info_id,
      });

      update_resource_info(
        queue_type, PassNodeID(pass_node_id), &resource_infos_[resource_info_id]);
    }
  }

  void RenderGraphExecution::wait_event(
    Vector<VkEvent>& events,
    PipelineStageFlags& stage_flags,
    u32 event_idx,
    PassNodeID pass_node_id)
  {
    EventInfo& event_info = event_infos_[event_idx];
    if (event_info.last_wait_pass_node_id == pass_node_id)
    {
      return;
    }
    event_info.last_wait_pass_node_id = pass_node_id;
    stage_flags |= event_info.src_stage_flags;
    events.push_back(event_info.vk_handle);
  }

  void RenderGraphExecution::set_event(
    PrimaryCommandBuffer command_buffer, u32 event_idx, PipelineStageFlags stage_flags)
  {
    EventInfo& event_info = event_infos_[event_idx];
    vkCmdSetEvent(command_buffer.get_vk_handle(), event_info.vk_handle, vk_cast(stage_flags));
    event_info.src_stage_flags = stage_flags;
  }

} // namespace soul::gpu::impl
