#pragma once

#include "core/type.h"
#include "lua_util.h"

using namespace soul;

namespace khaos
{
  auto textgen_push_message(lua_State* lua_state) -> int32_t;
  auto textgen_get_message(lua_State* lua_state) -> int32_t;
  auto textgen_open(lua_State* lua_state) -> int32_t;

} // namespace khaos
