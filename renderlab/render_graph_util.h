#include "core/sbo_vector.h"
#include "gpu/render_graph.h"
#include "gpu/render_graph_registry.h"
#include "gpu/type.h"

namespace renderlab
{
  struct RenderGraphUtil
  {
    struct CopyTexturePassParameter
    {
      gpu::TextureNodeID src_node_id;
      gpu::TextureNodeID dst_node_id;
      gpu::TextureRegionCopy region_copy;
    };

    static auto AddCopyTexturePass(
      NotNull<gpu::RenderGraph*> render_graph, CompStr name, CopyTexturePassParameter input_param)
      -> const auto&
    {
      return render_graph->add_non_shader_pass<CopyTexturePassParameter>(
        name,
        gpu::QueueType::GRAPHIC,
        [=](auto& parameter, auto& builder)
        {
          parameter.src_node_id = builder.add_src_texture(input_param.src_node_id);
          parameter.dst_node_id =
            builder.add_dst_texture(input_param.dst_node_id, gpu::TransferDataSource::GPU);
          parameter.region_copy = input_param.region_copy;
        },
        [](const auto& parameter, auto& registry, auto& command_list)
        {
          command_list.push(gpu::RenderCommandCopyTexture{
            .src_texture = registry.get_texture(parameter.src_node_id),
            .dst_texture = registry.get_texture(parameter.dst_node_id),
            .regions     = u32cspan(&parameter.region_copy, 1),
          });
        });
    }

    using BatchCopyTexturePassParameter = SBOVector<CopyTexturePassParameter>;

    static auto AddBatchCopyTexturePass(
      NotNull<gpu::RenderGraph*> render_graph,
      CompStr name,
      Span<const CopyTexturePassParameter*> input_params) -> const auto&
    {
      return render_graph->add_non_shader_pass<BatchCopyTexturePassParameter>(
        name,
        gpu::QueueType::GRAPHIC,
        [&](auto& parameter, auto& builder)
        {
          for (const auto& input_param : input_params)
          {
            parameter.push_back(CopyTexturePassParameter{
              .src_node_id = builder.add_src_texture(input_param.src_node_id),
              .dst_node_id =
                builder.add_dst_texture(input_param.dst_node_id, gpu::TransferDataSource::GPU),
              .region_copy = input_param.region_copy,
            });
          }
        },
        [](const auto& parameters, auto& registry, auto& command_list)
        {
          for (const auto copy_param : parameters)
          {
            command_list.push(gpu::RenderCommandCopyTexture{
              .src_texture = registry.get_texture(copy_param.src_node_id),
              .dst_texture = registry.get_texture(copy_param.dst_node_id),
              .regions     = u32cspan(&copy_param.region_copy, 1),
            });
          }
        });
    }

    static auto CreateDuplicateTexture(
      NotNull<gpu::RenderGraph*> render_graph,
      const gpu::System& gpu_system,
      CompStr name,
      gpu::TextureNodeID src_node_id) -> gpu::TextureNodeID

    {
      const auto src_texture_desc = render_graph->get_texture_desc(src_node_id, gpu_system);
      const auto dst_node =
        render_graph->create_texture(String::From(name.c_str()), src_texture_desc);

      const auto region_copy = gpu::TextureRegionCopy{
        .src_subresource = {.layer_count = src_texture_desc.layer_count},
        .dst_subresource = {.layer_count = src_texture_desc.layer_count},
        .extent          = src_texture_desc.extent,
      };

      return AddCopyTexturePass(
               render_graph,
               "Copy Pass For Duplicate Texture"_str,
               {src_node_id, dst_node, region_copy})
        .get_parameter()
        .dst_node_id;
    }
  };
} // namespace renderlab
