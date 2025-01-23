#include "app/window.h"

#include "app/input_state.h"

#include "core/log.h"
#include "core/option.h"
#include "core/panic.h"
#include "core/path.h"
#include "core/string.h"

#include "runtime/runtime.h"
#include "runtime/scope_allocator.h"

#include "misc/string_util.h"

#include "gpu/wsi.h"

#include <atomic>
#include <volk.h>
#include <GLFW/glfw3.h>

namespace soul::app
{

  void GLFWWsi::set_window(GLFWwindow* window)
  {
    window_ = window;
  }

  [[nodiscard]]
  auto GLFWWsi::create_vulkan_surface(VkInstance instance) -> VkSurfaceKHR
  {
    VkSurfaceKHR surface; // NOLINT
    SOUL_LOG_INFO("Creating vulkan surface");
    glfwCreateWindowSurface(instance, window_, nullptr, &surface);
    SOUL_LOG_INFO("Vulkan surface creation sucessful.");
    return surface;
  }

  [[nodiscard]]
  auto GLFWWsi::get_framebuffer_size() const -> vec2u32
  {
    int width, height; // NOLINT
    glfwGetFramebufferSize(window_, &width, &height);
    return {cast<u32>(width), cast<u32>(height)};
  }

  class ApiCallbacks
  {
  public:
    static void window_size_callback(GLFWwindow* glfw_window, i32 width, i32 height)
    {
      auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
      if (window != nullptr)
      {
        window->resize(width, height); // Window callback is handled in here
      }
    }

    static void keyboard_callback(
      GLFWwindow* glfw_window, i32 key, i32 /* scan_code */, i32 action, i32 modifiers)
    {
      const Option<KeyboardEvent> opt_event = try_get_keyboard_event(key, action, modifiers);
      if (opt_event.is_some())
      {
        const auto event = opt_event.unwrap();
        auto* window     = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
        if (window != nullptr)
        {
          window->callbacks_->handle_keyboard_event(event);
        }
      }
    }

    static void char_input_callback(GLFWwindow* glfw_window, u32 input)
    {
      const auto keyboard_event = KeyboardEvent{
        .type      = KeyboardEvent::Type::INPUT,
        .codepoint = input,
      };

      auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
      if (window != nullptr)
      {
        window->callbacks_->handle_keyboard_event(keyboard_event);
      }
    }

    static void mouse_move_callback(GLFWwindow* glfw_window, f64 mouse_x, f64 mouse_y)
    {
      auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
      if (window != nullptr)
      {
        // Prepare the mouse data
        const auto mouse_event = MouseEvent{
          .type        = MouseEvent::Type::MOVE,
          .pos         = calc_mouse_pos(mouse_x, mouse_y, window->get_mouse_scale()),
          .screen_pos  = {f32(mouse_x), f32(mouse_y)},
          .wheel_delta = {0.0f, 0.0f},
        };

        window->callbacks_->handle_mouse_event(mouse_event);
      }
    }

    static void mouse_button_callback(
      GLFWwindow* glfw_window, i32 button, i32 action, i32 modifiers)
    {
      MouseEvent event;
      // Prepare the mouse data
      MouseEvent::Type type =
        (action == GLFW_PRESS) ? MouseEvent::Type::BUTTON_DOWN : MouseEvent::Type::BUTTON_UP;
      switch (button)
      {
      case GLFW_MOUSE_BUTTON_LEFT:
        event.type   = type;
        event.button = MouseButton::LEFT;
        break;
      case GLFW_MOUSE_BUTTON_MIDDLE:
        event.type   = type;
        event.button = MouseButton::MIDDLE;
        break;
      case GLFW_MOUSE_BUTTON_RIGHT:
        event.type   = type;
        event.button = MouseButton::RIGHT;
        break;
      default:
        // Other keys are not supported
        return;
      }

      auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
      if (window != nullptr)
      {
        // Modifiers
        event.mods = get_modifier_flags(modifiers);
        f64 x      = NAN;
        f64 y      = NAN;
        glfwGetCursorPos(glfw_window, &x, &y);
        event.pos = calc_mouse_pos(x, y, window->get_mouse_scale());

        window->callbacks_->handle_mouse_event(event);
      }
    }

