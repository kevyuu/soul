#include "render_pipeline.h"
#include "app/gui.h"
#include "gpu/render_graph.h"
#include "gpu/type.h"
#include "misc/image_data.h"
#include "render_pipeline.shared.hlsl"
#include "runtime/data.h"
#include "runtime/runtime.h"

#include "utils/util.h"

namespace renderlab
{
  RenderPipeline::RenderPipeline(NotNull<const Scene*> scene, NotNull<memory::Allocator*> allocator)
      : allocator_(allocator), gpu_system_(scene->get_gpu_system()), scene_(scene)
  {
    value_options_[0] = ValueOption::X;
    value_options_[1] = ValueOption::Y;
    value_options_[2] = ValueOption::Z;
    value_options_[3] = ValueOption::W;
    program_id_ =
      util::create_compute_program(scene->get_gpu_system(), "render_pipeline_main.hlsl"_str);
  }

  RenderPipeline::~RenderPipeline()
  {
    for (const auto& context : render_node_contexts_)
    {
      allocator_->destroy(context.node);
    }

    for (const auto& texture_name : render_constant_.texture_names)
    {
      gpu_system_->destroy_texture(render_constant_.textures[texture_name]);
    }

    for (const auto& buffer_name : render_constant_.buffer_names)
    {
      gpu_system_->destroy_texture(render_constant_.textures[buffer_name]);
    }
  }

  void RenderPipeline::create_constant_texture(String&& name, const ImageData& image_data, b8 srgb)
  {
    const gpu::TextureFormat format = [&]
    {
      if (image_data.channel_count() == 1)
      {
        return gpu::TextureFormat::R8;
      } else
      {
        SOUL_ASSERT(0, image_data.channel_count() == 4);
        if (srgb)
        {
          return gpu::TextureFormat::SRGBA8;
        } else
        {
          return gpu::TextureFormat::RGBA8;
        }
      }
    }();

    const auto usage        = gpu::TextureUsageFlags({gpu::TextureUsage::SAMPLED});
    const auto texture_desc = gpu::TextureDesc::d2(
      format,
      1,
      usage,
      {
        gpu::QueueType::GRAPHIC,
        gpu::QueueType::COMPUTE,
      },
      image_data.dimension());

    const gpu::TextureRegionUpdate region_load = {
      .subresource = {.layer_count = 1},
      .extent      = vec3u32(image_data.dimension(), 1),
    };

    const auto raw_data = image_data.cspan();

    const gpu::TextureLoadDesc load_desc = {
      .data            = raw_data.data(),
      .data_size       = raw_data.size_in_bytes(),
      .regions         = {&region_load, 1},
      .generate_mipmap = false,
    };

    create_constant_texture(std::move(name), texture_desc, load_desc);
  }

  void RenderPipeline::create_constant_texture(
    String&& name, const gpu::TextureDesc& desc, const gpu::TextureLoadDesc& load_desc)
  {
    const auto texture_id = gpu_system_->create_texture(name.clone(), desc, load_desc);
    gpu_system_->flush_texture(texture_id, desc.usage_flags);

    render_constant_.texture_names.push_back(name.clone());
    render_constant_.textures.insert(std::move(name), texture_id);
  }

  void RenderPipeline::create_constant_buffer(
    String&& name, const gpu::BufferDesc& desc, const void* data)
  {
    const auto buffer_id = gpu_system_->create_buffer(name.clone(), desc, data);
    gpu_system_->flush_buffer(buffer_id);

    render_constant_.buffer_names.push_back(name.clone());
    render_constant_.buffers.insert(std::move(name), buffer_id);
  }

  void RenderPipeline::add_texture_edge(
    StringView src_node, StringView src_channel, StringView dst_node, StringView dst_channel)
  {
    SOUL_ASSERT(0, name_to_node_index_.contains(src_node));
    SOUL_ASSERT(0, name_to_node_index_.contains(dst_node));
    const auto src_node_index = name_to_node_index_[src_node];
    const auto dst_node_index = name_to_node_index_[dst_node];
    SOUL_ASSERT(0, src_node_index < dst_node_index);
    render_node_contexts_[dst_node_index].input_textures.push_back(RenderNodeInput{
      .channel_name     = String::From(dst_channel),
      .src_node_index   = src_node_index,
      .src_channel_name = String::From(src_channel),
    });
  }

