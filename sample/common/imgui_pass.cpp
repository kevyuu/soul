#include "gpu/gpu.h"

#include "gpu/type.h"
#include "imgui_pass.h"

#include "gpu/render_graph.h"
#include "imgui_impl_glfw.h"
#include "runtime/scope_allocator.h"

using namespace soul;

static constexpr const char* IMGUI_HLSL = R"HLSL(

struct Transform {
    float2 scale;
    float2 translate;
};

struct VSInput {
	[[vk::location(0)]] float2 position: POSITION;
	[[vk::location(1)]] float2 tex_coord: TEXCOORD;
	[[vk::location(2)]] uint color: COLOR;
};

[[vk::push_constant]]
struct push_constant {
    soulsl::DescriptorID transform_descriptor_id;
	soulsl::DescriptorID texture_descriptor_id;
	soulsl::DescriptorID sampler_descriptor_id;
} push_constant;

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color: COLOR0;
	float2 tex_coord: TEXCOORD;
};

[shader("vertex")]
VSOutput vsMain(VSInput input)
{
    Transform transform = get_buffer<Transform>(push_constant.transform_descriptor_id, 0);
	VSOutput output;
	output.position = float4((input.position * transform.scale) + transform.translate, 0.0, 1.0);
	output.color = float4((input.color & 0xFF) / 255.0f, ((input.color >> 8) & 0xFF) / 255.0f, ((input.color >> 16) & 0xFF) / 255.0f, ((input.color >> 24) & 0xFF) / 255.0f);
	output.tex_coord = input.tex_coord;
	return output;
}

struct PSOutput
{
	[[vk::location(0)]] float4 color: SV_Target;
};

[shader("pixel")]
PSOutput psMain(VSOutput input)
{
	PSOutput output;
	Texture2D render_texture = get_texture_2d(push_constant.texture_descriptor_id);
	SamplerState render_sampler = get_sampler(push_constant.sampler_descriptor_id);
	output.color = render_texture.Sample(render_sampler, input.tex_coord) * input.color;
	return output;
}

)HLSL";

ImGuiRenderGraphPass::ImGuiRenderGraphPass(soul::gpu::System* gpu_system) : gpu_system_(gpu_system)
{

  gpu::ShaderSource shader_source = gpu::ShaderString(CString::from(IMGUI_HLSL));
  std::filesystem::path search_path = "shaders/";
  constexpr auto entry_points = soul::Array{
    gpu::ShaderEntryPoint{gpu::ShaderStage::VERTEX, "vsMain"},
    gpu::ShaderEntryPoint{gpu::ShaderStage::FRAGMENT, "psMain"},
  };
  const gpu::ProgramDesc program_desc = {
    .search_paths = u32cspan(&search_path, 1),
    .sources = u32cspan(&shader_source, 1),
    .entry_points = entry_points.cspan<u32>(),
  };
  auto result = gpu_system_->create_program(program_desc);
  if (result.is_err()) {
    SOUL_PANIC("Fail to create program");
  }
  program_id_ = result.ok_ref();

  int width, height;
  unsigned char* font_pixels;

  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->GetTexDataAsRGBA32(&font_pixels, &width, &height);

  const gpu::TextureRegionUpdate region = {
    .subresource = {.layer_count = 1},
    .extent = {soul::cast<u32>(width), soul::cast<u32>(height), 1},
  };

  const gpu::TextureLoadDesc load_desc = {
    .data = font_pixels,
    .data_size = soul::cast<usize>(width) * height * 4 * sizeof(char),
    .regions = u32cspan(&region, 1),
  };

  const auto font_tex_desc = gpu::TextureDesc::d2(
    "Font Texture",
    gpu::TextureFormat::RGBA8,
    1,
    {gpu::TextureUsage::SAMPLED},
    {gpu::QueueType::GRAPHIC},
    vec2ui32(width, height));

  font_texture_id_ = gpu_system_->create_texture(font_tex_desc, load_desc);
  gpu_system_->flush_texture(font_texture_id_, {gpu::TextureUsage::SAMPLED});
  font_sampler_id_ = gpu_system->request_sampler(gpu::SamplerDesc::same_filter_wrap(
    gpu::TextureFilter::LINEAR, gpu::TextureWrap::CLAMP_TO_EDGE));

  ImGui::GetIO().Fonts->TexID = &font_texture_id_;
}

