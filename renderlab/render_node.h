#pragma once

#include "core/string.h"
#include "gpu/id.h"
#include "gpu/render_graph.h"
#include "scene.h"

using namespace soul;

namespace soul::gpu
{
  class RenderGraph;
} // namespace soul::gpu

namespace soul::app
{
  class Gui;
}

namespace renderlab
{
  struct RenderData
  {
    HashMap<String, gpu::TextureNodeID> textures;
    HashMap<String, gpu::BufferNodeID> buffers;

    gpu::TextureNodeID overlay_texture = gpu::TextureNodeID();
  };

  struct OverlayRenderVariable
  {
    gpu::TextureNodeID color_texture;
    gpu::TextureNodeID depth_texture;
  };

  struct RenderConstant
  {
    // TODO(kevinyu) : Implement iterable HashMap and use that instead of having separate vector for
    // names
    HashMap<String, gpu::TextureID> textures;
    Vector<String> texture_names;

    HashMap<String, gpu::BufferID> buffers;
    Vector<String> buffer_names;
  };

  struct RenderNodeField
  {
    enum class Type : u8
    {
      TEXTURE_2D,
      BUFFER,
      COUNT
    };

    CompStr name;
    Type type;

    [[nodiscard]]
    static constexpr auto Texture2D(CompStr name) -> RenderNodeField
    {
      return {name, Type::TEXTURE_2D};
    }

    [[nodiscard]]
    static constexpr auto Buffer(CompStr name) -> RenderNodeField
    {
      return {name, Type::BUFFER};
    }
  };

  class RenderNode
  {
  public:
    RenderNode() = default;

    RenderNode(const RenderNode&) = delete;

    RenderNode(RenderNode&&) = delete;

    auto operator=(const RenderNode&) -> RenderNode& = delete;

    auto operator=(RenderNode&&) -> RenderNode& = delete;

    virtual ~RenderNode() = default;

    [[nodiscard]]
    virtual auto get_input_fields() const -> Span<const RenderNodeField*> = 0;

    [[nodiscard]]
    virtual auto get_output_fields() const -> Span<const RenderNodeField*> = 0;

    virtual auto submit_pass(
      const Scene& scene,
      const RenderConstant& constant,
      const RenderData& inputs,
      NotNull<gpu::RenderGraph*> render_graph) -> RenderData = 0;

    virtual void on_gui_render(NotNull<app::Gui*> gui) = 0;

    [[nodiscard]]
    virtual auto get_gui_label() const -> CompStr = 0;
  };
} // namespace renderlab
