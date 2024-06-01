#include "input_state.h"

#include "core/compiler.h"

namespace soul::app
{
  auto InputState::is_input_modifier_down(InputModifier mod) const -> b8
  {
    return get_input_modifier_state(current_key_flags_, mod);
  }

  auto InputState::is_input_modifier_pressed(InputModifier mod) const -> b8
  {
    return get_input_modifier_state(current_key_flags_, mod) == true &&
           get_input_modifier_state(previous_key_flags_, mod) == false;
  }

  auto InputState::is_input_modifier_released(InputModifier mod) const -> b8
  {
    return get_input_modifier_state(current_key_flags_, mod) == false &&
           get_input_modifier_state(previous_key_flags_, mod) == true;
  }

  auto InputState::get_input_modifier_state(const KeyboardKeyFlags& states, InputModifier mod) const
    -> b8
  {
    switch (mod)
    {
    case InputModifier::SHIFT:
      return states[KeyboardKey::LEFT_SHIFT] || states[KeyboardKey::RIGHT_SHIFT];
    case InputModifier::CTRL:
      return states[KeyboardKey::LEFT_CONTROL] || states[KeyboardKey::RIGHT_CONTROL];
    case InputModifier::ALT: return states[KeyboardKey::LEFT_ALT] || states[KeyboardKey::RIGHT_ALT];
    default: unreachable(); return false;
    }
  }
} // namespace soul::app