ImGuiRenderGraphPass::~ImGuiRenderGraphPass()
{
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiRenderGraphPass::add_pass(
  soul::gpu::TextureNodeID render_target, soul::gpu::RenderGraph& render_graph)
{
  const vec2ui32 viewport = gpu_system_->get_swapchain_extent();
  const auto& draw_data = *ImGui::GetDrawData();

  if (draw_data.TotalVtxCount == 0) {
    return;
  }

  const gpu::ColorAttachmentDesc color_attachment_desc = {
    .node_id = render_target,
    .clear = false,
  };

  SOUL_ASSERT(0, draw_data.TotalVtxCount > 0 && draw_data.TotalIdxCount > 0, "");
  const gpu::BufferNodeID vertex_buffer_node_id = render_graph.create_buffer(
    "ImGui Vertex", {.size = sizeof(ImDrawVert) * draw_data.TotalVtxCount});
  const gpu::BufferNodeID index_buffer_node_id = render_graph.create_buffer(
    "ImGui Index", {.size = sizeof(ImDrawIdx) * draw_data.TotalIdxCount});

  struct Transform {
    float scale[2];
    float translate[2];
  };
  const gpu::BufferNodeID transform_buffer_node_id =
    render_graph.create_buffer("ImGui Transform Buffer", {.size = sizeof(Transform)});

  struct UpdatePassParameter {
    gpu::BufferNodeID vertex_buffer;
    gpu::BufferNodeID index_buffer;
    gpu::BufferNodeID transform_buffer;
  };
  const auto update_pass_parameter =
    render_graph
      .add_non_shader_pass<UpdatePassParameter>(
        "Update Texture Pass",
        gpu::QueueType::TRANSFER,
        [=](auto& parameter, auto& builder) {
          parameter.vertex_buffer =
            builder.add_dst_buffer(vertex_buffer_node_id, gpu::TransferDataSource::CPU);
          parameter.index_buffer =
            builder.add_dst_buffer(index_buffer_node_id, gpu::TransferDataSource::CPU);
          parameter.transform_buffer =
            builder.add_dst_buffer(transform_buffer_node_id, gpu::TransferDataSource::CPU);
        },
        [this, draw_data](const auto& parameter, auto& registry, auto& command_list) {
          runtime::ScopeAllocator scope_allocator("Imgui Update Pass execute");
          using Command = gpu::RenderCommandUpdateBuffer;
          {
            // update vertex_buffer
            Vector<ImDrawVert> im_draw_verts(&scope_allocator);
            im_draw_verts.reserve(draw_data.TotalVtxCount);
            for (auto cmd_list_idx = 0; cmd_list_idx < draw_data.CmdListsCount; cmd_list_idx++) {
              ImDrawList* cmd_list = draw_data.CmdLists[cmd_list_idx];
              std::ranges::copy(cmd_list->VtxBuffer, std::back_inserter(im_draw_verts));
            }
            const gpu::BufferRegionCopy region = {
              .size = im_draw_verts.size() * sizeof(ImDrawVert),
            };
            command_list.push(Command{
              .dst_buffer = registry.get_buffer(parameter.vertex_buffer),
              .data = im_draw_verts.data(),
              .regions = u32cspan(&region, 1),
            });
          }
          {
            // update index_buffer
            Vector<ImDrawIdx> im_draw_indexes(&scope_allocator);
            im_draw_indexes.reserve(draw_data.TotalIdxCount);
            for (auto cmd_list_idx = 0; cmd_list_idx < draw_data.CmdListsCount; cmd_list_idx++) {
              ImDrawList* cmd_list = draw_data.CmdLists[cmd_list_idx];
              std::ranges::copy(cmd_list->IdxBuffer, std::back_inserter(im_draw_indexes));
            }
            const gpu::BufferRegionCopy region = {
              .size = im_draw_indexes.size() * sizeof(ImDrawIdx),
            };
            command_list.push(Command{
              .dst_buffer = registry.get_buffer(parameter.index_buffer),
              .data = im_draw_indexes.data(),
              .regions = u32cspan(&region, 1),
            });
          }

          {
            // update transform buffer
            Transform transform = {
              .scale = {2.0f / draw_data.DisplaySize.x, 2.0f / draw_data.DisplaySize.y},
              .translate =
                {
                  -1.0f - draw_data.DisplayPos.x * (2.0f / draw_data.DisplaySize.x),
                  -1.0f - draw_data.DisplayPos.y * (2.0f / draw_data.DisplaySize.y),
                },
            };
            const gpu::BufferRegionCopy region = {.size = sizeof(Transform)};
            command_list.template push<Command>({
              .dst_buffer = registry.get_buffer(parameter.transform_buffer),
              .data = &transform,
              .regions = u32cspan(&region, 1),
            });
          }
        })
      .get_parameter();

  struct RenderPassParameter {
    gpu::BufferNodeID vertex_buffer;
    gpu::BufferNodeID index_buffer;
    gpu::BufferNodeID transform_buffer;
  };
  render_graph.add_raster_pass<RenderPassParameter>(
    "ImGui Render Pass",
    gpu::RGRenderTargetDesc(viewport, color_attachment_desc),
    [update_pass_parameter](auto& parameter, auto& builder) {
      parameter.vertex_buffer = builder.add_vertex_buffer(update_pass_parameter.vertex_buffer);
      parameter.index_buffer = builder.add_index_buffer(update_pass_parameter.index_buffer);
      parameter.transform_buffer = builder.add_shader_buffer(
        update_pass_parameter.transform_buffer,
        {gpu::ShaderStage::VERTEX},
        gpu::ShaderBufferReadUsage::STORAGE);
    },
    [viewport, &draw_data, this](const auto& parameter, auto& registry, auto& command_list) {
      runtime::ScopeAllocator scope_allocator("Imgui Render Pass Execute Scope Allocator");
      gpu::GraphicPipelineStateDesc pipeline_desc = {
        .program_id = program_id_,
        .input_bindings = {{.stride = sizeof(ImDrawVert)}},
        .input_attributes =
          {
            {.binding = 0,
             .offset = offsetof(ImDrawVert, pos),
             .type = gpu::VertexElementType::FLOAT2},
            {.binding = 0,
             .offset = offsetof(ImDrawVert, uv),
             .type = gpu::VertexElementType::FLOAT2},
            {
              .binding = 0,
              .offset = offsetof(ImDrawVert, col),
              .type = gpu::VertexElementType::UINT,
            },
          },
        .viewport =
          {
            .width = static_cast<float>(viewport.x),
            .height = static_cast<float>(viewport.y),
          },
        .color_attachment_count = 1,
        .color_attachments =
          {
            {
              .blend_enable = true,
              .src_color_blend_factor = gpu::BlendFactor::SRC_ALPHA,
              .dst_color_blend_factor = gpu::BlendFactor::ONE_MINUS_SRC_ALPHA,
              .color_blend_op = gpu::BlendOp::ADD,
              .src_alpha_blend_factor = gpu::BlendFactor::ONE,
              .dst_alpha_blend_factor = gpu::BlendFactor::ZERO,
              .alpha_blend_op = gpu::BlendOp::ADD,
            },
          },
      };

      const ImVec2 clip_offset = draw_data.DisplayPos;
      const ImVec2 clip_scale = draw_data.FramebufferScale;

      auto global_vtx_offset = 0;
      auto global_idx_offset = 0;

      struct PushConstant {
        gpu::DescriptorID transform_descriptor_id;
        gpu::DescriptorID texture_descriptor_id;
        gpu::DescriptorID sampler_descriptor_id;
      };

      Vector<PushConstant> push_constants;
      using Command = gpu::RenderCommandDrawIndex;
      Vector<Command> commands;

      for (auto n = 0; n < draw_data.CmdListsCount; n++) {
        const ImDrawList& cmd_list = *draw_data.CmdLists[n];
        for (auto cmd_i = 0; cmd_i < cmd_list.CmdBuffer.Size; cmd_i++) {
          const ImDrawCmd& cmd = cmd_list.CmdBuffer[cmd_i];
          if (cmd.UserCallback != NULL) {
            SOUL_NOT_IMPLEMENTED();
          } else {
            // Project scissor/clipping rectangles into framebuffer space
            ImVec4 clip_rect;
            clip_rect.x = (cmd.ClipRect.x - clip_offset.x) * clip_scale.x;
            clip_rect.y = (cmd.ClipRect.y - clip_offset.y) * clip_scale.y;
            clip_rect.z = (cmd.ClipRect.z - clip_offset.x) * clip_scale.x;
            clip_rect.w = (cmd.ClipRect.w - clip_offset.y) * clip_scale.y;

            if (
              clip_rect.x < static_cast<float>(viewport.x) &&
              clip_rect.y < static_cast<float>(viewport.y) && clip_rect.z >= 0.0f &&
              clip_rect.w >= 0.0f) {
              if (clip_rect.x < 0.0f) {
                clip_rect.x = 0.0f;
              }
              if (clip_rect.y < 0.0f) {
                clip_rect.y = 0.0f;
              }

              pipeline_desc.scissor = {
                .offset = {static_cast<int32_t>(clip_rect.x), static_cast<int32_t>(clip_rect.y)},
                .extent = {
                  static_cast<uint32_t>(clip_rect.z - clip_rect.x),
                  static_cast<uint32_t>(clip_rect.w - clip_rect.y)}};

              gpu::TextureID texture_id = *static_cast<gpu::TextureID*>(cmd.TextureId);
              const PushConstant push_constant = {
                .transform_descriptor_id = gpu_system_->get_ssbo_descriptor_id(
                  registry.get_buffer(parameter.transform_buffer)),
                .texture_descriptor_id = gpu_system_->get_srv_descriptor_id(texture_id),
                .sampler_descriptor_id = gpu_system_->get_sampler_descriptor_id(font_sampler_id_),
              };
              push_constants.push_back(push_constant);

              const auto first_index = soul::cast<u16>(cmd.IdxOffset + global_idx_offset);

              // ReSharper disable once CppUnreachableCode
              static constexpr gpu::IndexType INDEX_TYPE =
                sizeof(ImDrawIdx) == 2 ? gpu::IndexType::UINT16 : gpu::IndexType::UINT32;

              const Command command = {
                .pipeline_state_id = registry.get_pipeline_state(pipeline_desc),
                .push_constant_data = &push_constants.back(),
                .push_constant_size = sizeof(PushConstant),
                .vertex_buffer_ids = {registry.get_buffer(parameter.vertex_buffer)},
                .vertex_offsets = {soul::cast<u16>(cmd.VtxOffset + global_vtx_offset)},
                .index_buffer_id = registry.get_buffer(parameter.index_buffer),
                .index_type = INDEX_TYPE,
                .first_index = first_index,
                .index_count = soul::cast<u16>(cmd.ElemCount),
              };
              commands.push_back(command);
            }
          }
        }
        global_idx_offset += cmd_list.IdxBuffer.Size;
        global_vtx_offset += cmd_list.VtxBuffer.Size;
      }
      command_list.push(commands.size(), commands.data());
    });
}
