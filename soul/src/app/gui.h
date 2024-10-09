#pragma once

#include "app/input_state.h"

#include "core/comp_str.h"
#include "core/string_view.h"
#include "gpu/render_graph.h"

#include "core/config.h"
#include "core/tuple.h"
#include "core/type.h"

namespace soul::gpu
{
  class System;
}

namespace soul::memory
{
  class Allocator;
}

namespace soul::app
{
  struct KeyboardEvent;
  struct MouseEvent;

  using GuiID = ID<struct gui_id_tag, u32, 0>;

  class Gui
  {
  public:
    enum class WindowFlag
    {
      SHOW_TITLE_BAR,
      ALLOW_MOVE,
      SET_FOCUS,
      CLOSE_BUTTON,
      NO_RESIZE,
      AUTO_RESIZE,
      NO_SCROLLBAR,
      COUNT,
    };
    using WindowFlags = FlagSet<WindowFlag>;
    static constexpr auto WINDOW_FLAGS_DEFAULT =
      WindowFlags{WindowFlag::SHOW_TITLE_BAR, WindowFlag::ALLOW_MOVE};

    enum class InputFlag
    {
      CHARS_DECIMAL,
      CHARS_HEXADECIMAL,
      CHARS_SCIENTIFIC,
      CHARS_UPPERCASE,
      CHARS_NO_BLANK,

      ALLOW_TAB_INPUT,
      ENTER_RETURNS_TRUE,

      ESCAPE_CLEARS_ALL,

      CTRL_ENTER_FOR_NEW_LINE,

      READ_ONLY,
      PASSWORD,
      ALWAYS_OVERWRITE,
      AUTO_SELECT_ALL,
      PARSE_EMPTY_REF_VAL,
      DISPLAY_EMPTY_REF_VAL,

      NO_HORIZONTAL_SCROLL,
      NO_UNDO_REDO,

      COUNT,
    };
    using InputFlags                         = FlagSet<InputFlag>;
    static constexpr auto INPUT_FLAG_DEFAULT = InputFlags{};

    enum class Direction
    {
      LEFT,
      RIGHT,
      UP,
      DOWN,
      COUNT,
    };

    enum class TreeNodeFlag
    {
      SELECTED,             // Draw as selected
      FRAMED,               // Draw frame with background
      ALLOW_OVERLAP,        // Hit testing to allow subsequent widgets to overlap this one
      NO_TREE_PUSH_ON_OPEN, // Don't do TreePush() when open
      DEFAULT_OPEN,         // Default node to be open
      OPEN_ON_DOUBLE_CLICK, // Need double-click to open node
      OPEN_ON_ARROW, // Only open when clicking on the arrow part. If OPEN_ON_DOUBLE_CLICK is also
                     // set, single click arrow or double-click all box to open
      LEAF,          // No collapsing, no arrow (use as convinience for leaf nodes)
      BULLET,        // Display a bullet instead of arrow
      FRAME_PADDING,
      SPAN_AVAIL_WIDTH,
      SPAN_FULL_WIDTH,
      SPAN_ALL_COLUMNS,
      COUNT
    };
    using TreeNodeFlags = FlagSet<TreeNodeFlag>;

    enum class GizmoMode
    {
      LOCAL,
      WORLD,
      COUNT
    };
    enum class GizmoOp
    {
      TRANSLATE,
      ROTATE,
      SCALE,
      COUNT
    };

    struct PerspectiveDesc
    {
      f32 fovy_degrees;
      f32 aspect_ratio;
      f32 z_near;
      f32 z_far;
    };

    using DockID = u32;

  private:
    struct Impl;
    Impl* impl_;
    NotNull<memory::Allocator*> allocator_;

  public:
    explicit Gui(
      NotNull<gpu::System*> gpu_system,
      f32 scale_factor,
      NotNull<memory::Allocator*> allocator = get_default_allocator());

    Gui(const Gui&) = delete;

    Gui(Gui&&) noexcept;

    auto operator=(const Gui&) -> Gui& = delete;

    auto operator=(Gui&&) noexcept -> Gui&;

    ~Gui();

    void cleanup();

    void begin_frame();

    void render_frame(
      NotNull<gpu::RenderGraph*> render_graph,
      gpu::TextureNodeID render_target,
      double elapsed_second);

    void on_window_resize(u32 width, u32 height);

    auto on_mouse_event(const MouseEvent& mouse_event) -> b8;