  void RenderPipeline::add_buffer_edge(
    StringView src_node, StringView src_channel, StringView dst_node, StringView dst_channel)
  {
    SOUL_ASSERT(0, name_to_node_index_.contains(src_node));
    SOUL_ASSERT(0, name_to_node_index_.contains(dst_node));
    const auto src_node_index = name_to_node_index_[src_node];
    const auto dst_node_index = name_to_node_index_[dst_node];
    SOUL_ASSERT(0, src_node_index < dst_node_index);
    render_node_contexts_[dst_node_index].input_buffers.push_back(RenderNodeInput{
      .channel_name     = String::From(dst_channel),
      .src_node_index   = src_node_index,
      .src_channel_name = String::From(src_channel),
    });
  }

  void RenderPipeline::set_output(StringView node_name, StringView channel_name)
  {
    selected_node_idx_   = name_to_node_index_[node_name];
    selected_field_name_ = String::From(channel_name);
  }

  auto RenderPipeline::get_output() const -> gpu::TextureNodeID
  {
    return output_node_id_;
  }

  void RenderPipeline::submit_passes(NotNull<gpu::RenderGraph*> render_graph)
  {
    node_outputs_.cleanup();
    node_outputs_.reserve(render_node_contexts_.size());

    gpu::TextureNodeID overlay_texture_node = render_graph->create_texture(
      "Overlay Texture Node"_str,
      gpu::RGTextureDesc::create_d2(
        gpu::TextureFormat::RGBA8,
        1,
        scene_->get_viewport(),
        true,
        gpu::ClearValue::Color({0, 0, 0, 0})));

    for (const auto& context : render_node_contexts_)
    {
      NotNull<RenderNode*> render_node = context.node;

      RenderData inputs;
      for (const auto& texture_input : context.input_textures)
      {
        inputs.textures.insert(
          texture_input.channel_name.clone(),
          node_outputs_[texture_input.src_node_index].textures[texture_input.src_channel_name]);
      }
      for (const auto& buffer_input : context.input_buffers)
      {
        inputs.buffers.insert(
          buffer_input.channel_name.clone(),
          node_outputs_[buffer_input.src_node_index].buffers[buffer_input.src_channel_name]);
      }
      inputs.overlay_texture = overlay_texture_node;
      node_outputs_.push_back(
        render_node->submit_pass(*scene_, render_constant_, inputs, render_graph));
      if (node_outputs_.back().overlay_texture.is_valid())
      {
        overlay_texture_node = node_outputs_.back().overlay_texture;
      }
    }

    struct RenderPipelineParameter
    {
      gpu::TextureNodeID input_texture;
      gpu::TextureNodeID overlay_texture;
      gpu::TextureNodeID output_texture;
    };

    const auto input_texture = [this]()
    {
      if (selected_node_idx_.is_some() && selected_field_name_.is_some())
      {
        const auto node_idx      = selected_node_idx_.unwrap();
        const auto& channel_name = selected_field_name_.some_ref();
        return node_outputs_[node_idx].textures[channel_name.cspan()];
      } else
      {
        return gpu::TextureNodeID();
      }
    }();

    const auto output_dim = [this, input_texture, &render_graph]()
    {
      if (input_texture.is_null())
      {
        return vec2u32(8, 8);
      } else
      {
        const auto texture_desc =
          render_graph->get_texture_desc(input_texture, *scene_->get_gpu_system());
        return texture_desc.extent.xy();
      }
    }();

    auto output_texture = render_graph->create_texture(
      "Render Pipeline Output Texture"_str,
      gpu::RGTextureDesc::create_d2(
        gpu::TextureFormat::RGBA8,
        1,
        output_dim,
        true,
        gpu::ClearValue(vec4f32(0, 0, 0, 1), 0, 0)));

    if (input_texture.is_valid())
    {
      const auto& output_pass_node = render_graph->add_compute_pass<RenderPipelineParameter>(
        "Render Pipeline Output Pass"_str,
        [&](auto& parameter, auto& builder)
        {
          parameter.input_texture   = builder.add_srv(input_texture);
          parameter.overlay_texture = builder.add_srv(overlay_texture_node);
          parameter.output_texture  = builder.add_uav(output_texture);
        },
        [this, output_dim](const auto& parameter, auto& registry, auto& command_list)
        {
          RenderPipelinePC push_constant = {
            .input_texture      = registry.get_srv_descriptor_id(parameter.input_texture),
            .overlay_texture    = registry.get_srv_descriptor_id(parameter.overlay_texture),
            .output_texture     = registry.get_uav_descriptor_id(parameter.output_texture),
            .postprocess_option = PostProcessOption::ACES,
            .value_options =
              {
                value_options_[0],
                value_options_[1],
                value_options_[2],
                value_options_[3],
              },
          };

          const gpu::ComputePipelineStateDesc desc = {.program_id = program_id_};
          const auto pipeline_state_id             = registry.get_pipeline_state(desc);
          using Command                            = gpu::RenderCommandDispatch;
          command_list.template push<Command>({
            .pipeline_state_id  = pipeline_state_id,
            .push_constant_data = &push_constant,
            .push_constant_size = sizeof(RenderPipelinePC),
            .group_count =
              vec3u32{output_dim.x / WORK_GROUP_SIZE_X, output_dim.y / WORK_GROUP_SIZE_Y, 1},
          });
        });
      output_node_id_ = output_pass_node.get_parameter().output_texture;
    } else
    {
      output_node_id_ = output_texture;
    }
  }

