#pragma once

#include "app/input_state.h"
#include "gpu/wsi.h"

#include "core/builtins.h"
#include "core/path.h"

class GLFWwindow;

namespace soul::app
{
  class GLFWWsi : public gpu::WSI
  {
  private:
    GLFWwindow* window_{};

  public:
    GLFWWsi(const GLFWWsi&) = delete;

    GLFWWsi(GLFWWsi&&) = delete;

    auto operator=(const GLFWWsi&) -> GLFWWsi& = delete;

    auto operator=(GLFWWsi&&) -> GLFWWsi& = delete;

    ~GLFWWsi() override = default;

    explicit GLFWWsi() = default;

    void set_window(GLFWwindow* window);

    [[nodiscard]]
    auto create_vulkan_surface(VkInstance instance) -> VkSurfaceKHR override;

    [[nodiscard]]
    auto get_framebuffer_size() const -> vec2u32 override;
  };

  class Window
  {
  public:
    enum class WindowMode
    {
      NORMAL,    ///< NORMAL WINDOW.
      MINIMIZED, ///< MINIMIZED WINDOW.
      MAXIMIZED,
      FULLSCREEN, ///< FULLSCREEN WINDOW.
      COUNT,
    };

    struct Desc
    {
      u32 width           = 1920;                  ///< The width of the client area size.
      u32 height          = 1080;                  ///< The height of the client area size.
      String title        = "Application"_str;     ///< Window title.
      WindowMode mode     = WindowMode::MAXIMIZED; ///< Window mode. In full screen mode, width and
                                                   ///< height will be ignored.
      b8 resizable_window = true;                  ///< Allow the user to resize the window.
    };

    class Callbacks
    {
    public:
      Callbacks(const Callbacks&) = default;

      Callbacks(Callbacks&&) = default;

      auto operator=(const Callbacks&) -> Callbacks& = default;

      auto operator=(Callbacks&&) -> Callbacks& = default;

      virtual ~Callbacks() = default;

      Callbacks() = default;

      virtual void handle_window_size_change() = 0;

      virtual void handle_render_frame() = 0;

      virtual void handle_keyboard_event(const KeyboardEvent& keyEvent) = 0;

      virtual void handle_mouse_event(const MouseEvent& mouseEvent) = 0;

      virtual void handle_window_focus_event(b8 focused) = 0;

      virtual void handle_dropped_file(const Path& path) = 0;
    };

  private:
    Desc desc_;
    GLFWwindow* glfw_window_;
    GLFWWsi wsi_;
    vec2f32 mouse_scale_;
    Callbacks* callbacks_ = nullptr;

  public:
    Window(const Window&) = delete;

    Window(Window&&) = delete;

    auto operator=(const Window&) = delete;

    auto operator=(Window&&) -> Window& = delete;

    ~Window();

    Window(OwnRef<Desc> desc, Callbacks* pCallbacks);

    void shutdown();

    [[nodiscard]]
    auto should_close() const -> b8;

    void resize(u32 width, u32 height);

    void msg_loop();

    void poll_for_events();

    void handle_gamepad_input();

    void set_window_pos(i32 x, i32 y);

    void set_window_title(StringView title);

    void set_window_icon(const Path& path);

    [[nodiscard]]
    auto wsi_ref() -> gpu::WSI&;

    [[nodiscard]]
    auto get_client_area_size() const -> vec2u32
    {
      return {desc_.width, desc_.height};
    }

    [[nodiscard]]
    auto get_desc() const -> const Desc&
    {
      return desc_;
    }

  private:
    void update_window_size();
    void set_window_size(u32 width, u32 height);

    [[nodiscard]]
    auto get_mouse_scale() const -> vec2f32
    {
      return mouse_scale_;
    }

    friend class ApiCallbacks;
  };
} // namespace soul::app