    auto on_keyboard_event(const KeyboardEvent& keyboard_event) -> b8;

    void on_window_focus_event(b8 focused);

    void begin_dock_window();

    // ----------------------------------------------------------------------------
    // Window
    // ----------------------------------------------------------------------------
    auto begin_window(
      CompStr label,
      vec2f32 size,
      vec2f32 pos       = {0, 0},
      WindowFlags flags = WINDOW_FLAGS_DEFAULT) -> b8;

    void end_window();

    [[nodiscard]]
    auto get_window_pos() const -> vec2f32;

    [[nodiscard]]
    auto get_window_size() const -> vec2f32;

    // ----------------------------------------------------------------------------
    // Dock
    // ----------------------------------------------------------------------------
    auto dock_space(GuiID gui_id) -> GuiID;

    auto dock_builder_is_node_exist(GuiID dock_id) -> b8;

    auto dock_builder_split_dock(GuiID parent_dock_id, Direction split_dir, f32 ratio)
      -> Tuple<GuiID, GuiID>;

    auto dock_builder_init(GuiID dock_id) -> b8;

    void dock_builder_dock_window(CompStr label, GuiID dock_id);

    void dock_builder_finish(GuiID dock_id);

    // ----------------------------------------------------------------------------
    // Widgets: Text
    // ----------------------------------------------------------------------------

    void text(StringView text);

    void text_disabled(StringView text);

    void text_colored(StringView text, vec4f32 color);

    void label_text(CompStr label, StringView text);

    void separator_text(CompStr label);

    // ----------------------------------------------------------------------------
    // Widgets: Main
    // ----------------------------------------------------------------------------
    auto button(CompStr label, vec2f32 size = {0, 0}) -> b8;

    auto button(StringView label, vec2f32 size = {0, 0}) -> b8;

    auto image_button(
      CompStr label,
      gpu::TextureID texture_id,
      vec4f32 tint_normal,
      vec4f32 tint_hovered,
      vec4f32 tint_pressed,
      vec2f32 size = {}) -> b8;

    auto image_button(
      CompStr label,
      const Path& path,
      vec4f32 tint_normal,
      vec4f32 tint_hovered,
      vec4f32 tint_pressed,
      vec2f32 size = {}) -> b8;

    auto checkbox(CompStr label, NotNull<b8*> value) -> b8;

    auto radio_button(CompStr label, NotNull<i32*> val, i32 button_val) -> b8;

    // ----------------------------------------------------------------------------
    // Widgets: Input
    // ----------------------------------------------------------------------------

    auto input_text(CompStr label, String* text) -> b8;

    auto input_text(CompStr label, Span<char*> buffer) -> b8;

    auto input_i32(CompStr label, NotNull<i32*> value) -> b8;

    auto input_f32(CompStr label, NotNull<f32*> value) -> b8;

    auto input_vec3f32(CompStr label, NotNull<vec3f32*> value) -> b8;

    auto input_vec3i32(CompStr label, NotNull<vec3i32*> value) -> b8;

    // ----------------------------------------------------------------------------
    // Widgets: Combo
    // ----------------------------------------------------------------------------

    auto begin_combo(CompStr label, StringView preview) -> b8;

    void end_combo();

    // ----------------------------------------------------------------------------
    // Widgets: Slider
    // ----------------------------------------------------------------------------
    enum class SliderFlag
    {
      ALWAYS_CLAMP,
      LOGARITHMIC,
      NO_ROUND_TO_FORMAT,
      NO_INPUT,
      COUNT
    };

    using SliderFlags = FlagSet<SliderFlag>;

    auto slider_i32(CompStr label, NotNull<i32*> val, i32 min, i32 max, SliderFlags flags = {})
      -> b8;

    auto slider_f32(CompStr label, NotNull<f32*> val, f32 v_min, f32 v_max, SliderFlags flags = {})
      -> b8;

    auto slider_vec2f32(
      CompStr label, NotNull<vec2f32*> val, f32 v_min, f32 v_max, SliderFlags flags = {}) -> b8;

    auto slider_vec3f32(
      CompStr label, NotNull<vec3f32*> val, f32 v_min, f32 v_max, SliderFlags flags = {}) -> b8;

    auto slider_vec4f32(
      CompStr label, NotNull<vec4f32*> val, f32 v_min, f32 v_max, SliderFlags flags = {}) -> b8;

