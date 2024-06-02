#include "app/gui.h"
#include "app/imnodes.h"
#include "app/impl/gui_texture_id.h"
#include "app/input_state.h"

#include "core/boolean.h"
#include "core/compiler.h"
#include "core/floating_point.h"
#include "core/log.h"
#include "core/vec.h"
#include "gpu/gpu.h"

#include "core/comp_str.h"
#include "core/sbo_vector.h"
#include "misc/image_data.h"

#include "gpu/render_graph.h"

#include "math/matrix.h"
#include "runtime/scope_allocator.h"

#include "impl/soul_imconfig.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imguizmo/ImGuizmo.h>

#include <iostream>

using namespace soul::literals;

static constexpr soul::CompStr IMGUI_HLSL = R"HLSL(

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
VSOutput vs_main(VSInput input)
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
PSOutput ps_main(VSOutput input)
{
	PSOutput output;
	Texture2D render_texture = get_texture_2d(push_constant.texture_descriptor_id);
	SamplerState render_sampler = get_sampler(push_constant.sampler_descriptor_id);
  vec4f32 final_color = render_texture.Sample(render_sampler, input.tex_coord) * input.color;
	output.color = vec4f32(final_color);
	return output;
}

)HLSL"_str;

namespace soul::app
{
  namespace
  {
    [[nodiscard]]
    auto into_imgui_key(KeyboardKey key) -> ImGuiKey
    {
      switch (key)
      {
      case KeyboardKey::TAB: return ImGuiKey_Tab;
      case KeyboardKey::LEFT: return ImGuiKey_LeftArrow;
      case KeyboardKey::RIGHT: return ImGuiKey_RightArrow;
      case KeyboardKey::UP: return ImGuiKey_UpArrow;
      case KeyboardKey::DOWN: return ImGuiKey_DownArrow;
      case KeyboardKey::PAGE_UP: return ImGuiKey_PageUp;
      case KeyboardKey::PAGE_DOWN: return ImGuiKey_PageDown;
      case KeyboardKey::HOME: return ImGuiKey_Home;
      case KeyboardKey::END: return ImGuiKey_End;
      case KeyboardKey::INSERT: return ImGuiKey_Insert;
      case KeyboardKey::DEL: return ImGuiKey_Delete;
      case KeyboardKey::BACKSPACE: return ImGuiKey_Backspace;
      case KeyboardKey::SPACE: return ImGuiKey_Space;
      case KeyboardKey::ENTER: return ImGuiKey_Enter;
      case KeyboardKey::ESCAPE: return ImGuiKey_Escape;
      case KeyboardKey::APOSTROPHE: return ImGuiKey_Apostrophe;
      case KeyboardKey::COMMA: return ImGuiKey_Comma;
      case KeyboardKey::MINUS: return ImGuiKey_Minus;
      case KeyboardKey::PERIOD: return ImGuiKey_Period;
      case KeyboardKey::SLASH: return ImGuiKey_Slash;
      case KeyboardKey::SEMICOLON: return ImGuiKey_Semicolon;
      case KeyboardKey::EQUAL: return ImGuiKey_Equal;
      case KeyboardKey::LEFT_BRACKET: return ImGuiKey_LeftBracket;
      case KeyboardKey::BACKSLASH: return ImGuiKey_Backslash;
      case KeyboardKey::RIGHT_BRACKET: return ImGuiKey_RightBracket;
      case KeyboardKey::GRAVE_ACCENT: return ImGuiKey_GraveAccent;
      case KeyboardKey::CAPS_LOCK: return ImGuiKey_CapsLock;
      case KeyboardKey::SCROLL_LOCK: return ImGuiKey_ScrollLock;
      case KeyboardKey::NUM_LOCK: return ImGuiKey_NumLock;
      case KeyboardKey::PRINT_SCREEN: return ImGuiKey_PrintScreen;
      case KeyboardKey::PAUSE: return ImGuiKey_Pause;
      case KeyboardKey::KEYPAD_0: return ImGuiKey_Keypad0;
      case KeyboardKey::KEYPAD_1: return ImGuiKey_Keypad1;
      case KeyboardKey::KEYPAD_2: return ImGuiKey_Keypad2;
      case KeyboardKey::KEYPAD_3: return ImGuiKey_Keypad3;
      case KeyboardKey::KEYPAD_4: return ImGuiKey_Keypad4;
      case KeyboardKey::KEYPAD_5: return ImGuiKey_Keypad5;
      case KeyboardKey::KEYPAD_6: return ImGuiKey_Keypad6;
      case KeyboardKey::KEYPAD_7: return ImGuiKey_Keypad7;
      case KeyboardKey::KEYPAD_8: return ImGuiKey_Keypad8;
      case KeyboardKey::KEYPAD_9: return ImGuiKey_Keypad9;
      case KeyboardKey::KEYPAD_DECIMAL: return ImGuiKey_KeypadDecimal;
      case KeyboardKey::KEYPAD_DIVIDE: return ImGuiKey_KeypadDivide;
      case KeyboardKey::KEYPAD_MULTIPLY: return ImGuiKey_KeypadMultiply;
      case KeyboardKey::KEYPAD_SUBTRACT: return ImGuiKey_KeypadSubtract;
      case KeyboardKey::KEYPAD_ADD: return ImGuiKey_KeypadAdd;
      case KeyboardKey::KEYPAD_ENTER: return ImGuiKey_KeypadEnter;
      case KeyboardKey::KEYPAD_EQUAL: return ImGuiKey_KeypadEqual;
      case KeyboardKey::LEFT_SHIFT: return ImGuiKey_LeftShift;
      case KeyboardKey::LEFT_CONTROL: return ImGuiKey_LeftCtrl;
      case KeyboardKey::LEFT_ALT: return ImGuiKey_LeftAlt;
      case KeyboardKey::LEFT_SUPER: return ImGuiKey_LeftSuper;
      case KeyboardKey::RIGHT_SHIFT: return ImGuiKey_RightShift;
      case KeyboardKey::RIGHT_CONTROL: return ImGuiKey_RightCtrl;
      case KeyboardKey::RIGHT_ALT: return ImGuiKey_RightAlt;
      case KeyboardKey::RIGHT_SUPER: return ImGuiKey_RightSuper;
      case KeyboardKey::MENU: return ImGuiKey_Menu;
      case KeyboardKey::KEY_0: return ImGuiKey_0;
      case KeyboardKey::KEY_1: return ImGuiKey_1;
      case KeyboardKey::KEY_2: return ImGuiKey_2;
      case KeyboardKey::KEY_3: return ImGuiKey_3;
      case KeyboardKey::KEY_4: return ImGuiKey_4;
      case KeyboardKey::KEY_5: return ImGuiKey_5;
      case KeyboardKey::KEY_6: return ImGuiKey_6;
      case KeyboardKey::KEY_7: return ImGuiKey_7;
      case KeyboardKey::KEY_8: return ImGuiKey_8;
      case KeyboardKey::KEY_9: return ImGuiKey_9;
      case KeyboardKey::A: return ImGuiKey_A;
      case KeyboardKey::B: return ImGuiKey_B;
      case KeyboardKey::C: return ImGuiKey_C;
      case KeyboardKey::D: return ImGuiKey_D;
      case KeyboardKey::E: return ImGuiKey_E;
      case KeyboardKey::F: return ImGuiKey_F;
      case KeyboardKey::G: return ImGuiKey_G;
      case KeyboardKey::H: return ImGuiKey_H;
      case KeyboardKey::I: return ImGuiKey_I;
      case KeyboardKey::J: return ImGuiKey_J;
      case KeyboardKey::K: return ImGuiKey_K;
      case KeyboardKey::L: return ImGuiKey_L;
      case KeyboardKey::M: return ImGuiKey_M;
      case KeyboardKey::N: return ImGuiKey_N;
      case KeyboardKey::O: return ImGuiKey_O;
      case KeyboardKey::P: return ImGuiKey_P;
      case KeyboardKey::Q: return ImGuiKey_Q;
      case KeyboardKey::R: return ImGuiKey_R;
      case KeyboardKey::S: return ImGuiKey_S;
      case KeyboardKey::T: return ImGuiKey_T;
      case KeyboardKey::U: return ImGuiKey_U;
      case KeyboardKey::V: return ImGuiKey_V;
      case KeyboardKey::W: return ImGuiKey_W;
      case KeyboardKey::X: return ImGuiKey_X;
      case KeyboardKey::Y: return ImGuiKey_Y;
      case KeyboardKey::Z: return ImGuiKey_Z;
      case KeyboardKey::F1: return ImGuiKey_F1;
      case KeyboardKey::F2: return ImGuiKey_F2;
      case KeyboardKey::F3: return ImGuiKey_F3;
      case KeyboardKey::F4: return ImGuiKey_F4;
      case KeyboardKey::F5: return ImGuiKey_F5;
      case KeyboardKey::F6: return ImGuiKey_F6;
      case KeyboardKey::F7: return ImGuiKey_F7;
      case KeyboardKey::F8: return ImGuiKey_F8;
      case KeyboardKey::F9: return ImGuiKey_F9;
      case KeyboardKey::F10: return ImGuiKey_F10;
      case KeyboardKey::F11: return ImGuiKey_F11;
      case KeyboardKey::F12: return ImGuiKey_F12;
      case KeyboardKey::F13: return ImGuiKey_F13;
      case KeyboardKey::F14: return ImGuiKey_F14;
      case KeyboardKey::F15: return ImGuiKey_F15;
      case KeyboardKey::F16: return ImGuiKey_F16;
      case KeyboardKey::F17: return ImGuiKey_F17;
      case KeyboardKey::F18: return ImGuiKey_F18;
      case KeyboardKey::F19: return ImGuiKey_F19;
      case KeyboardKey::F20: return ImGuiKey_F20;
      case KeyboardKey::F21: return ImGuiKey_F21;
      case KeyboardKey::F22: return ImGuiKey_F22;
      case KeyboardKey::F23: return ImGuiKey_F23;
      case KeyboardKey::F24: return ImGuiKey_F24;
      case KeyboardKey::UNKNOWN: return ImGuiKey_None;
      case KeyboardKey::COUNT: unreachable();
      }
    }

