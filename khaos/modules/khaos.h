#pragma once

#include "core/log.h"
#include "core/type.h"

#include "lua_util.h"
#include "textgen.h"

using namespace soul;

namespace khaos
{
  auto lua_print(lua_State* lua_state) -> int32_t
  {
    SOUL_LOG_INFO("{}", lua_to_string_view(lua_state, -1));
    return 0;
  }

  auto luaopen_khaos(lua_State* lua_state) -> int32_t
  {
    lua_insist_global(lua_state, "khaos"_str);

    Store* store = reinterpret_cast<Store*>(lua_touserdata(lua_state, lua_upvalueindex(1)));
    lua_preload(lua_state, store, textgen_open, "khaos.textgen"_str);

    lua_pushcfunction(lua_state, lua_print);
    lua_setfield(lua_state, -2, "print");

    return 1;
  }

} // namespace khaos
