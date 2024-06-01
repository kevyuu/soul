#pragma once

#include "core/builtins.h"
#include "core/flag_set.h"

namespace soul::app
{

  enum class MouseButton : u8
  {
    LEFT,
    MIDDLE,
    RIGHT,
    COUNT
  };
  using MouseButtonFlags = FlagSet<MouseButton>;

  enum class InputModifier : u8
  {
    SHIFT,
    CTRL,
    ALT,
    COUNT
  };
  using InputModifierFlags = FlagSet<InputModifier>;

  enum class KeyboardKey : u32
  {
    SPACE         = ' ',
    APOSTROPHE    = '\'',
    COMMA         = ',',
    MINUS         = '-',
    PERIOD        = '.',
    SLASH         = '/',
    KEY_0         = '0',
    KEY_1         = '1',
    KEY_2         = '2',
    KEY_3         = '3',
    KEY_4         = '4',
    KEY_5         = '5',
    KEY_6         = '6',
    KEY_7         = '7',
    KEY_8         = '8',
    KEY_9         = '9',
    SEMICOLON     = ';',
    EQUAL         = '=',
    A             = 'A',
    B             = 'B',
    C             = 'C',
    D             = 'D',
    E             = 'E',
    F             = 'F',
    G             = 'G',
    H             = 'H',
    I             = 'I',
    J             = 'J',
    K             = 'K',
    L             = 'L',
    M             = 'M',
    N             = 'N',
    O             = 'O',
    P             = 'P',
    Q             = 'Q',
    R             = 'R',
    S             = 'S',
    T             = 'T',
    U             = 'U',
    V             = 'V',
    W             = 'W',
    X             = 'X',
    Y             = 'Y',
    Z             = 'Z',
    LEFT_BRACKET  = '[',
    BACKSLASH     = '\\',
    RIGHT_BRACKET = ']',
    GRAVE_ACCENT  = '`',

    // SPECIAL KEYS START AT KEY CODE 256.
    ESCAPE = 256,
    TAB,
    ENTER,
    BACKSPACE,
    INSERT,
    DEL,
    RIGHT,
    LEFT,
    DOWN,
    UP,
    PAGE_UP,
    PAGE_DOWN,
    HOME,
    END,
    CAPS_LOCK,
    SCROLL_LOCK,
    NUM_LOCK,
    PRINT_SCREEN,
    PAUSE,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,
    KEYPAD_0,
    KEYPAD_1,
    KEYPAD_2,
    KEYPAD_3,
    KEYPAD_4,
    KEYPAD_5,
    KEYPAD_6,
    KEYPAD_7,
    KEYPAD_8,
    KEYPAD_9,
    KEYPAD_DECIMAL,
    KEYPAD_DIVIDE,
    KEYPAD_MULTIPLY,
    KEYPAD_SUBTRACT,
    KEYPAD_ADD,
    KEYPAD_ENTER,
    KEYPAD_EQUAL,
    LEFT_SHIFT,
    LEFT_CONTROL,
    LEFT_ALT,
    LEFT_SUPER, // WINDOWS KEY ON WINDOWS
    RIGHT_SHIFT,
    RIGHT_CONTROL,
    RIGHT_ALT,
    RIGHT_SUPER, // WINDOWS KEY ON WINDOWS
    MENU,
    UNKNOWN, // ANY UNKNOWN KEY CODE
    COUNT,
  };
  using KeyboardKeyFlags = FlagSet<KeyboardKey>;

  struct MouseEvent
  {
    enum class Type
    {
      BUTTON_DOWN, ///< MOUSE BUTTON WAS PRESSED
      BUTTON_UP,   ///< MOUSE BUTTON WAS RELEASED
      MOVE,        ///< MOUSE CURSOR POSITION CHANGED
      WHEEL,       ///< MOUSE WHEEL WAS SCROLLED
      COUNT,
    };

    Type type; ///< Event Type.
    vec2f32
      pos; ///< Normalized coordinates x,y in range [0, 1]. (0,0) is the top-left corner of the
    vec2f32 screen_pos;  ///< Screen-space coordinates in range [0, clientSize]. (0,0) is the
                         ///< top-left corner of the window.
    vec2f32 wheel_delta; ///< If the current event is CMouseEvent#Type#Wheel, the change in wheel
                         ///< scroll. Otherwise zero.
    InputModifierFlags
      mods; ///< Keyboard modifier flags. Only valid if the event Type is one the button events
    MouseButton
      button; ///< Which button was active. Only valid if the event Type is ButtonDown or ButtonUp.
  };

  struct KeyboardEvent
  {
    enum class Type
    {
      KEY_PRESSED,  ///< KEY WAS PRESSED.
      KEY_RELEASED, ///< KEY WAS RELEASED.
      KEY_REPEATED, ///< KEY IS REPEATEDLY DOWN.
      INPUT         ///< CHARACTER INPUT
    };

    Type type;               ///< The event type
    KeyboardKey key;         ///< The last key that was pressed/released
    InputModifierFlags mods; ///< Keyboard modifier flags
    u32 codepoint = 0;       ///< UTF-32 codepoint from GLFW for Input event types
  };

  class InputState
  {
  public:
    [[nodiscard]]
    auto is_mouse_moving() const -> b8
    {
      return is_mouse_moving_;
    }

    [[nodiscard]]
    auto is_key_down(KeyboardKey key) const -> b8
    {
      return current_key_flags_[key];
    }

    [[nodiscard]]
    auto is_key_pressed(KeyboardKey key) const -> b8
    {
      return current_key_flags_[key] && !previous_key_flags_[key];
    }

    [[nodiscard]]
    auto is_key_released(KeyboardKey key) const -> b8
    {
      return !current_key_flags_[key] && previous_key_flags_[key];
    }

    [[nodiscard]]
    auto is_mouse_button_down(MouseButton mouse_button) const -> b8
    {
      return current_mouse_flags_[mouse_button];
    }

    [[nodiscard]]
    auto is_mouse_button_clicked(MouseButton mouse_button) const -> b8
    {
      return current_mouse_flags_[mouse_button] && !previous_mouse_flags_[mouse_button];
    }

    [[nodiscard]]
    auto is_mouse_button_released(MouseButton mouse_button) const -> b8
    {
      return !current_mouse_flags_[mouse_button] && previous_mouse_flags_[mouse_button];
    }

    [[nodiscard]]
    auto is_input_modifier_down(InputModifier mod) const -> b8;

    [[nodiscard]]
    auto is_input_modifier_pressed(InputModifier mod) const -> b8;

    [[nodiscard]]
    auto is_input_modifier_released(InputModifier mod) const -> b8;

  private:
    [[nodiscard]]
    auto get_input_modifier_state(const KeyboardKeyFlags& states, InputModifier mod) const -> b8;

    KeyboardKeyFlags current_key_flags_;
    KeyboardKeyFlags previous_key_flags_;
    MouseButtonFlags current_mouse_flags_;
    MouseButtonFlags previous_mouse_flags_;

    b8 is_mouse_moving_ = false;

    friend class App;
  };
} // namespace soul::app