    [[nodiscard]]
    auto into_imgui_mouse_button(MouseButton mouse_button) -> ImGuiMouseButton
    {
      switch (mouse_button)
      {
      case MouseButton::LEFT: return ImGuiMouseButton_Left;
      case MouseButton::MIDDLE: return ImGuiMouseButton_Middle;
      case MouseButton::RIGHT: return ImGuiMouseButton_Right;
      case MouseButton::COUNT: unreachable();
      }
    }

    [[nodiscard]]
    auto into_imgui_window_flags(Gui::WindowFlags flags) -> ImGuiWindowFlags
    {
      int imgui_flags = 0;
      if (!flags.test(Gui::WindowFlag::SHOW_TITLE_BAR))
      {
        imgui_flags |= ImGuiWindowFlags_NoTitleBar;
      }
      if (!flags.test(Gui::WindowFlag::ALLOW_MOVE))
      {
        imgui_flags |= ImGuiWindowFlags_NoMove;
      }
      if (!flags.test(Gui::WindowFlag::SET_FOCUS))
      {
        imgui_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
      }
      if (flags.test(Gui::WindowFlag::NO_RESIZE))
      {
        imgui_flags |= ImGuiWindowFlags_NoResize;
      }
      if (flags.test(Gui::WindowFlag::AUTO_RESIZE))
      {
        imgui_flags |= ImGuiWindowFlags_AlwaysAutoResize;
      }
      if (flags.test(Gui::WindowFlag::NO_SCROLLBAR))
      {
        imgui_flags |= ImGuiWindowFlags_NoScrollbar;
      }
      return imgui_flags;
    }

    [[nodiscard]]
    auto into_imgui_tree_node_flags(Gui::TreeNodeFlags tree_node_flags) -> ImGuiTreeNodeFlags
    {
      return tree_node_flags.map<ImGuiTreeNodeFlags>({
        ImGuiTreeNodeFlags_Selected,
        ImGuiTreeNodeFlags_Framed,
        ImGuiTreeNodeFlags_AllowOverlap,
        ImGuiTreeNodeFlags_NoTreePushOnOpen,
        ImGuiTreeNodeFlags_DefaultOpen,
        ImGuiTreeNodeFlags_OpenOnDoubleClick,
        ImGuiTreeNodeFlags_OpenOnArrow,
        ImGuiTreeNodeFlags_Leaf,
        ImGuiTreeNodeFlags_Bullet,
        ImGuiTreeNodeFlags_FramePadding,
        ImGuiTreeNodeFlags_SpanAvailWidth,
        ImGuiTreeNodeFlags_SpanFullWidth,
        ImGuiTreeNodeFlags_SpanAllColumns,
      });
    }

    [[nodiscard]]
    auto into_imgui_size(vec2f32 size) -> ImVec2
    {
      return ImVec2(static_cast<f32>(size.x), static_cast<f32>(size.y));
    }