    // ----------------------------------------------------------------------------
    // Widgets: Menu Bar
    // ----------------------------------------------------------------------------
    auto begin_main_menu_bar() -> b8;

    void end_main_menu_bar();

    auto begin_menu(CompStr label) -> b8;

    void end_menu();

    auto menu_item(CompStr label) -> b8;

    // ----------------------------------------------------------------------------
    // Popup
    // ----------------------------------------------------------------------------
    auto begin_popup(CompStr label) -> b8;

    auto begin_popup_modal(CompStr label) -> b8;

    void end_popup();

    void open_popup(CompStr label);

    void close_current_popup();

    void set_item_default_focus();

    auto get_id(CompStr label) -> GuiID;

    // ----------------------------------------------------------------------------
    // Widgets: Color
    // ----------------------------------------------------------------------------
    auto color_edit3(CompStr label, NotNull<vec3f32*> value) -> b8;

    // ----------------------------------------------------------------------------
    // Widgets: Selectable
    // ----------------------------------------------------------------------------
    auto selectable(StringView label, b8 selected) -> b8;

    auto gizmo(
      const mat4f32& view,
      const PerspectiveDesc& perspective_desc,
      vec2f32 rect_min,
      vec2f32 rect_size,
      GizmoOp op,
      GizmoMode mode,
      NotNull<mat4f32*> transform_matrix) -> b8;

    void draw_grid(
      const mat4f32& view,
      const PerspectiveDesc& perspective_desc,
      const mat4f32& transform_matrix,
      f32 grid_size);

    void image(gpu::TextureID texture_id, vec2f32 size);

    void image(gpu::TextureNodeID texture_node_id, vec2f32 size);

    [[nodiscard]]
    auto tree_node(u64 id, TreeNodeFlags flags, StringView name) -> b8;

    void tree_push(u64 id);

    void tree_pop();

    // ----------------------------------------------------------------------------
    // TabBar
    // ----------------------------------------------------------------------------
    auto begin_tab_bar(CompStr label) -> b8;

    void end_tab_bar();

    auto begin_tab_item(CompStr label) -> b8;

    void end_tab_item();

    auto collapsing_header(StringView label) -> b8;

    void show_demo_window();

    void show_style_editor();

    // ----------------------------------------------------------------------------
    // Layout
    // ----------------------------------------------------------------------------
    void separator();

    void same_line(f32 offset_from_start_x = 0.0_f32, f32 spacing = -1.0_f32);

    void new_line();

    void spacing();

    void dummy(vec2f32 size);

    void indent(f32 indent_w = 0.0_f32);

    void unindent(f32 indent_w = 0.0_f32);

    void push_item_width(f32 width);

    void pop_item_width();

    [[nodiscard]]
    auto is_item_clicked() -> b8;

    [[nodiscard]]
    auto is_window_hovered() const -> b8;

    [[nodiscard]]
    auto is_mouse_down(MouseButton mouse_button) const -> b8;

    [[nodiscard]]
    auto is_mouse_clicked(MouseButton mouse_button) const -> b8;

    [[nodiscard]]
    auto is_mouse_released(MouseButton mouse_button) const -> b8;

    [[nodiscard]]
    auto is_mouse_double_clisked(MouseButton mouse_button) const -> b8;

    [[nodiscard]]
    auto is_mouse_dragging(MouseButton mouse_button, f32 lock_threshold = -1.0f) const -> b8;

    [[nodiscard]]
    auto get_mouse_pos() const -> vec2f32;

    [[nodiscard]]
    auto get_mouse_drag_delta(MouseButton mouse_button, f32 lock_threshold = -1.0f) const
      -> vec2f32;

    [[nodiscard]]
    auto get_mouse_wheel_delta() const -> f32;

    [[nodiscard]]
    auto get_mouse_delta() const -> vec2f32;

    [[nodiscard]]
    auto get_delta_time() const -> f32;

    [[nodiscard]]
    auto is_key_down(KeyboardKey key) const -> b8;

    [[nodiscard]]
    auto is_key_pressed(KeyboardKey key, b8 repeat = true) const -> b8;

    [[nodiscard]]
    auto is_key_released(KeyboardKey) const -> b8;

    void set_cursor_pos(vec2f32 pos);

    void push_id(StringView id);

    void push_id(i32 id);

    void pop_id();

    [[nodiscard]]
    auto get_frame_rate() const -> f32;

    [[nodiscard]]
    auto get_display_size() const -> vec2f32;
  };
} // namespace soul::app
