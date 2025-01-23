#include "script_system.h"
#include "core/panic_format.h"

#include "misc/filesystem.h"
#include "misc/string_util.h"

#include "core/log.h"
#include "store/store.h"
#include "type.h"

#include "modules/khaos.h"
#include "modules/lua_util.h"

namespace khaos
{
  namespace
  {
    auto loader(lua_State* lua_state) -> i32
    {
      SOUL_LOG_INFO("Loader");
      auto* system =
        reinterpret_cast<ScriptSystem*>(lua_touserdata(lua_state, lua_upvalueindex(1)));
      const auto module_name   = lua_to_string_view(lua_state, 1);
      const String module_path = str::replace_char(module_name, '.', '/');
      SOUL_LOG_INFO("module_path : {}", module_path.cview());

      for (const auto& require_path : system->require_path_cspan())
      {
        const auto actual_require_path = Path::From(
          str::replace_substr(require_path.cview(), "?"_str, module_path.cview()).cview());
        if (fs::exists(actual_require_path) && !fs::is_directory(actual_require_path))
        {
          const auto lua_code = fs::get_file_content(actual_require_path);
          const auto status =
            luaL_loadbuffer(lua_state, lua_code.data(), lua_code.size(), module_path.c_str());
          switch (status)
          {
          case LUA_ERRMEM:
            return luaL_error(
              lua_state, "Memory allocation error: %s\n", lua_tostring(lua_state, -1));
          case LUA_ERRSYNTAX:
            return luaL_error(lua_state, "Syntax error: %s\n", lua_tostring(lua_state, -1));
          default: // success
            return 1;
          }
        }
      }

      lua_pushfstring(lua_state, "'%s' not found in game directories", module_path.c_str());
      return 1;
    }

    i32 luax_arr_push_back(lua_State* lua_state, i32 tindex, i32 vindex)
    {
      if (tindex < 0)
      {
        tindex = lua_gettop(lua_state) + 1 + tindex;
      }
      if (vindex < 0)
      {
        vindex = lua_gettop(lua_state) + 1 + vindex;
      }

      lua_pushvalue(lua_state, vindex);
      lua_rawseti(lua_state, tindex, lua_objlen(lua_state, tindex) + 1);
      return 0;
    }

    i32 luax_register_searcher(lua_State* lua_state, lua_CFunction f, void* data = nullptr)
    {
      lua_getglobal(lua_state, "package");

      if (lua_isnil(lua_state, -1))
      {
        return luaL_error(lua_state, "can't register searcher: package table does not exist.");
      }

      lua_getfield(lua_state, -1, "loaders");

      // lua 5.2 renamed package.loaders to package.searchers.
      if (lua_isnil(lua_state, -1))
      {
        lua_pop(lua_state, 1);
        lua_getfield(lua_state, -1, "searchers");
      }

      if (lua_isnil(lua_state, -1))
      {
        return luaL_error(
          lua_state, "can't register searcher: package.loaders table does not exist.");
      }

      if (data != nullptr)
      {
        lua_pushlightuserdata(lua_state, data);
        lua_pushcclosure(lua_state, f, 1);
      } else
      {
        lua_pushcfunction(lua_state, f);
      }

      luax_arr_push_back(lua_state, -2, -1);

      lua_pop(lua_state, 3);

      SOUL_ASSERT(0, lua_gettop(lua_state) == 0);

      return 0;
    }

    i32 luax_print(lua_State* lua_state)
    {
      SOUL_LOG_INFO("{}", lua_to_string_view(lua_state, -1));
      return 0;
    }
  } // namespace

  

  ScriptSystem::ScriptSystem() {}

  ScriptSystem::~ScriptSystem()
  {
    if (!lua_state_.is_some())
    {
      lua_close(lua_state_.some_ref());
    }
  }

  void ScriptSystem::init(const Path& path, Store* store)
  {
    require_paths_.clear();
    require_paths_.push_back(String::Format("{}/?/init.lua", path.string()));
    require_paths_.push_back(String::Format("{}/?.lua", path.string()));

    lua_state_ = luaL_newstate();
    luaL_openlibs(lua_state_);

    SOUL_LOG_INFO("Init script system");

    SOUL_ASSERT(0, lua_gettop(lua_state_) == 0);

    lua_preload(lua_state_, store, luaopen_khaos, "khaos"_str);

    const Path& main_file = path / "main.lua"_str;

    SOUL_ASSERT(0, lua_gettop(lua_state_) == 0);
    luax_register_searcher(lua_state_, loader, this);

    SOUL_ASSERT(0, lua_gettop(lua_state_) == 0);
    const auto status = luaL_dofile(lua_state_, main_file.string().c_str());
    if (status)
    {
      SOUL_PANIC_FORMAT("Couldn't run project: {}", lua_tostring(lua_state_, -1));
    }

    lua_pushcfunction(lua_state_, luax_print);
    lua_setglobal(lua_state_, "lua_print");

    SOUL_ASSERT(0, lua_gettop(lua_state_) == 0);
  }

  void ScriptSystem::on_new_journey()
  {
    lua_getglobal(lua_state_, "khaos");
    lua_getfield(lua_state_, -1, "on_new_journey");
    SOUL_ASSERT(0, lua_isfunction(lua_state_, -1));
    const auto status = lua_pcall(lua_state_, 0, 0, 0);
    if (status)
    {
      SOUL_PANIC_FORMAT("Error running function : {}", lua_tostring(lua_state_, -1));
    }
  }

  void ScriptSystem::on_user_text_input(StringView user_input)
  {
    lua_getglobal(lua_state_, "khaos");
    lua_getfield(lua_state_, -1, "on_user_text_input");
    SOUL_ASSERT(0, lua_isfunction(lua_state_, -1));
    lua_push_string_view(lua_state_, user_input);
    const auto status = lua_pcall(lua_state_, 1, 0, 0);
    if (status)
    {
      SOUL_PANIC_FORMAT("Error running function : {}", lua_tostring(lua_state_, -1));
    }
  }

  void ScriptSystem::on_textgen_response(StringView textgen_response) {}

  auto ScriptSystem::require_path_cspan() -> Span<const String*>
  {
    return require_paths_.cspan();
  }
} // namespace khaos