    [[nodiscard]]
    auto into_imgui_color(vec4f32 color) -> ImU32
    {
      return ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, color.w));
    }

    [[nodiscard]]
    auto into_imgui_direction(Gui::Direction dir) -> ImGuiDir
    {
      switch (dir)
      {
      case Gui::Direction::LEFT: return ImGuiDir_Left;
      case Gui::Direction::RIGHT: return ImGuiDir_Right;
      case Gui::Direction::UP: return ImGuiDir_Up;
      case Gui::Direction::DOWN: return ImGuiDir_Down;
      case Gui::Direction::COUNT: unreachable();
      }
    }

    [[nodiscard]]
    auto into_imguizmo_operation(Gui::GizmoOp op) -> ImGuizmo::OPERATION
    {
      switch (op)
      {
      case Gui::GizmoOp::TRANSLATE: return ImGuizmo::TRANSLATE;
      case Gui::GizmoOp::ROTATE: return ImGuizmo::ROTATE;
      case Gui::GizmoOp::SCALE: return ImGuizmo::SCALE;
      case Gui::GizmoOp::COUNT: unreachable();
      }
    }

    [[nodiscard]]
    auto into_imguizmo_mode(Gui::GizmoMode mode) -> ImGuizmo::MODE
    {
      switch (mode)
      {
      case Gui::GizmoMode::LOCAL: return ImGuizmo::LOCAL;
      case Gui::GizmoMode::WORLD: return ImGuizmo::WORLD;
      case Gui::GizmoMode::COUNT: unreachable();
      }
    }
  } // namespace

  struct Gui::Impl
  {

    NotNull<soul::gpu::System*> gpu_system;
    ImGuiContext* imgui_context;
    soul::gpu::ProgramID program_id      = soul::gpu::ProgramID();
    soul::gpu::TextureID font_texture_id = soul::gpu::TextureID();
    soul::gpu::SamplerID font_sampler_id = soul::gpu::SamplerID();
    f32 scale_factor;
    SBOVector<gpu::TextureNodeID> texture_node_ids;
    InputState input_state;

    soul::HashMap<Path, gpu::TextureID> textures;

    auto load_image(const Path& path) -> gpu::TextureID
    {
      if (textures.contains(path))
      {
        return textures.ref(path);
      }

      auto image_data = soul::ImageData::FromFile(path);

      if (image_data.channel_count() == 3)
      {
        image_data = soul::ImageData::FromFile(path, 4);
      }

      const gpu::TextureFormat format = [&]
      {
        if (image_data.channel_count() == 1)
        {
          return gpu::TextureFormat::R8;
        } else
        {
          SOUL_ASSERT(0, image_data.channel_count() == 4);
          return gpu::TextureFormat::SRGBA8;
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
      const auto texture_id = gpu_system->create_texture(""_str, texture_desc, load_desc);
      gpu_system->flush_texture(texture_id, usage);

      textures.insert(path.clone(), texture_id);

      return texture_id;
    }

    void set_style(f32 scale_factor)
    {
      ImGuiStyle& style = ImGui::GetStyle();
      ImVec4* colors    = style.Colors;

      colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
      colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
      colors[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
      colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
      colors[ImGuiCol_PopupBg]               = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
      colors[ImGuiCol_Border]                = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
      colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
      colors[ImGuiCol_FrameBg]               = ImVec4(0.25f, 0.25f, 0.25f, 0.54f);
      colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
      colors[ImGuiCol_FrameBgActive]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
      colors[ImGuiCol_TitleBg]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_TitleBgActive]         = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
      colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
      colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
      colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
      colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
      colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
      colors[ImGuiCol_CheckMark]             = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
      colors[ImGuiCol_SliderGrab]            = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
      colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
      colors[ImGuiCol_Button]                = ImVec4(0.30f, 0.30f, 0.30f, 0.54f);
      colors[ImGuiCol_ButtonHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
      colors[ImGuiCol_ButtonActive]          = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
      colors[ImGuiCol_Header]                = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
      colors[ImGuiCol_HeaderHovered]         = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
      colors[ImGuiCol_HeaderActive]          = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
      colors[ImGuiCol_Separator]             = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
      colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
      colors[ImGuiCol_SeparatorActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
      colors[ImGuiCol_ResizeGrip]            = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
      colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
      colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
      colors[ImGuiCol_Tab]                   = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
      colors[ImGuiCol_TabHovered]            = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
      colors[ImGuiCol_TabActive]             = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
      colors[ImGuiCol_TabUnfocused]          = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
      colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
      colors[ImGuiCol_DockingPreview]        = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
      colors[ImGuiCol_DockingEmptyBg]        = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_PlotHistogram]         = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
      colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
      colors[ImGuiCol_TableBorderLight]      = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
      colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
      colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
      colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
      colors[ImGuiCol_DragDropTarget]        = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
      colors[ImGuiCol_NavHighlight]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
      colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
      colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

      for (i32 i = 0; i < ImGuiCol_COUNT; i++)
      {
        ImVec4& col = style.Colors[i];

        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, h, s, v);
        h = 0.163f;
        ImGui::ColorConvertHSVtoRGB(h, s, v, col.x, col.y, col.z);
      }

      style.PopupRounding = 3;

      style.WindowPadding    = ImVec2(8, 8);
      style.FramePadding     = ImVec2(8, 8);
      style.ItemSpacing      = ImVec2(8, 8);
      style.ItemInnerSpacing = ImVec2(6, 4);

      style.ScrollbarSize = 18;

      style.WindowBorderSize = 1;
      style.ChildBorderSize  = 1;
      style.PopupBorderSize  = 1;
      style.FrameBorderSize  = 0.0f;

      style.WindowRounding    = 0;
      style.ChildRounding     = 0;
      style.FrameRounding     = 0;
      style.ScrollbarRounding = 4;
      style.GrabRounding      = 4;

      style.TabBorderSize = 0.0f;
      style.TabRounding   = 6;

      colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
      colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

      if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
      {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
      }

      style.ScaleAllSizes(scale_factor);
    }
  };

  Gui::Gui(
    NotNull<gpu::System*> gpu_system, f32 scale_factor, NotNull<memory::Allocator*> allocator)
      : allocator_(allocator)
  {
    ImGuiContext* imgui_context = ImGui::CreateContext();

    ImGui::SetCurrentContext(imgui_context);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;

    const auto shader_source = gpu::ShaderSource::From(gpu::ShaderString(String::From(IMGUI_HLSL)));
    const auto search_path   = Path::From("shaders/"_str);
    constexpr auto entry_points = Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::VERTEX, "vs_main"_str},
      gpu::ShaderEntryPoint{gpu::ShaderStage::FRAGMENT, "ps_main"_str},
    };
    const gpu::ProgramDesc program_desc = {
      .search_paths = u32cspan(&search_path, 1),
      .sources      = u32cspan(&shader_source, 1),
      .entry_points = entry_points.cspan<u32>(),
    };
    auto result = gpu_system->create_program(program_desc);
    if (result.is_err())
    {
      SOUL_PANIC("Fail to create program");
    }
    const auto program_id = result.ok_ref();

    i32 width, height;          // NOLINT
    unsigned char* font_pixels; // NOLINT
    io.Fonts->GetTexDataAsRGBA32(&font_pixels, &width, &height);

    const gpu::TextureRegionUpdate region = {
      .subresource = {.layer_count = 1},
      .extent      = {cast<u32>(width), cast<u32>(height), 1},
    };

    const gpu::TextureLoadDesc load_desc = {
      .data      = font_pixels,
      .data_size = cast<usize>(width) * height * 4 * sizeof(char),
      .regions   = u32cspan(&region, 1),
    };

    const auto font_tex_desc = gpu::TextureDesc::d2(
      gpu::TextureFormat::RGBA8,
      1,
      {gpu::TextureUsage::SAMPLED},
      {gpu::QueueType::GRAPHIC},
      vec2u32(width, height));

    auto font_texture_id = gpu_system->create_texture("Font Texture"_str, font_tex_desc, load_desc);
    gpu_system->flush_texture(font_texture_id, {gpu::TextureUsage::SAMPLED});
    const auto font_sampler_id = gpu_system->request_sampler(gpu::SamplerDesc::same_filter_wrap(
      gpu::TextureFilter::LINEAR, gpu::TextureWrap::CLAMP_TO_EDGE));
    io.Fonts->TexID            = GuiTextureID(font_texture_id);

    io.FontGlobalScale = 1.0f;

    impl_ = allocator_
              ->generate(
                [=]()
                {
                  return Impl{
                    .gpu_system       = gpu_system,
                    .imgui_context    = imgui_context,
                    .program_id       = program_id,
                    .font_texture_id  = font_texture_id,
                    .font_sampler_id  = font_sampler_id,
                    .scale_factor     = scale_factor,
                    .texture_node_ids = SBOVector<gpu::TextureNodeID>(),
                    .textures         = HashMap<Path, gpu::TextureID>(),
                  };
                })
              .unwrap();

    impl_->set_style(scale_factor);

    ImGuizmo::SetImGuiContext(impl_->imgui_context);
  }

  Gui::Gui(Gui&& other) noexcept
      : impl_(std::exchange(other.impl_, nullptr)), allocator_(other.allocator_)
  {
  }

  auto Gui::operator=(Gui&& other) noexcept -> Gui&
  {
    cleanup();
    using std::swap;
    swap(other.impl_, impl_);
    swap(other.allocator_, allocator_);
    return *this;
  }

  Gui::~Gui()
  {
    cleanup();
  }

  void Gui::cleanup()
  {
    if (impl_ != nullptr)
    {
      ImGui::DestroyContext(impl_->imgui_context);
      impl_->gpu_system->destroy_texture(impl_->font_texture_id);
      impl_->gpu_system->destroy_program(impl_->program_id);
      allocator_->destroy<Impl>(impl_);
      impl_ = nullptr;
    }
  }

  void Gui::begin_frame()
  {
    ImGui::SetCurrentContext(impl_->imgui_context);
    ImGui::NewFrame();
    ImGuizmo::SetImGuiContext(impl_->imgui_context);
    ImGuizmo::BeginFrame();
    impl_->texture_node_ids.clear();
  }

  void Gui::render_frame(
    NotNull<gpu::RenderGraph*> render_graph,
    gpu::TextureNodeID render_target,
    double elapsed_second)
  {
    ImGui::SetCurrentContext(impl_->imgui_context);
    ImGuiIO& io  = ImGui::GetIO();
    io.DeltaTime = f32(elapsed_second) > 0.0f ? f32(elapsed_second) : 1.0f / 60.0f;
    // while (group_stack_size) {
    //   mpWrapper->endGroup();
    // }

    // Set the mouse state
    ImGui::Render();

    const vec2u32 viewport = impl_->gpu_system->get_swapchain_extent();
    const auto& draw_data  = *ImGui::GetDrawData();

    if (draw_data.TotalVtxCount == 0)
    {
      return;
    }

    const gpu::RGColorAttachmentDesc color_attachment_desc = {
      .node_id = render_target,
      .clear   = true,
    };

    SOUL_ASSERT(0, draw_data.TotalVtxCount > 0 && draw_data.TotalIdxCount > 0);
    const gpu::BufferNodeID vertex_buffer_node_id = render_graph->create_buffer(
      "ImGui Vertex"_str, {.size = sizeof(ImDrawVert) * draw_data.TotalVtxCount});
    const gpu::BufferNodeID index_buffer_node_id = render_graph->create_buffer(
      "ImGui Index"_str, {.size = sizeof(ImDrawIdx) * draw_data.TotalIdxCount});

    struct Transform
    {
      float scale[2];
      float translate[2];
    };

    const gpu::BufferNodeID transform_buffer_node_id =
      render_graph->create_buffer("ImGui Transform Buffer"_str, {.size = sizeof(Transform)});

    struct UpdatePassParameter
    {
      gpu::BufferNodeID vertex_buffer;
      gpu::BufferNodeID index_buffer;
      gpu::BufferNodeID transform_buffer;
    };

    const auto update_pass_parameter =
      render_graph
        ->add_non_shader_pass<UpdatePassParameter>(
          "Update Texture Pass"_str,
          gpu::QueueType::TRANSFER,
          [=](auto& parameter, auto& builder)
          {
            parameter.vertex_buffer =
              builder.add_dst_buffer(vertex_buffer_node_id, gpu::TransferDataSource::CPU);
            parameter.index_buffer =
              builder.add_dst_buffer(index_buffer_node_id, gpu::TransferDataSource::CPU);
            parameter.transform_buffer =
              builder.add_dst_buffer(transform_buffer_node_id, gpu::TransferDataSource::CPU);
          },
          [this, &draw_data](const auto& parameter, auto& registry, auto& command_list)
          {
            runtime::ScopeAllocator scope_allocator("Imgui Update Pass execute"_str);
            using Command = gpu::RenderCommandUpdateBuffer;
            {
              // update vertex_buffer
              Vector<ImDrawVert> im_draw_verts(&scope_allocator);
              im_draw_verts.reserve(draw_data.TotalVtxCount);
              for (auto cmd_list_idx = 0; cmd_list_idx < draw_data.CmdListsCount; cmd_list_idx++)
              {
                ImDrawList* cmd_list = draw_data.CmdLists[cmd_list_idx];
                std::ranges::copy(cmd_list->VtxBuffer, std::back_inserter(im_draw_verts));
              }
              const gpu::BufferRegionCopy region = {
                .size = im_draw_verts.size() * sizeof(ImDrawVert),
              };
              command_list.push(Command{
                .dst_buffer = registry.get_buffer(parameter.vertex_buffer),
                .data       = im_draw_verts.data(),
                .regions    = u32cspan(&region, 1),
              });
            }
            {
              // update index_buffer
              Vector<ImDrawIdx> im_draw_indexes(&scope_allocator);
              im_draw_indexes.reserve(draw_data.TotalIdxCount);
              for (auto cmd_list_idx = 0; cmd_list_idx < draw_data.CmdListsCount; cmd_list_idx++)
              {
                ImDrawList* cmd_list = draw_data.CmdLists[cmd_list_idx];
                std::ranges::copy(cmd_list->IdxBuffer, std::back_inserter(im_draw_indexes));
              }
              const gpu::BufferRegionCopy region = {
                .size = im_draw_indexes.size() * sizeof(ImDrawIdx),
              };
              command_list.push(Command{
                .dst_buffer = registry.get_buffer(parameter.index_buffer),
                .data       = im_draw_indexes.data(),
                .regions    = u32cspan(&region, 1),
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
              constexpr gpu::BufferRegionCopy region = {.size = sizeof(Transform)};
              command_list.template push<Command>({
                .dst_buffer = registry.get_buffer(parameter.transform_buffer),
                .data       = &transform,
                .regions    = u32cspan(&region, 1),
              });
            }
          })
        .get_parameter();

    struct RenderPassParameter
    {
      gpu::BufferNodeID vertex_buffer;
      gpu::BufferNodeID index_buffer;
      gpu::BufferNodeID transform_buffer;
    };

    render_graph->add_raster_pass<RenderPassParameter>(
      "ImGui Render Pass"_str,
      gpu::RGRenderTargetDesc(viewport, color_attachment_desc),
      [this, update_pass_parameter](auto& parameter, auto& builder)
      {
        parameter.vertex_buffer    = builder.add_vertex_buffer(update_pass_parameter.vertex_buffer);
        parameter.index_buffer     = builder.add_index_buffer(update_pass_parameter.index_buffer);
        parameter.transform_buffer = builder.add_shader_buffer(
          update_pass_parameter.transform_buffer,
          {gpu::ShaderStage::VERTEX},
          gpu::ShaderBufferReadUsage::STORAGE);
        for (const auto node_id : impl_->texture_node_ids)
        {
          builder.add_shader_texture(
            node_id,
            {gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT},
            gpu::ShaderTextureReadUsage::UNIFORM);
        }
      },
      [viewport, &draw_data, this](const auto& parameter, auto& registry, auto& command_list)
      {
        runtime::ScopeAllocator scope_allocator("Imgui Render Pass Execute Scope Allocator"_str);
        gpu::GraphicPipelineStateDesc pipeline_desc = {
          .program_id = impl_->program_id,
          .input_bindings =
            {
              .list =
                {
                  {.stride = sizeof(ImDrawVert)},
                },
            },
          .input_attributes =
            {
              .list =
                {
                  {
                    .binding = 0,
                    .offset  = offsetof(ImDrawVert, pos),
                    .type    = gpu::VertexElementType::FLOAT2,
                  },
                  {
                    .binding = 0,
                    .offset  = offsetof(ImDrawVert, uv),
                    .type    = gpu::VertexElementType::FLOAT2,
                  },
                  {
                    .binding = 0,
                    .offset  = offsetof(ImDrawVert, col),
                    .type    = gpu::VertexElementType::UINT,
                  },
                },
            },
          .viewport =
            {
              .width  = static_cast<float>(viewport.x),
              .height = static_cast<float>(viewport.y),
            },
          .color_attachment_count = 1,
          .color_attachments =
            {
              .list = {{
                .blend_enable           = true,
                .src_color_blend_factor = gpu::BlendFactor::SRC_ALPHA,
                .dst_color_blend_factor = gpu::BlendFactor::ONE_MINUS_SRC_ALPHA,
                .color_blend_op         = gpu::BlendOp::ADD,
                .src_alpha_blend_factor = gpu::BlendFactor::ONE,
                .dst_alpha_blend_factor = gpu::BlendFactor::ZERO,
                .alpha_blend_op         = gpu::BlendOp::ADD,
              }},
            },
        };
        const auto transform_descriptor_id = impl_->gpu_system->get_ssbo_descriptor_id(
          registry.get_buffer(parameter.transform_buffer));
        const auto vertex_buffer_id = registry.get_buffer(parameter.vertex_buffer);
        const auto index_buffer_id  = registry.get_buffer(parameter.index_buffer);
        const auto sampler_descriptor_id =
          impl_->gpu_system->get_sampler_descriptor_id(impl_->font_sampler_id);

        const ImVec2 clip_offset = draw_data.DisplayPos;
        const ImVec2 clip_scale  = draw_data.FramebufferScale;

        auto global_vtx_offset = 0;
        auto global_idx_offset = 0;

        struct PushConstant
        {
          gpu::DescriptorID transform_descriptor_id;
          gpu::DescriptorID texture_descriptor_id;
          gpu::DescriptorID sampler_descriptor_id;
        };

        auto command_count = 0;
        for (auto n = 0; n < draw_data.CmdListsCount; n++)
        {
          const ImDrawList& cmd_list = *draw_data.CmdLists[n];
          command_count += cmd_list.CmdBuffer.Size;
        }

        Vector<PushConstant> push_constants;
        push_constants.reserve(command_count);
        using Command = gpu::RenderCommandDrawIndex;
        Vector<Command> commands;
        commands.reserve(command_count);

        for (auto n = 0; n < draw_data.CmdListsCount; n++)
        {
          const ImDrawList& cmd_list = *draw_data.CmdLists[n];
          for (auto cmd_i = 0; cmd_i < cmd_list.CmdBuffer.Size; cmd_i++)
          {
            const ImDrawCmd& cmd       = cmd_list.CmdBuffer[cmd_i];
            const b8 has_user_callback = cmd.UserCallback != nullptr;
            if (has_user_callback)
            {
              SOUL_NOT_IMPLEMENTED();
            } else
            {

              // Project scissor/clipping rectangles into framebuffer space
              ImVec4 clip_rect;
              clip_rect.x = (cmd.ClipRect.x - clip_offset.x) * clip_scale.x;
              clip_rect.y = (cmd.ClipRect.y - clip_offset.y) * clip_scale.y;
              clip_rect.z = (cmd.ClipRect.z - clip_offset.x) * clip_scale.x;
              clip_rect.w = (cmd.ClipRect.w - clip_offset.y) * clip_scale.y;

              if (
                clip_rect.x < static_cast<float>(viewport.x) &&
                clip_rect.y < static_cast<float>(viewport.y) && clip_rect.z >= 0.0f &&
                clip_rect.w >= 0.0f)
              {
                if (clip_rect.x < 0.0f)
                {
                  clip_rect.x = 0.0f;
                }
                if (clip_rect.y < 0.0f)
                {
                  clip_rect.y = 0.0f;
                }

                pipeline_desc.scissor = {
                  .offset = {static_cast<int32_t>(clip_rect.x), static_cast<int32_t>(clip_rect.y)},
                  .extent =
                    {
                      static_cast<uint32_t>(clip_rect.z - clip_rect.x),
                      static_cast<uint32_t>(clip_rect.w - clip_rect.y),
                    },
                };

                const GuiTextureID gui_texture_id = cmd.TextureId;
                // SOUL_ASSERT(0, gui_texture_id.is_texture_id());
                const auto texture_id             = [&]()
                {
                  if (gui_texture_id.is_texture_id())
                  {
                    return gui_texture_id.get_texture_id();
                  } else
                  {
                    return registry.get_texture(gui_texture_id.get_texture_node_id());
                  }
                }();
                const PushConstant push_constant = {
                  .transform_descriptor_id = transform_descriptor_id,
                  .texture_descriptor_id   = impl_->gpu_system->get_srv_descriptor_id(texture_id),
                  .sampler_descriptor_id   = sampler_descriptor_id,
                };
                push_constants.push_back(push_constant);

                const auto first_index = cast<u16>(cmd.IdxOffset + global_idx_offset);

                static constexpr gpu::IndexType INDEX_TYPE =
                  sizeof(ImDrawIdx) == 2 ? gpu::IndexType::UINT16 : gpu::IndexType::UINT32;

                const Command command = {
                  .pipeline_state_id  = registry.get_pipeline_state(pipeline_desc),
                  .push_constant_data = &push_constants.back(),
                  .push_constant_size = sizeof(PushConstant),
                  .vertex_buffer_ids  = {vertex_buffer_id},
                  .vertex_offsets     = {cast<u16>(cmd.VtxOffset + global_vtx_offset)},
                  .index_buffer_id    = index_buffer_id,
                  .index_type         = INDEX_TYPE,
                  .first_index        = first_index,
                  .index_count        = cast<u16>(cmd.ElemCount),
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

  void Gui::on_window_resize(u32 width, u32 height)
  {
    ImGui::SetCurrentContext(impl_->imgui_context);
    ImGuiIO& io      = ImGui::GetIO();
    io.DisplaySize.x = static_cast<f32>(width);
    io.DisplaySize.y = static_cast<f32>(height);
  }

  auto Gui::on_mouse_event(const MouseEvent& mouse_event) -> b8
  {
    ImGui::SetCurrentContext(impl_->imgui_context);
    ImGuiIO& io = ImGui::GetIO();
    switch (mouse_event.type)
    {
    case MouseEvent::Type::BUTTON_DOWN:
    case MouseEvent::Type::BUTTON_UP:
    {
      const b8 is_down        = mouse_event.type == MouseEvent::Type::BUTTON_DOWN;
      const auto mouse_button = into_imgui_mouse_button(mouse_event.button);
      io.AddMouseButtonEvent(mouse_button, is_down);
    }
    break;
    case MouseEvent::Type::MOVE:
    {
      const float x = mouse_event.pos.x * io.DisplaySize.x;
      const float y = mouse_event.pos.y * io.DisplaySize.y;
      io.AddMousePosEvent(x, y);
    }
    break;
    case MouseEvent::Type::WHEEL:
      io.AddMouseWheelEvent(mouse_event.wheel_delta.x, mouse_event.wheel_delta.y);
      break;
    default: unreachable();
    }

    return io.WantCaptureMouse;
  }

  auto Gui::on_keyboard_event(const KeyboardEvent& keyboard_event) -> b8
  {
    ImGui::SetCurrentContext(impl_->imgui_context);
    ImGuiIO& io = ImGui::GetIO();

    if (keyboard_event.type == KeyboardEvent::Type::INPUT)
    {
      io.AddInputCharacter(keyboard_event.codepoint);

      // Gui consumes keyboard input
      return true;
    } else
    {

      io.AddKeyEvent(ImGuiMod_Ctrl, keyboard_event.mods.test(InputModifier::CTRL));
      io.AddKeyEvent(ImGuiMod_Shift, keyboard_event.mods.test(InputModifier::SHIFT));
      io.AddKeyEvent(ImGuiMod_Alt, keyboard_event.mods.test(InputModifier::ALT));
      io.AddKeyEvent(ImGuiMod_Super, false);

      const auto imgui_key = into_imgui_key(keyboard_event.key);
      switch (keyboard_event.type)
      {
      case KeyboardEvent::Type::KEY_REPEATED:
      case KeyboardEvent::Type::KEY_PRESSED: io.AddKeyEvent(imgui_key, true); break;
      case KeyboardEvent::Type::KEY_RELEASED: io.AddKeyEvent(imgui_key, false); break;
      default: unreachable();
      }

      return io.WantCaptureKeyboard;
    }
  }

  void Gui::on_window_focus_event(b8 focused)
  {
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(focused);
  }

  auto Gui::begin_main_menu_bar() -> b8
  {
    return ImGui::BeginMainMenuBar();
  }

  void Gui::end_main_menu_bar()
  {
    return ImGui::EndMainMenuBar();
  }

  auto Gui::begin_menu(CompStr label) -> b8
  {
    return ImGui::BeginMenu(label.c_str());
  }

  void Gui::end_menu()
  {
    ImGui::EndMenu();
  }

  auto Gui::menu_item(CompStr label) -> b8
  {
    return ImGui::MenuItem(label.c_str());
  }

  void Gui::begin_dock_window()
  {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport       = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
  }

  auto Gui::begin_window(CompStr label, vec2f32 size, vec2f32 pos, WindowFlags flags) -> b8
  {
    const auto imgui_pos =
      ImVec2(f32(pos.x) * impl_->scale_factor, f32(pos.y) * impl_->scale_factor);
    const auto imgui_size =
      ImVec2(f32(size.x) * impl_->scale_factor, f32(size.y) * impl_->scale_factor);
    ImGui::SetNextWindowSize(imgui_size, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(imgui_pos, ImGuiCond_FirstUseEver);
    const auto imgui_flags = into_imgui_window_flags(flags);
    const b8 open          = ImGui::Begin(label.c_str(), nullptr, imgui_flags);
    if (open)
    {
      ImGui::PushItemWidth(-230.0f);
    }
    return open;
  }

  void Gui::end_window()
  {
    ImGui::End();
  }

  auto Gui::get_window_pos() const -> vec2f32
  {
    const auto imgui_pos = ImGui::GetWindowPos();
    return vec2f32(imgui_pos.x, imgui_pos.y);
  }

  auto Gui::get_window_size() const -> vec2f32
  {
    const auto imgui_size = ImGui::GetWindowSize();
    return vec2f32(imgui_size.x, imgui_size.y);
  }

  auto Gui::begin_popup(CompStr label) -> b8
  {
    return ImGui::BeginPopup(label.c_str());
  }

  auto Gui::begin_popup_modal(CompStr label) -> b8
  {
    return ImGui::BeginPopupModal(label.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  }

  void Gui::end_popup()
  {
    ImGui::EndPopup();
  }

  void Gui::open_popup(CompStr label)
  {
    ImGui::OpenPopup(label.c_str());
  }

  void Gui::close_current_popup()
  {
    ImGui::CloseCurrentPopup();
  }

  void Gui::set_item_default_focus()
  {
    ImGui::SetItemDefaultFocus();
  }

  auto Gui::get_id(CompStr label) -> GuiID
  {
    return GuiID(ImGui::GetID(label.c_str()));
  }

  // ----------------------------------------------------------------------------
  // Dock
  // ----------------------------------------------------------------------------
  auto Gui::dock_space(GuiID gui_id) -> GuiID
  {
    return GuiID(ImGui::DockSpace(gui_id.id));
  }

  auto Gui::dock_builder_is_node_exist(GuiID dock_id) -> b8
  {
    return ImGui::DockBuilderGetNode(dock_id.id) != nullptr;
  }

  auto Gui::dock_builder_split_dock(GuiID parent_dock_id, Direction split_dir, f32 ratio)
    -> Tuple<GuiID, GuiID>
  {
    u32 opposite_dir_dock_id = 0;
    const auto dock_id       = ImGui::DockBuilderSplitNode(
      parent_dock_id.id, into_imgui_direction(split_dir), ratio, nullptr, &opposite_dir_dock_id);
    return {GuiID(dock_id), GuiID(opposite_dir_dock_id)};
  }

  auto Gui::dock_builder_init(GuiID dock_id) -> b8
  {
    if (ImGui::DockBuilderGetNode(dock_id.id) != nullptr)
    {
      return false;
    }
    ImGui::DockBuilderAddNode(dock_id.id);
    ImGui::DockBuilderSetNodePos(dock_id.id, ImVec2(0, 0));
    ImGui::DockBuilderSetNodeSize(dock_id.id, ImGui::GetMainViewport()->WorkSize);
    return true;
  }

  void Gui::dock_builder_dock_window(CompStr label, GuiID dock_id)
  {
    ImGui::DockBuilderDockWindow(label.c_str(), dock_id.id);
  }

  void Gui::dock_builder_finish(GuiID dock_id)
  {
    ImGui::DockBuilderFinish(dock_id.id);
  }

  // ----------------------------------------------------------------------------
  // Widgets: Text
  // ----------------------------------------------------------------------------
  void Gui::text(StringView text)
  {
    SOUL_ASSERT(0, text.is_null_terminated());
    ImGui::Text(text.data().unwrap().get());
  }

  void Gui::label_text(CompStr label, StringView text)
  {
    SOUL_ASSERT(0, text.is_null_terminated());
    ImGui::LabelText(label.c_str(), "%s", text.data().unwrap().get()); // NOLINT
  }

  void Gui::separator_text(CompStr label)
  {
    ImGui::SeparatorText(label.c_str());
  }

  // ----------------------------------------------------------------------------
  // Widgets: Main
  // ----------------------------------------------------------------------------
  auto Gui::button(CompStr label, vec2f32 size) -> b8
  {
    return ImGui::Button(label.c_str(), into_imgui_size(size));
  }

  auto Gui::image_button(
    CompStr label,
    gpu::TextureID texture_id,
    vec4f32 tint_normal,
    vec4f32 tint_hovered,
    vec4f32 tint_pressed,
    vec2f32 size) -> b8
  {
    const b8 pressed    = ImGui::InvisibleButton(label.c_str(), into_imgui_size(size));
    auto* drawList      = ImGui::GetWindowDrawList();
    const auto rect_min = ImGui::GetItemRectMin();
    const auto rect_max = ImGui::GetItemRectMax();
    if (ImGui::IsItemActive())
    {
      drawList->AddImage(
        GuiTextureID(texture_id),
        rect_min,
        rect_max,
        ImVec2(0, 0),
        ImVec2(1, 1),
        into_imgui_color(tint_pressed));
    } else if (ImGui::IsItemHovered())
    {
      drawList->AddImage(
        GuiTextureID(texture_id),
        rect_min,
        rect_max,
        ImVec2(0, 0),
        ImVec2(1, 1),
        into_imgui_color(tint_hovered));
    } else
    {
      drawList->AddImage(
        GuiTextureID(texture_id),
        rect_min,
        rect_max,
        ImVec2(0, 0),
        ImVec2(1, 1),
        into_imgui_color(tint_normal));
    }
    return pressed;
  }

  auto Gui::image_button(
    CompStr label,
    const Path& path,
    vec4f32 tint_normal,
    vec4f32 tint_hovered,
    vec4f32 tint_pressed,
    vec2f32 size) -> b8
  {
    const b8 pressed      = ImGui::InvisibleButton(label.c_str(), into_imgui_size(size));
    auto* draw_list       = ImGui::GetWindowDrawList();
    const auto rect_min   = ImGui::GetItemRectMin();
    const auto rect_max   = ImGui::GetItemRectMax();
    const auto texture_id = impl_->load_image(path);

    if (ImGui::IsItemActive())
    {
      draw_list->AddImage(
        GuiTextureID(texture_id),
        rect_min,
        rect_max,
        ImVec2(0, 0),
        ImVec2(1, 1),
        into_imgui_color(tint_pressed));
    } else if (ImGui::IsItemHovered())
    {
      draw_list->AddImage(
        GuiTextureID(texture_id),
        rect_min,
        rect_max,
        ImVec2(0, 0),
        ImVec2(1, 1),
        into_imgui_color(tint_hovered));
    } else
    {
      draw_list->AddImage(
        GuiTextureID(texture_id),
        rect_min,
        rect_max,
        ImVec2(0, 0),
        ImVec2(1, 1),
        into_imgui_color(tint_normal));
    }
    return pressed;
  }

  auto Gui::checkbox(CompStr label, NotNull<b8*> value) -> b8
  {
    return ImGui::Checkbox(label.c_str(), value.get());
  }

  auto Gui::radio_button(CompStr label, NotNull<i32*> val, i32 button_val) -> b8
  {
    return ImGui::RadioButton(label.c_str(), val, button_val);
  }

  // ----------------------------------------------------------------------------
  // Widgets: Input
  // ----------------------------------------------------------------------------
  auto Gui::input_text(CompStr label, String& text, usize text_length_limit) -> b8
  {
    runtime::ScopeAllocator scope_allocator("Input Text"_str);
    char* buffer = scope_allocator.allocate_array<char>(text_length_limit);
    std::memcpy(buffer, text.c_str(), text.size() + 1);
    const b8 is_change = ImGui::InputText(label.c_str(), buffer, text_length_limit);
    text.assign(buffer);
    return is_change;
  }

  auto Gui::input_text(CompStr label, Span<char*> buffer) -> b8
  {
    return ImGui::InputText(label.c_str(), buffer.data(), buffer.size());
  }

  auto Gui::input_i32(CompStr label, NotNull<i32*> value) -> b8
  {
    return ImGui::InputInt(label.c_str(), value.get());
  }

  auto Gui::input_f32(CompStr label, NotNull<f32*> value) -> b8
  {
    return ImGui::InputFloat(label.c_str(), value.get());
  }

  auto Gui::input_vec3f32(CompStr label, NotNull<vec3f32*> value) -> b8
  {
    return ImGui::InputFloat3(label.c_str(), value->data);
  }

  auto Gui::input_vec3i32(CompStr label, NotNull<vec3i32*> value) -> b8
  {
    return ImGui::InputInt3(label.c_str(), value->data);
  }

  // ----------------------------------------------------------------------------
  // Widgets: Combo
  // ----------------------------------------------------------------------------

  auto Gui::begin_combo(CompStr label, StringView preview) -> b8
  {
    SOUL_ASSERT(0, preview.is_null_terminated());
    return ImGui::BeginCombo(label.c_str(), preview.data());
  }

  void Gui::end_combo()
  {
    ImGui::EndCombo();
  }

  // ----------------------------------------------------------------------------
  // Widgets: Slider
  // ----------------------------------------------------------------------------
  namespace
  {
    [[nodiscard]]
    auto into_imgui_slider_flags(Gui::SliderFlags flags) -> ImGuiSliderFlags
    {
      return flags.map<ImGuiSliderFlags>({
        ImGuiSliderFlags_AlwaysClamp,
        ImGuiSliderFlags_Logarithmic,
        ImGuiSliderFlags_NoRoundToFormat,
        ImGuiSliderFlags_NoInput,
      });
    }
  } // namespace

  auto Gui::slider_i32(CompStr label, NotNull<i32*> val, i32 min, i32 max, SliderFlags flags) -> b8
  {
    return ImGui::SliderInt(
      label.c_str(), val.get(), min, max, "%d", into_imgui_slider_flags(flags));
  }

  auto Gui::slider_f32(CompStr label, NotNull<f32*> val, f32 v_min, f32 v_max, SliderFlags flags)
    -> b8
  {
    return ImGui::SliderFloat(
      label.c_str(), val.get(), v_min, v_max, "%.3f", into_imgui_slider_flags(flags));
  }

  auto Gui::slider_vec2f32(
    CompStr label, NotNull<vec2f32*> val, f32 v_min, f32 v_max, SliderFlags flags) -> b8
  {
    return ImGui::SliderFloat2(
      label.c_str(), val->data, v_min, v_max, "%.3f", into_imgui_slider_flags(flags));
  }

  auto Gui::slider_vec3f32(
    CompStr label, NotNull<vec3f32*> val, f32 v_min, f32 v_max, SliderFlags flags) -> b8
  {
    return ImGui::SliderFloat3(
      label.c_str(), val->data, v_min, v_max, "%.3f", into_imgui_slider_flags(flags));
  }

  auto Gui::slider_vec4f32(
    CompStr label, NotNull<vec4f32*> val, f32 v_min, f32 v_max, SliderFlags flags) -> b8
  {
    return ImGui::SliderFloat4(
      label.c_str(), val->data, v_min, v_max, "%.3f", into_imgui_slider_flags(flags));
  }

  // ----------------------------------------------------------------------------
  // Widgets: Color
  // ----------------------------------------------------------------------------
  auto Gui::color_edit3(CompStr label, NotNull<vec3f32*> value) -> b8
  {
    float color[3]       = {value->x, value->y, value->z};
    const auto is_change = ImGui::ColorEdit3(label.c_str(), color);
    if (is_change)
    {
      *value = {color[0], color[1], color[2]};
    }
    return is_change;
  }

  auto Gui::selectable(StringView label, b8 selected) -> b8
  {
    SOUL_ASSERT(0, label.is_null_terminated());
    return ImGui::Selectable(label.data(), selected);
  }

  auto Gui::gizmo(
    const mat4f32& view,
    const PerspectiveDesc& perspective_desc,
    vec2f32 rect_offset,
    vec2f32 rect_size,
    GizmoOp op,
    GizmoMode mode,
    NotNull<mat4f32*> transform_matrix) -> b8
  {
    const auto guizmo_view = math::transpose(view);
    auto guizmo_transform  = math::transpose(*transform_matrix);
    float perspecitve_data[16];
    ImGuizmo::Perspective(
      perspective_desc.fovy_degrees / 2,
      perspective_desc.aspect_ratio,
      perspective_desc.z_near,
      perspective_desc.z_far,
      perspecitve_data);

    const auto window_pos  = ImGui::GetWindowPos();
    const auto window_size = ImGui::GetWindowSize();
    ImGuizmo::SetImGuiContext(impl_->imgui_context);
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(
      window_pos.x + rect_offset.x, window_pos.y + rect_offset.y, rect_size.x, rect_size.y);
    ImGuizmo::Enable(true);
    const b8 is_changed = ImGuizmo::Manipulate(
      guizmo_view.data(),
      perspecitve_data,
      into_imguizmo_operation(op),
      into_imguizmo_mode(mode),
      guizmo_transform.data());
    if (is_changed)
    {
      *transform_matrix = math::transpose(guizmo_transform);
    }
    return is_changed;
  }

  void Gui::draw_grid(
    const mat4f32& view,
    const PerspectiveDesc& perspective_desc,
    const mat4f32& transform_matrix,
    f32 grid_size)
  {
    const auto guizmo_view = math::transpose(view);
    auto guizmo_transform  = math::transpose(transform_matrix);
    float perspecitve_data[16];
    ImGuizmo::Perspective(
      perspective_desc.fovy_degrees / 2,
      perspective_desc.aspect_ratio,
      perspective_desc.z_near,
      perspective_desc.z_far,
      perspecitve_data);
    ImGuizmo::SetImGuiContext(impl_->imgui_context);
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    const auto window_pos  = ImGui::GetWindowPos();
    const auto window_size = ImGui::GetWindowSize();
    ImGuizmo::SetRect(window_pos.x, window_pos.y, window_size.x, window_size.y);
    ImGuizmo::DrawGrid(guizmo_view.data(), perspecitve_data, guizmo_transform.data(), grid_size);
  }

  void Gui::image(gpu::TextureID texture_id, vec2f32 size)
  {
    ImGui::Image(GuiTextureID(texture_id), into_imgui_size(size));
  }

  void Gui::image(gpu::TextureNodeID texture_node_id, vec2f32 size)
  {
    impl_->texture_node_ids.push_back(texture_node_id);
    ImGui::Image(GuiTextureID(texture_node_id), into_imgui_size(size));
  }

  auto Gui::tree_node(u64 id, TreeNodeFlags flags, StringView name) -> b8
  {
    SOUL_ASSERT(0, name.is_null_terminated());
    return ImGui::TreeNodeEx(            // NOLINT
      reinterpret_cast<const void*>(id), // NOLINT
      into_imgui_tree_node_flags(flags),
      "%s",
      name.data().unwrap().get());
  }

  void Gui::tree_push(u64 id)
  {
    ImGui::TreePush((const void*)id); // NOLINT
  }

  void Gui::tree_pop()
  {
    ImGui::TreePop();
  }

  // ----------------------------------------------------------------------------
  // TabBar
  // ----------------------------------------------------------------------------
  auto Gui::begin_tab_bar(CompStr label) -> b8
  {
    return ImGui::BeginTabBar(label.c_str());
  }

  void Gui::end_tab_bar()
  {
    ImGui::EndTabBar();
  }

  auto Gui::begin_tab_item(CompStr label) -> b8
  {
    return ImGui::BeginTabItem(label.c_str());
  }

  void Gui::end_tab_item()
  {
    ImGui::EndTabItem();
  }

  auto Gui::collapsing_header(StringView label) -> b8
  {
    SOUL_ASSERT(0, label.is_null_terminated());
    return ImGui::CollapsingHeader(label.data());
  }

  void Gui::show_demo_window()
  {
    ImGui::ShowDemoWindow();
  }

  void Gui::show_style_editor()
  {
    ImGui::ShowStyleEditor();
  }

  // ----------------------------------------------------------------------------
  // Layout
  // ----------------------------------------------------------------------------

  void Gui::separator()
  {
    ImGui::Separator();
  }

  void Gui::same_line(f32 offset_from_start_x, f32 spacing)
  {
    ImGui::SameLine(offset_from_start_x, spacing);
  }

  void Gui::new_line()
  {
    ImGui::NewLine();
  }

  void Gui::spacing()
  {
    ImGui::Spacing();
  }

  void Gui::dummy(vec2f32 size)
  {
    ImGui::Dummy(into_imgui_size(size));
  }

  void Gui::indent(f32 indent_w)
  {
    ImGui::Indent(indent_w);
  }

  void Gui::unindent(f32 indent_w)
  {
    ImGui::Unindent(indent_w);
  }

  // ----------------------------------------------------------------------------
  // Input
  // ----------------------------------------------------------------------------
  auto Gui::is_item_clicked() -> b8
  {
    return ImGui::IsItemClicked();
  }

  auto Gui::is_window_hovered() const -> b8
  {
    return ImGui::IsWindowHovered();
  }

  auto Gui::is_mouse_down(MouseButton mouse_button) const -> b8
  {
    return ImGui::IsMouseDown(into_imgui_mouse_button(mouse_button));
  }

  auto Gui::is_mouse_clicked(MouseButton mouse_button) const -> b8
  {
    return ImGui::IsMouseClicked(into_imgui_mouse_button(mouse_button));
  }

  auto Gui::is_mouse_released(MouseButton mouse_button) const -> b8
  {
    return ImGui::IsMouseReleased(into_imgui_mouse_button(mouse_button));
  }

  auto Gui::is_mouse_double_clisked(MouseButton mouse_button) const -> b8
  {
    return ImGui::IsMouseDoubleClicked(into_imgui_mouse_button(mouse_button));
  }

  auto Gui::is_mouse_dragging(MouseButton mouse_button, f32 lock_threshold) const -> b8
  {
    return ImGui::IsMouseDragging(into_imgui_mouse_button(mouse_button), lock_threshold);
  }

  auto Gui::get_mouse_pos() const -> vec2f32
  {
    const auto pos = ImGui::GetMousePos();
    return vec2f32(pos.x, pos.y);
  }

  auto Gui::get_mouse_drag_delta(MouseButton mouse_button, f32 lock_threshold) const -> vec2f32
  {
    const auto delta =
      ImGui::GetMouseDragDelta(into_imgui_mouse_button(mouse_button), lock_threshold);
    return vec2f32(delta.x, delta.y);
  }

  auto Gui::get_mouse_wheel_delta() const -> f32
  {
    return ImGui::GetIO().MouseWheel;
  }

  auto Gui::get_mouse_delta() const -> vec2f32
  {
    const auto delta = ImGui::GetIO().MouseDelta;
    return vec2f32(delta.x, delta.y);
  }

  auto Gui::get_delta_time() const -> f32
  {
    return ImGui::GetIO().DeltaTime;
  }

  auto Gui::is_key_down(KeyboardKey key) const -> b8
  {
    return ImGui::IsKeyDown(into_imgui_key(key));
  }

  auto Gui::is_key_pressed(KeyboardKey key, b8 repeat) const -> b8
  {
    return ImGui::IsKeyPressed(into_imgui_key(key), repeat);
  }

  auto Gui::is_key_released(KeyboardKey key) const -> b8
  {
    return ImGui::IsKeyReleased(into_imgui_key(key));
  }

  void Gui::set_cursor_pos(vec2f32 pos)
  {
    ImGui::SetCursorPos(into_imgui_size(pos));
  }

  void Gui::push_id(i32 id)
  {
    ImGui::PushID(id);
  }

  void Gui::pop_id()
  {
    ImGui::PopID();
  }

  auto Gui::get_frame_rate() const -> f32
  {
    return ImGui::GetIO().Framerate;
  }
} // namespace soul::app
