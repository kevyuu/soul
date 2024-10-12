#pragma once

#include "app/input_state.h"

#include "core/comp_str.h"
#include "core/floating_point.h"
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
    enum class ColorVar : u8
    {
      TEXT,
      TEXT_DISABLED,
      WINDOW_BG,
      CHILD_BG,
      POPUP_BG,
      BORDER,
      BORDER_SHADOW,
      FRAME_BG,
      FRAME_BG_HOVERED,
      FRAME_BG_ACTIVE,
      TITLE_BG,
      TITLE_BG_ACTIVE,
      TITLE_BG_COLLAPSED,
      MENU_BAR_BG,
      SCROLLBAR_BG,
      SCROLLBAR_GRAB,
      SCROLLBAR_GRAB_HOVERED,
      SCROLLBAR_GRAB_ACTIVE,
      CHECK_MARK,
      SLIDER_GRAB,
      SLIDER_GRAB_ACTIVE,
      BUTTON,
      BUTTON_HOVERED,
      BUTTON_ACTIVE,
      HEADER,
      HEADER_HOVERED,
      HEADER_ACTIVE,
      SEPARATOR,
      SEPARATOR_HOVERED,
      SEPARATOR_ACTIVE,
      RESIZE_GRIP,
      RESIZE_GRIP_HOVERED,
      RESIZE_GRIP_ACTIVE,
      TAB_HOVERED,
      TAB,
      TAB_SELECTED,
      TAB_SELECTED_OVERLINE,
      TAB_DIMMED,
      TAB_DIMMED_SELECTED,
      TAB_DIMMED_SELECTED_OVERLINE,
      DOCKING_PREVIEW,
      DOCKING_EMPTY_BG,
      PLOT_LINES,
      PLOT_LINES_HOVERED,
      PLOT_HISTOGRAM,
      PLOT_HISTOGRAM_HOVERED,
      TABLE_HEADER_BG,
      TABLE_BORDER_STRONG,
      TABLE_BORDER_LIGHT,
      TABLE_ROW_BG,
      TABLE_ROW_BG_ALT,
      TEXT_LINK,
      TEXT_SELECTED_BG,
      DRAG_DROP_TARGET,
      NAV_HIGHLIGHT,
      NAV_WINDOWING_HIGHLIGHT,
      NAV_WINDOWING_DIM_BG,
      MODAL_WINDOW_DIM_BG,
      COUNT
    };

    enum class StyleVar : u8
    {
      ALPHA,
      DISABLED_ALPHA,
      WINDOW_PADDING,
      WINDOW_ROUNDING,
      WINDOW_BORDER_SIZE,
      WINDOW_MIN_SIZE,
      WINDOW_TITLE_ALIGN,
      CHILD_ROUNDING,
      CHILD_BORDER_SIZE,
      POPUP_ROUNDING,
      POPUP_BORDER_SIZE,
      FRAME_PADDING,
      FRAME_ROUNDING,
      FRAME_BORDER_SIZE,
      ITEM_SPACING,
      ITEM_INNER_SPACING,
      INDENT_SPACING,
      CELL_PADDING,
      SCROLLBAR_SIZE,
      SCROLLBAR_ROUNDING,
      GRAB_MIN_SIZE,
      GRAB_ROUNDING,
      TAB_ROUNDING,
      TAB_BORDER_SIZE,
      TAB_BAR_BORDER_SIZE,
      TAB_BAR_OVERLINE_SIZE,
      TABLE_ANGLED_HEADERS_ANGLE,
      TABLE_ANGLED_HEADERS_TEXT_ALIGN,
      BUTTON_TEXT_ALIGN,
      SELECTABLE_TEXT_ALIGN,
      SEPARATOR_TEXT_BORDER_SIZE,
      SEPARATOR_TEXT_ALIGN,
      SEPARATOR_TEXT_PADDING,
      DOCKING_SEPARATOR_SIZE,
      COUNT,
    };

    enum class WindowFlag
    {
      NO_TITLE_BAR,
      NO_RESIZE,
      NO_MOVE,
      NO_SCROLLBAR,
      NO_SCROLL_WITH_MOUSE,

      NO_COLLAPSE,

      ALWAYS_AUTO_RESIZE,
      NO_BACKGROUND,

      NO_SAVED_SETTINGS,
      NO_MOUSE_INPUTS,
      MENU_BAR,
      HORIZONTAL_SCROLLBAR,

      NO_FOCUS_ON_APPEARING,
      NO_BRING_TO_FRONT_ON_FOCUS,

      ALWAYS_VERTICAL_SCROLLBAR,
      ALWAYS_HORIZONTAL_SCROLLBAR,
      NO_NAV_INPUTS,
      NO_NAV_FOCUS,

      UNSAVED_DOCUMENT,

      NO_DOCKING,
      COUNT,
    };
    using WindowFlags                                = FlagSet<WindowFlag>;
    static constexpr WindowFlags WINDOW_FLAGS_NO_NAV = {
      WindowFlag::NO_NAV_INPUTS, WindowFlag::NO_NAV_INPUTS};
    static constexpr WindowFlags WINDOW_FALGS_NO_DECORATION = {
      WindowFlag::NO_TITLE_BAR,
      WindowFlag::NO_RESIZE,
      WindowFlag::NO_SCROLLBAR,
      WindowFlag::NO_COLLAPSE};
    static constexpr WindowFlags WINDOW_FLAGS_NO_INPUTS = {
      WindowFlag::NO_MOUSE_INPUTS,
      WindowFlag::NO_NAV_INPUTS,
      WindowFlag::NO_NAV_FOCUS,
    };

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
    using InputFlags = FlagSet<InputFlag>;

    enum class FocusedFlag : u8
    {
      CHILD_WINDOWS,
      ROOT_WINDOW,
      ANY_WINDOW,
      NO_POPUP_HIERARCHY,
      DOCK_HIERARCHY,
      COUNT,
    };
    using FocusedFlags = FlagSet<FocusedFlag>;

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

    // ----------------------------------------------------------------------------
    // Window
    // ----------------------------------------------------------------------------
    enum class LayoutCond : u8
    {
      ALWAYS,
      ONCE,
      FIRST_USE_EVER,
      APPEARING,
      COUNT,
    };

    auto begin_window(
      StringView label,
      vec2f32 size,
      vec2f32 pos            = {0, 0},
      WindowFlags flags      = {},
      LayoutCond layout_cond = LayoutCond::FIRST_USE_EVER) -> b8;

    void end_window();

    enum class ChildWindowFlag : u8
    {
      BORDERS,
      ALWAYS_USE_WINDOW_PADDING,
      RESIZE_X,
      RESIZE_Y,
      AUTO_RESIZE_X,
      AUTO_RESIZE_Y,
      ALWAYS_AUTO_RESIZE,
      FRAME_STYLE,
      NAV_FLATTENED,
      COUNT,
    };
    using ChildWindowFlags = FlagSet<ChildWindowFlag>;

    auto begin_child_window(
      StringView label,
      vec2f32 size                        = vec2f32(0, 0),
      ChildWindowFlags child_window_flags = {},
      WindowFlags window_flags            = {}) -> b8;

    void end_child_window();

    // ----------------------------------------------------------------------------
    // Parameter Stacks (Shared)
    // ----------------------------------------------------------------------------
    void push_style_color(ColorVar color_var, vec4f32 color);

    void pop_style_color();

    void push_style_var(StyleVar style_var, vec2f32 value);

    void pop_style_var();

    void push_font_size(f32 font_size);

    void pop_font_size();

    // ----------------------------------------------------------------------------
    // Parameter Stacks (Window)
    // ----------------------------------------------------------------------------
    void push_item_width(f32 item_width);

    void pop_item_width();

    void set_next_item_width(f32 item_width);

    auto calc_item_width() -> f32;

    void push_text_wrap_pos(f32 wrap_local_pos_x = 0.0f);

    void pop_text_wrap_pos();

    // ----------------------------------------------------------------------------
    // Layout cursor positioning
    // ----------------------------------------------------------------------------
    auto get_cursor_screen_pos() const -> vec2f32;

    void set_cursor_screen_pos(const vec2f32& pos);

    auto get_content_region_avail() const -> vec2f32;

    auto get_cursor_pos() -> vec2f32;

    void set_cursor_pos(const vec2f32& local_pos);

    void set_cursor_pos_x(f32 local_x);

    void set_cursor_pos_y(f32 local_y);

    auto get_frame_padding() const -> vec2f32;

    auto get_frame_height() const -> f32;

    auto get_frame_height(f32 font_size) const -> f32;

    // ----------------------------------------------------------------------------
    // Other layout functions
    // ----------------------------------------------------------------------------
    void separator();

    void same_line(f32 offset_from_start_x = 0.0_f32, f32 spacing = -1.0_f32);

    void new_line();

    void spacing();

    void dummy(vec2f32 size);

    void indent(f32 indent_w = 0.0_f32);

    void unindent(f32 indent_w = 0.0_f32);

    void begin_group();

    void end_group();

    void align_text_to_frame_padding();

    // ----------------------------------------------------------------------------
    // ID stack / scopes
    // ----------------------------------------------------------------------------
    void push_id(StringView id);

    void push_id(i32 id);

    void pop_id();

    auto get_id(StringView label) -> GuiID;

    // ----------------------------------------------------------------------------
    // Dock
    // ----------------------------------------------------------------------------
    void begin_dock_window();

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

    void text(StringView label);

    void text(StringView label, f32 font_size);

    void text_disabled(StringView label);

    void text_colored(StringView label, vec4f32 color);

    void text_colored(StringView label, vec4f32 color, f32 font_size);

    void text_wrapped(StringView label);

    void text_wrapped(StringView label, f32 font_size);

    void label_text(StringView label, StringView text);

    void separator_text(StringView label);

    // ----------------------------------------------------------------------------
    // Widgets: Main
    // ----------------------------------------------------------------------------
    auto invisible_button(StringView label, vec2f32 size = {0, 0}) -> b8;

    auto button(StringView label, vec2f32 size = {0, 0}) -> b8;

    auto image_button(
      StringView label,
      gpu::TextureID texture_id,
      vec4f32 tint_normal,
      vec4f32 tint_hovered,
      vec4f32 tint_pressed,
      vec2f32 size = {}) -> b8;

    auto image_button(
      StringView label,
      const Path& path,
      vec4f32 tint_normal,
      vec4f32 tint_hovered,
      vec4f32 tint_pressed,
      vec2f32 size = {}) -> b8;

    auto checkbox(StringView label, NotNull<b8*> value) -> b8;

    auto radio_button(StringView label, NotNull<i32*> val, i32 button_val) -> b8;

    // ----------------------------------------------------------------------------
    // Widgets: Input
    // ----------------------------------------------------------------------------

    auto input_text(StringView label, String* text, InputFlags flags = {}) -> b8;

    auto input_text_multiline(StringView label, String* text, vec2f32 size = vec2f32(0, 0)) -> b8;

    auto input_text(StringView label, Span<char*> buffer) -> b8;

    auto input_i32(StringView label, NotNull<i32*> value) -> b8;

    auto input_f32(StringView label, NotNull<f32*> value) -> b8;

    auto input_vec3f32(StringView label, NotNull<vec3f32*> value) -> b8;

    auto input_vec3i32(StringView label, NotNull<vec3i32*> value) -> b8;

    // ----------------------------------------------------------------------------
    // Widgets: Combo
    // ----------------------------------------------------------------------------

    auto begin_combo(StringView label, StringView preview) -> b8;

    void end_combo();

    // ----------------------------------------------------------------------------
    // Widgets: Listbox
    // ----------------------------------------------------------------------------

    auto begin_list_box(StringView label, const vec2f32 size = {}) -> b8;

    void end_list_box();

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

    auto slider_i32(StringView label, NotNull<i32*> val, i32 min, i32 max, SliderFlags flags = {})
      -> b8;

    auto slider_f32(
      StringView label, NotNull<f32*> val, f32 v_min, f32 v_max, SliderFlags flags = {}) -> b8;

    auto slider_vec2f32(
      StringView label, NotNull<vec2f32*> val, f32 v_min, f32 v_max, SliderFlags flags = {}) -> b8;

    auto slider_vec3f32(
      StringView label, NotNull<vec3f32*> val, f32 v_min, f32 v_max, SliderFlags flags = {}) -> b8;

    auto slider_vec4f32(
      StringView label, NotNull<vec4f32*> val, f32 v_min, f32 v_max, SliderFlags flags = {}) -> b8;

    // ----------------------------------------------------------------------------
    // Widgets: Menu Bar
    // ----------------------------------------------------------------------------
    auto begin_main_menu_bar() -> b8;

    void end_main_menu_bar();

    auto begin_menu(StringView label) -> b8;

    void end_menu();

    auto menu_item(StringView label) -> b8;

    // ----------------------------------------------------------------------------
    // Popup
    // ----------------------------------------------------------------------------
    auto begin_popup(StringView label) -> b8;

    auto begin_popup_modal(StringView label) -> b8;

    void end_popup();

    void open_popup(StringView label);

    void close_current_popup();

    void set_item_default_focus();

    // ----------------------------------------------------------------------------
    // Tables
    // ----------------------------------------------------------------------------
    enum class TableFlag : u8
    {
      RESIZABLE,
      REORDERABLE,
      HIDEABLE,
      SORTABLE,
      NO_SAVED_SETTINGS,
      CONTEXT_MENU_IN_BODY,
      ROW_BG,
      BORDERS_INNER_H,
      BORDERS_OUTER_H,
      BORDERS_INNER_V,
      BORDERS_OUTER_V,
      NO_BORDERS_IN_BODY,
      NO_BORDERS_IN_BODY_UNTIL_RESIZE,
      SIZING_FIXED_FIT,
      SIZING_FIXED_SAME,
      SIZING_STRETCH_SAME,
      NO_HOST_EXTEND_X,
      NO_HOST_EXTEND_Y,
      NO_KEEP_COLUMNS_VISIBLE,
      PRECISE_WIDTHS,
      NO_CLIP,
      PAD_OUTER_X,
      NO_PAD_OUTER_X,
      NO_PAD_INNER_X,
      SCROLL_X,
      SCROLL_Y,
      SORT_MULTI,
      SORT_TRISTATE,
      HIGHLIGHT_HOVERED_COLUMN,
      COUNT,
    };
    using TableFlags = FlagSet<TableFlag>;

    enum class TableColumnFlag : u8
    {
      DISABLED,
      DEFAULT_HIDE,
      DEFAULT_SORT,
      WIDTH_STRETCH,
      WIDTH_FIXED,
      NO_RESIZE,
      NO_REORDER,
      NO_HIDE,
      NO_CLIP,
      NO_SORT,
      NO_SORT_ASCENDING,
      NO_SORT_DESCENDING,
      NO_HEADER_LABEL,
      NO_HEADER_WIDTH,
      PREFER_SORT_ASCENDING,
      PREFER_SORT_DESCENDING,
      INDENT_ENABLE,
      INDENT_DISABLE,
      ANGLED_HEADER,
      IS_ENABLED,
      IS_VISIBLE,
      IS_SORTED,
      IS_HOVERED,
      COUNT,
    };
    using TableColumnFlags = FlagSet<TableColumnFlag>;

    enum class TableRowFlag : u8
    {
      HEADERS,
      COUNT
    };
    using TableRowFlags = FlagSet<TableRowFlag>;

    auto begin_table(
      StringView str_id,
      u32 columns,
      TableFlags flags          = {},
      const vec2f32& outer_size = vec2f32(0.0f, 0.0f),
      f32 inner_width           = 0.0f) -> b8;

    void end_table();

    void table_next_row(TableRowFlags row_flags = {}, f32 min_row_height = 0.0f);

    auto table_next_column() -> b8;

    auto table_set_column_index(u32 column_n) -> b8;

    void table_setup_column(
      StringView label,
      TableColumnFlags flags   = {},
      f32 init_width_or_weight = 0.0f,
      GuiID user_id            = GuiID());

    void table_setup_scroll_freeze(i32 cols, i32 rows);

    void table_header(StringView label);

    void table_headers_row();

    void table_angled_headers_row();

    // ----------------------------------------------------------------------------
    // Widgets: Color
    // ----------------------------------------------------------------------------
    auto color_edit3(StringView label, NotNull<vec3f32*> value) -> b8;

    // ----------------------------------------------------------------------------
    // Widgets: Selectable
    // ----------------------------------------------------------------------------
    enum class SelectableFlag : u8
    {
      NO_AUTO_CLOSE_POPUPS,
      SPAN_ALL_COLUMNS,
      ALLOW_DOUBLE_CLICK,
      DISABLED,
      ALLOW_OVERLAP,
      HIGHLIGHT,
      COUNT,
    };
    using SelectableFlags = FlagSet<SelectableFlag>;

    auto selectable(
      StringView label, b8 selected, SelectableFlags flags = {}, vec2f32 size = {0, 0}) -> b8;

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
    auto begin_tab_bar(StringView label) -> b8;

    void end_tab_bar();

    auto begin_tab_item(StringView label) -> b8;

    void end_tab_item();

    auto collapsing_header(StringView label) -> b8;

    void show_demo_window();

    void show_style_editor();

    // ----------------------------------------------------------------------------
    // Item/Widgets Utilities and Query Functions
    // ----------------------------------------------------------------------------

    [[nodiscard]]
    auto is_item_hovered() -> b8;

    [[nodiscard]]
    auto is_item_active() -> b8;

    [[nodiscard]]
    auto is_item_focused() -> b8;

    [[nodiscard]]
    auto is_item_clicked(MouseButton mouse_button = MouseButton::LEFT) -> b8;

    [[nodiscard]]
    auto is_item_visible() -> b8;

    [[nodiscard]]
    auto is_item_edited() -> b8;

    [[nodiscard]]
    auto is_item_activated() -> b8;

    [[nodiscard]]
    auto is_item_deactivated() -> b8;

    [[nodiscard]]
    auto is_item_deactivated_after_edit() -> b8;

    // ----------------------------------------------------------------------------
    // Window Utilities and Query Functions
    // ----------------------------------------------------------------------------

    [[nodiscard]]
    auto is_window_appearing() const -> b8;

    [[nodiscard]]
    auto is_window_collapsed() const -> b8;

    [[nodiscard]]
    auto is_window_focused(FocusedFlags focused_flags = {}) const -> b8;

    [[nodiscard]]
    auto is_window_hovered() const -> b8;

    [[nodiscard]]
    auto get_window_pos() const -> vec2f32;

    [[nodiscard]]
    auto get_window_size() const -> vec2f32;

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

    [[nodiscard]]
    auto get_frame_rate() const -> f32;

    [[nodiscard]]
    auto get_display_size() const -> vec2f32;

    auto open_file_dialog(
      StringView name,
      const Path& initial_path     = Path::From(""_str),
      StringView filter_name       = "File"_str,
      StringView filter_extensions = "*"_str) -> Option<Path>;

    auto open_folder_dialog(StringView name, const Path& default_path = Path::From(""_str))
      -> Option<Path>;
  };
} // namespace soul::app