  void RenderPipeline::on_gui_render(NotNull<app::Gui*> gui)

  {
    for (const auto& context : render_node_contexts_)
    {
      if (gui->collapsing_header(context.name.cspan()))
      {
        context.node->on_gui_render(gui);
      }
    }

    const StringView node_combo_str =
      !selected_node_idx_.is_some()
        ? ""_str
        : render_node_contexts_[selected_node_idx_.some_ref()].name.cspan();

    if (gui->begin_combo("Node"_str, node_combo_str))
    {
      for (usize node_i = 0; node_i < render_node_contexts_.size(); node_i++)
      {
        const auto& context  = render_node_contexts_[node_i];
        const b8 is_selected = selected_node_idx_.is_some_and(
          [node_i](u32 idx) -> b8
          {
            return idx == node_i;
          });

        if (gui->selectable(context.name.cspan(), is_selected))
        {
          selected_node_idx_   = node_i;
          selected_field_name_ = nilopt;
        }
        if (is_selected)
        {
          gui->set_item_default_focus();
        }
      }
      gui->end_combo();
    }

    const StringView output_combo_str =
      selected_field_name_.is_some() ? selected_field_name_.some_ref().cspan() : ""_str;
    if (gui->begin_combo("Output"_str, output_combo_str))
    {
      const NotNull<const RenderNode*> selected_node =
        render_node_contexts_[selected_node_idx_.unwrap()].node;
      for (const RenderNodeField& field : selected_node->get_output_fields())
      {
        const b8 is_selected = selected_field_name_.is_some_and(
          [&field](const String& name)
          {
            return name == field.name;
          });

        if (gui->selectable(field.name, is_selected))
        {
          selected_field_name_ = String::From(field.name);
        }
        if (is_selected)
        {
          gui->set_item_default_focus();
        }
      }
      gui->end_combo();
    }

    static constexpr FlagMap<ValueOption, CompStr> VALUE_OPTION_STR = {
      "x"_str,
      "y"_str,
      "z"_str,
      "w"_str,
      "one"_str,
      "zero"_str,
    };

    gui->text("Channel source : "_str);
    for (i32 channel_i = 0; channel_i < value_options_.size(); channel_i++)
    {
      gui->push_id(channel_i);
      if (gui->begin_combo(""_str, VALUE_OPTION_STR[value_options_[channel_i]]))
      {
        for (const auto option : FlagIter<ValueOption>())
        {
          const b8 is_selected = value_options_[channel_i] == option;
          if (gui->selectable(VALUE_OPTION_STR[option], is_selected))
          {
            value_options_[channel_i] = option;
          }
          if (is_selected)
          {
            gui->set_item_default_focus();
          }
        }
        gui->end_combo();
      }
      gui->pop_id();
    }
  }
} // namespace renderlab
