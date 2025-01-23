#pragma once

#include "core/config.h"
#include "core/sbo_vector.h"
#include "core/type_traits.h"
#include "core/vector.h"

#include "misc/image_data.h"

#include "gpu/render_graph.h"
#include "gpu/type.h"
#include "render_node.h"
#include "runtime/runtime.h"
#include "scene.h"

#include "render_pipeline.shared.hlsl"

using namespace soul;

namespace soul::gpu
{
  class RenderGraph;
  class System;
}; // namespace soul::gpu

namespace soul::app
{
  class Gui;
}

namespace renderlab
{
  struct RenderNodeChannel
  {
    u32 node_index;
    String name;
  };

  class RenderPipeline
  {
  private:
    struct RenderNodeInput
    {
      String channel_name;
      u32 src_node_index;
      String src_channel_name;
    };

    struct RenderNodeContext
    {
      NotNull<RenderNode*> node;
      String name;
      SBOVector<RenderNodeInput> input_textures = SBOVector<RenderNodeInput>();
      SBOVector<RenderNodeInput> input_buffers  = SBOVector<RenderNodeInput>();
    };

    memory::Allocator* allocator_;
    NotNull<gpu::System*> gpu_system_;
    NotNull<const Scene*> scene_;
    RenderConstant render_constant_;
    Vector<String> render_constant_texture_names_;
    Vector<RenderNodeContext> render_node_contexts_;
    Vector<RenderData> node_outputs_;
    HashMap<String, u32> name_to_node_index_;

    Option<u32> selected_node_idx_;
    Option<String> selected_field_name_;
    PostProcessOption postprocess_option_;
    Array<ValueOption, 4> value_options_;

    gpu::ProgramID program_id_;
    gpu::TextureNodeID output_node_id_;

  public:
    explicit RenderPipeline(
      NotNull<const Scene*> scene, NotNull<memory::Allocator*> allocator = get_default_allocator());

    RenderPipeline(const RenderPipeline&) = delete;

    RenderPipeline(RenderPipeline&&) = default;

    auto operator=(const RenderPipeline&) -> RenderPipeline& = delete;

    auto operator=(RenderPipeline&&) -> RenderPipeline& = default;

    ~RenderPipeline();

    template <ts_invocable Fn>
      requires(can_convert_v<invoke_result_t<Fn>*, RenderNode*>)
    void generate_node(String&& name, Fn render_node_fn)
    {
      auto result_node = allocator_->generate(render_node_fn);
      render_node_contexts_.push_back(RenderNodeContext{
        .node = result_node.some_ref().get(),
        .name = name.clone(),
      });

      name_to_node_index_.insert(std::move(name), render_node_contexts_.size() - 1);
    }

    template <ts_fn<void, const RenderNode&, StringView> Fn>
    void for_each_node(Fn fn) const
    {
      for (const auto& context : render_node_contexts_)
      {
        fn(*context.node, context.name.cview());
      }
    }

    void create_constant_texture(String&& name, const ImageData& image_data, b8 srgb);

    void create_constant_texture(
      String&& name, const gpu::TextureDesc& desc, const gpu::TextureLoadDesc& load_desc);

    void create_constant_buffer(String&& name, const gpu::BufferDesc& desc, const void* data);

    void add_texture_edge(
      StringView src_node, StringView src_channel, StringView dst_node, StringView dst_channel);

    void add_buffer_edge(
      StringView src_node, StringView src_channel, StringView dst_node, StringView dst_channel);

    void set_output(StringView node_name, StringView channel_name);

    [[nodiscard]]
    auto get_output() const -> gpu::TextureNodeID;

    auto get_node(StringView name) -> NotNull<RenderNode*>;

    void submit_passes(NotNull<gpu::RenderGraph*> render_graph);

    void on_gui_render(NotNull<app::Gui*> gui);
  };

} // namespace renderlab