    static void mouse_wheel_callback(GLFWwindow* glfw_window, f64 scrollX, f64 scrollY)
    {
      auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
      if (window != nullptr)
      {
        f64 x = NAN;
        f64 y = NAN;
        glfwGetCursorPos(glfw_window, &x, &y);

        const auto mouse_event = MouseEvent{
          .type        = MouseEvent::Type::WHEEL,
          .pos         = calc_mouse_pos(x, y, window->get_mouse_scale()),
          .wheel_delta = (vec2f32(f32(scrollX), f32(scrollY))),
        };

        window->callbacks_->handle_mouse_event(mouse_event);
      }
    }

    static void error_callback(i32 errorCode, const char* pDescription)
    {
      // GLFW errors are always recoverable. Therefore we just log the error.
      SOUL_LOG_ERROR("GLFW error {}: {}", errorCode, pDescription);
    }

    static void dropped_file_callback(GLFWwindow* glfw_window, i32 count, const char** paths)
    {
      auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
      if (window != nullptr)
      {
        for (i32 i = 0; i < count; i++)
        {
          const auto path = Path::From(StringView(paths[i]));
          window->callbacks_->handle_dropped_file(path);
        }
      }
    }

  private:
    static inline auto glfw_to_soul_key(i32 glfw_key) -> KeyboardKey
    {
      static_assert(GLFW_KEY_ESCAPE == 256, "GLFW_KEY_ESCAPE is expected to be 256");
      static_assert(
        (u32)KeyboardKey::ESCAPE >= 256, "KeyboardKey::Escape is expected to be at least 256");

      if (glfw_key < GLFW_KEY_ESCAPE)
      {
        // Printable keys are expected to have the same value
        return (KeyboardKey)glfw_key;
      }

      switch (glfw_key)
      {
      case GLFW_KEY_ESCAPE: return KeyboardKey::ESCAPE;
      case GLFW_KEY_ENTER: return KeyboardKey::ENTER;
      case GLFW_KEY_TAB: return KeyboardKey::TAB;
      case GLFW_KEY_BACKSPACE: return KeyboardKey::BACKSPACE;
      case GLFW_KEY_INSERT: return KeyboardKey::INSERT;
      case GLFW_KEY_DELETE: return KeyboardKey::DEL;
      case GLFW_KEY_RIGHT: return KeyboardKey::RIGHT;
      case GLFW_KEY_LEFT: return KeyboardKey::LEFT;
      case GLFW_KEY_DOWN: return KeyboardKey::DOWN;
      case GLFW_KEY_UP: return KeyboardKey::UP;
      case GLFW_KEY_PAGE_UP: return KeyboardKey::PAGE_UP;
      case GLFW_KEY_PAGE_DOWN: return KeyboardKey::PAGE_DOWN;
      case GLFW_KEY_HOME: return KeyboardKey::HOME;
      case GLFW_KEY_END: return KeyboardKey::END;
      case GLFW_KEY_CAPS_LOCK: return KeyboardKey::CAPS_LOCK;
      case GLFW_KEY_SCROLL_LOCK: return KeyboardKey::SCROLL_LOCK;
      case GLFW_KEY_NUM_LOCK: return KeyboardKey::NUM_LOCK;
      case GLFW_KEY_PRINT_SCREEN: return KeyboardKey::PRINT_SCREEN;
      case GLFW_KEY_PAUSE: return KeyboardKey::PAUSE;
      case GLFW_KEY_F1: return KeyboardKey::F1;
      case GLFW_KEY_F2: return KeyboardKey::F2;
      case GLFW_KEY_F3: return KeyboardKey::F3;
      case GLFW_KEY_F4: return KeyboardKey::F4;
      case GLFW_KEY_F5: return KeyboardKey::F5;
      case GLFW_KEY_F6: return KeyboardKey::F6;
      case GLFW_KEY_F7: return KeyboardKey::F7;
      case GLFW_KEY_F8: return KeyboardKey::F8;
      case GLFW_KEY_F9: return KeyboardKey::F9;
      case GLFW_KEY_F10: return KeyboardKey::F10;
      case GLFW_KEY_F11: return KeyboardKey::F11;
      case GLFW_KEY_F12: return KeyboardKey::F12;
      case GLFW_KEY_F13: return KeyboardKey::F13;
      case GLFW_KEY_F14: return KeyboardKey::F14;
      case GLFW_KEY_F15: return KeyboardKey::F15;
      case GLFW_KEY_F16: return KeyboardKey::F16;
      case GLFW_KEY_F17: return KeyboardKey::F17;
      case GLFW_KEY_F18: return KeyboardKey::F18;
      case GLFW_KEY_F19: return KeyboardKey::F19;
      case GLFW_KEY_F20: return KeyboardKey::F20;
      case GLFW_KEY_F21: return KeyboardKey::F21;
      case GLFW_KEY_F22: return KeyboardKey::F22;
      case GLFW_KEY_F23: return KeyboardKey::F23;
      case GLFW_KEY_F24: return KeyboardKey::F24;
      case GLFW_KEY_KP_0: return KeyboardKey::KEYPAD_0;
      case GLFW_KEY_KP_1: return KeyboardKey::KEYPAD_1;
      case GLFW_KEY_KP_2: return KeyboardKey::KEYPAD_2;
      case GLFW_KEY_KP_3: return KeyboardKey::KEYPAD_3;
      case GLFW_KEY_KP_4: return KeyboardKey::KEYPAD_4;
      case GLFW_KEY_KP_5: return KeyboardKey::KEYPAD_5;
      case GLFW_KEY_KP_6: return KeyboardKey::KEYPAD_6;
      case GLFW_KEY_KP_7: return KeyboardKey::KEYPAD_7;
      case GLFW_KEY_KP_8: return KeyboardKey::KEYPAD_8;
      case GLFW_KEY_KP_9: return KeyboardKey::KEYPAD_9;
      case GLFW_KEY_KP_DECIMAL: return KeyboardKey::KEYPAD_DECIMAL;
      case GLFW_KEY_KP_DIVIDE: return KeyboardKey::KEYPAD_DIVIDE;
      case GLFW_KEY_KP_MULTIPLY: return KeyboardKey::KEYPAD_MULTIPLY;
      case GLFW_KEY_KP_SUBTRACT: return KeyboardKey::KEYPAD_SUBTRACT;
      case GLFW_KEY_KP_ADD: return KeyboardKey::KEYPAD_ADD;
      case GLFW_KEY_KP_ENTER: return KeyboardKey::KEYPAD_ENTER;
      case GLFW_KEY_KP_EQUAL: return KeyboardKey::KEYPAD_EQUAL;
      case GLFW_KEY_LEFT_SHIFT: return KeyboardKey::LEFT_SHIFT;
      case GLFW_KEY_LEFT_CONTROL: return KeyboardKey::LEFT_CONTROL;
      case GLFW_KEY_LEFT_ALT: return KeyboardKey::LEFT_ALT;
      case GLFW_KEY_LEFT_SUPER: return KeyboardKey::LEFT_SUPER;
      case GLFW_KEY_RIGHT_SHIFT: return KeyboardKey::RIGHT_SHIFT;
      case GLFW_KEY_RIGHT_CONTROL: return KeyboardKey::RIGHT_CONTROL;
      case GLFW_KEY_RIGHT_ALT: return KeyboardKey::RIGHT_ALT;
      case GLFW_KEY_RIGHT_SUPER: return KeyboardKey::RIGHT_SUPER;
      case GLFW_KEY_MENU: return KeyboardKey::MENU;
      default: return KeyboardKey::UNKNOWN;
      }
    }

