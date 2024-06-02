#pragma once

#include <vcruntime.h>
#include "gpu/id.h"
#include "gpu/render_graph.h"

namespace soul::app
{

  struct NilGuiTexture
  {
    auto operator==(const NilGuiTexture& other) const -> bool = default;
  };

  constexpr auto nil_gui_texture = NilGuiTexture{};

  struct GuiTextureID
  {
    using InternalID = Variant<soul::gpu::TextureID, soul::gpu::TextureNodeID, NilGuiTexture>;

    GuiTextureID(i32 val) // NOLINT
        : id(InternalID::From(nil_gui_texture))
    {
    }

    GuiTextureID() : id(InternalID::From(nil_gui_texture)) {}

    GuiTextureID(NilGuiTexture val) // NOLINT
        : id(InternalID::From(val))
    {
    }

    explicit GuiTextureID(gpu::TextureID texture_id) : id(InternalID::From(texture_id)) {}

    explicit GuiTextureID(gpu::TextureNodeID texture_node_id)
        : id(InternalID::From(texture_node_id))
    {
    }

    [[nodiscard]]
    auto is_texture_id() const -> b8
    {
      return id.has_value<gpu::TextureID>();
    }

    [[nodiscard]]
    auto get_texture_id() const -> gpu::TextureID
    {
      return id.ref<gpu::TextureID>();
    }

    [[nodiscard]]
    auto get_texture_node_id() const -> gpu::TextureNodeID
    {
      return id.ref<gpu::TextureNodeID>();
    }

    [[nodiscard]] explicit operator intptr_t() const
    {
      if (id.has_value<NilGuiTexture>())
      {
        return 0;
      } else if (is_texture_id())
      {
        return id.ref<gpu::TextureID>().index();
      } else
      {
        return id.ref<gpu::TextureNodeID>().id.id;
      }
    }

    InternalID id;

    auto operator==(const GuiTextureID& other) const -> bool = default;
  };
} // namespace soul::app
