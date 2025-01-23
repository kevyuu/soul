#pragma once

extern "C"
{
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include "core/comp_str.h"
#include "core/string.h"
#include "core/string_view.h"
#include "core/type.h"

using namespace soul;

namespace khaos
{
  class Store;
}

namespace khaos
{
  inline auto lua_insist_global(lua_State* lua_state, CompStr k) -> int32_t
  {
    lua_getglobal(lua_state, k.c_str());

    if (!lua_istable(lua_state, -1))
    {
      lua_pop(lua_state, 1); // Pop the non-table.
      lua_newtable(lua_state);
      lua_pushvalue(lua_state, -1);
      lua_setglobal(lua_state, k.c_str());
    }
    return 1;
  }

  inline auto lua_preload(lua_State* L, Store* store, lua_CFunction f, CompStr name) -> int32_t
  {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushlightuserdata(L, store);
    lua_pushcclosure(L, f, 1);
    lua_setfield(L, -2, name.c_str());
    lua_pop(L, 2);
    return 0;
  }

  inline auto lua_to_string_view(lua_State* lua_state, i32 index) -> StringView
  {
    usize len            = 0;
    const char* str_data = lua_tolstring(lua_state, index, &len);
    return StringView{str_data, len};
  }

  inline auto lua_to_string(
    lua_State* lua_state,
    i32 index,
    NotNull<memory::Allocator*> allocator = get_default_allocator()) -> String
  {
    usize len            = 0;
    const char* str_data = lua_tolstring(lua_state, index, &len);
    return String::From(str_data, allocator);
  }

  inline void lua_push_string_view(lua_State* lua_state, StringView str_view)
  {
    lua_pushlstring(lua_state, str_view.begin(), cast<i32>(str_view.size()));
  }
} // namespace khaos