    static inline auto get_modifier_flags(i32 modifiers) -> InputModifierFlags
    {
      InputModifierFlags flags;
      if ((modifiers & GLFW_MOD_ALT) != 0)
      {
        flags.set(InputModifier::ALT);
      }
      if ((modifiers & GLFW_MOD_CONTROL) != 0)
      {
        flags.set(InputModifier::CTRL);
      }
      if ((modifiers & GLFW_MOD_SHIFT) != 0)
      {
        flags.set(InputModifier::SHIFT);
      }
      return flags;
    }

    static auto fix_glfw_modifiers(i32 modifiers, i32 key, i32 action) -> i32
    {
      i32 bit = 0;
      if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
      {
        bit = GLFW_MOD_SHIFT;
      }
      if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
      {
        bit = GLFW_MOD_CONTROL;
      }
      if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
      {
        bit = GLFW_MOD_ALT;
      }
      return (action == GLFW_RELEASE) ? modifiers & (~bit) : modifiers | bit;
    }

    static inline auto calc_mouse_pos(f64 x_pos, f64 y_pos, vec2f32 mouse_scale) -> vec2f32
    {
      const auto pos_f32 = vec2f32(f32(x_pos), f32(y_pos));
      return pos_f32 * mouse_scale;
    }

    static inline auto try_get_keyboard_event(i32 key, i32 action, i32 modifiers)
      -> Option<KeyboardEvent>
    {
      if (key == GLFW_KEY_UNKNOWN)
      {
        return nilopt;
      }

      modifiers = fix_glfw_modifiers(modifiers, key, action);

      const auto get_event_type = [](i32 action)
      {
        switch (action)
        {
        case GLFW_RELEASE: return KeyboardEvent::Type::KEY_RELEASED;
        case GLFW_PRESS: return KeyboardEvent::Type::KEY_PRESSED;
        case GLFW_REPEAT: return KeyboardEvent::Type::KEY_REPEATED;
        default: unreachable(); break;
        }
      };
      const auto keyboard_event = KeyboardEvent{
        .type = get_event_type(action),
        .key  = glfw_to_soul_key(key),
        .mods = get_modifier_flags(modifiers),
      };
      return Option<KeyboardEvent>::Some(keyboard_event);
    }
  };

  static std::atomic<size_t> s_window_count; // NOLINT

  Window::Window(OwnRef<Desc> desc, Callbacks* callbacks)
      : desc_(std::move(desc)),
        mouse_scale_(1.0f / (f32)desc_.width, 1.0f / (f32)desc_.height),
        callbacks_(callbacks)
  {
    // Set error callback
    glfwSetErrorCallback(ApiCallbacks::error_callback);

    // Init GLFW when first window is created.
    if (s_window_count.fetch_add(1) == 0)
    {
      if (glfwInit() == GLFW_FALSE)
      {
        SOUL_PANIC("Failed to initialize GLFW");
      }
    }

    // Create the window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    u32 width  = desc_.width;
    u32 height = desc_.height;

    if (desc_.mode == WindowMode::FULLSCREEN)
    {
      glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
      auto mon = glfwGetPrimaryMonitor();
      auto mod = glfwGetVideoMode(mon);
      width    = mod->width;
      height   = mod->height;
    } else if (desc_.mode == WindowMode::MAXIMIZED)
    {
      glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
      auto mon = glfwGetPrimaryMonitor();
      auto mod = glfwGetVideoMode(mon);
      width    = mod->width;
      height   = mod->height;
    } else if (desc_.mode == WindowMode::MINIMIZED)
    {
      // Start with window being invisible
      glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
      glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
      glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
    }

    if (!desc_.resizable_window)
    {
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    glfw_window_ =
      glfwCreateWindow(cast<i32>(width), cast<i32>(height), desc_.title.c_str(), nullptr, nullptr);
    if (glfw_window_ == nullptr)
    {
      SOUL_PANIC("Failed to create GLFW window");
    }

    wsi_.set_window(glfw_window_);

    update_window_size();

    glfwSetWindowUserPointer(glfw_window_, this);

    // Set callbacks
    glfwSetWindowSizeCallback(glfw_window_, ApiCallbacks::window_size_callback);
    glfwSetKeyCallback(glfw_window_, ApiCallbacks::keyboard_callback);
    glfwSetMouseButtonCallback(glfw_window_, ApiCallbacks::mouse_button_callback);
    glfwSetCursorPosCallback(glfw_window_, ApiCallbacks::mouse_move_callback);
    glfwSetScrollCallback(glfw_window_, ApiCallbacks::mouse_wheel_callback);
    glfwSetCharCallback(glfw_window_, ApiCallbacks::char_input_callback);
    glfwSetDropCallback(glfw_window_, ApiCallbacks::dropped_file_callback);

    if (desc_.mode == WindowMode::MINIMIZED)
    {
      // Iconify and show window to make it available if user clicks on it
      glfwIconifyWindow(glfw_window_);
      glfwShowWindow(glfw_window_);
    } else
    {
      glfwShowWindow(glfw_window_);
      glfwFocusWindow(glfw_window_);
    }
  }

  Window::~Window()
  {
    glfwDestroyWindow(glfw_window_);

    // Shutdown GLFW when last window is destroyed.
    if (s_window_count.fetch_sub(1) == 1)
    {
      glfwTerminate();
    }
  }

  void Window::update_window_size()
  {
    // Actual window size may be clamped to slightly lower than monitor resolution
    i32 width  = 0;
    i32 height = 0;
    glfwGetWindowSize(glfw_window_, &width, &height);
    set_window_size(width, height);
  }

  void Window::set_window_size(u32 width, u32 height)
  {
    SOUL_ASSERT(0, width > 0 && height > 0);

    desc_.width    = width;
    desc_.height   = height;
    mouse_scale_.x = 1.0f / (f32)desc_.width;
    mouse_scale_.y = 1.0f / (f32)desc_.height;
  }

  void Window::shutdown()
  {
    glfwSetWindowShouldClose(glfw_window_, 1);
  }

  auto Window::should_close() const -> b8
  {
    return glfwWindowShouldClose(glfw_window_) != 0;
  }

  void Window::resize(u32 width, u32 height)
  {
    if (width == 0 || height == 0)
    {
      glfwWaitEvents();
      return;
    }
    glfwSetWindowSize(glfw_window_, cast<i32>(width), cast<i32>(height));

    // In minimized mode GLFW reports incorrect window size
    if (desc_.mode == WindowMode::MINIMIZED)
    {
      set_window_size(width, height);
    } else
    {
      update_window_size();
    }

    callbacks_->handle_window_size_change();
  }

  void Window::msg_loop()
  {
    callbacks_->handle_window_size_change();

    while (!should_close())
    {
      poll_for_events();
      callbacks_->handle_render_frame();
    }
  }

  void Window::set_window_pos(i32 x, i32 y)
  {
    glfwSetWindowPos(glfw_window_, x, y);
  }

  void Window::set_window_title(StringView title)
  {
    runtime::ScopeAllocator scope_allocator(
      "soul::app::set_window_title"_str, runtime::get_temp_allocator());
    String cstr_buffer     = String(&scope_allocator);
    const char* title_cstr = str::into_c_str(title, &cstr_buffer);
    glfwSetWindowTitle(glfw_window_, title_cstr);
  }

  void Window::set_window_icon(const Path& path) {}

  auto Window::wsi_ref() -> gpu::WSI&
  {
    return wsi_;
  }

  void Window::poll_for_events()
  {
    glfwPollEvents();
  }

} // namespace soul::app
