#include "lua_util.h"

#include "store/store.h"
#include "type.h"

#include "core/log.h"
#include "core/string.h"

static const char textgen_lua[] =
#include "textgen.lua"
  ;

namespace khaos
{
  namespace
  {
    auto lua_to_message(lua_State* lua_state, i32 index) -> Message
    {
      SOUL_ASSERT(0, lua_istable(lua_state, index));
      lua_pushvalue(lua_state, index);
      lua_getfield(lua_state, -1, "role");
      lua_getfield(lua_state, -2, "label");
      lua_getfield(lua_state, -3, "content");
      lua_getfield(lua_state, -4, "is_visible_to_message");

      auto message = Message{
        .role    = static_cast<Role>(lua_tointeger(lua_state, -4)),
        .label   = String::From(lua_to_string_view(lua_state, -3)),
        .content = String::From(lua_to_string_view(lua_state, -2)),
      };

      lua_pop(lua_state, 5);

      return message;
    }
  } // namespace

  auto textgen_push_message(lua_State* lua_state) -> i32
  {
    Store* store = static_cast<Store*>(lua_touserdata(lua_state, lua_upvalueindex(1)));
    store->push_message(lua_to_message(lua_state, -1));
    return 0;
  }

  auto textgen_set_message(lua_State* lua_state) -> i32
  {
    constexpr auto number_of_return_values = 0;

    Store* store = static_cast<Store*>(lua_touserdata(lua_state, lua_upvalueindex(1)));
    const i32 lua_idx = lua_tointeger(lua_state, -2);

    if (lua_idx == 0)
    {
      return number_of_return_values;
    }

    const auto messages_count = store->active_journey_cref().messages.size();
    const u64 cpp_idx = [lua_idx, messages_count]() -> u64
    {
      if (lua_idx < 0)
      {
        return messages_count + lua_idx; 
      } else
      {
        return lua_idx - 1;
      }
      
    }();
    store->set_message(cpp_idx, lua_to_message(lua_state, -1));

    return number_of_return_values;
  }

  auto textgen_get_message_count(lua_State* lua_state) -> i32
  {
    Store* store = static_cast<Store*>(lua_touserdata(lua_state, lua_upvalueindex(1)));
    lua_pushnumber(lua_state, store->active_journey_cref().messages.size());
    return 1;
  }

  auto textgen_continue(lua_State* lua_state) -> int32_t
  {
    Store* store = static_cast<Store*>(lua_touserdata(lua_state, lua_upvalueindex(1)));

    const auto header_prompt = lua_to_string_view(lua_state, -3);

    const auto grammar_string = lua_to_string_view(lua_state, -2); 

    lua_pushvalue(lua_state, -1);
    const i32 callback_idx    = luaL_ref(lua_state, LUA_REGISTRYINDEX);
    lua_pop(lua_state, 1);

    const auto& chatbot_setting = store->app_setting_cref().chatbot_setting;
    const auto& messages = store->active_journey_cref().messages;
    auto& textgen_system = store->textgen_system_ref();

    textgen_system.push_task(TextgenTask{
      .header_prompt = String::From(header_prompt), 
      .messages      = messages.clone(),
      .api_url       = chatbot_setting.api_url.clone(),
      .prompt_format_parameter = store->active_prompt_format_cref().parameter.clone(),
      .sampler_parameter       = store->active_sampler_cref().parameter.clone(),
      .grammar_string          = String::From(grammar_string),
      .max_token_count = chatbot_setting.response_token_count,
      .tokenizer_type = chatbot_setting.tokenizer_type,
      .callback                = [lua_state, callback_idx](StringView str_view)
      {
        lua_rawgeti(lua_state, LUA_REGISTRYINDEX, callback_idx); 
        lua_push_string_view(lua_state, str_view);
        lua_pcall(lua_state, 1, 0, 0);
        luaL_unref(lua_state, LUA_REGISTRYINDEX, callback_idx);
      },
    });

    return 0;
  }

  auto textgen_open(lua_State* lua_state) -> int32_t
  {
    Store* store = reinterpret_cast<Store*>(lua_touserdata(lua_state, lua_upvalueindex(1)));

    luaL_dostring(lua_state, textgen_lua);

    lua_pushlightuserdata(lua_state, store);
    lua_pushcclosure(lua_state, textgen_push_message, 1);
    lua_setfield(lua_state, -2, "push_message");

    lua_pushlightuserdata(lua_state, store);
    lua_pushcclosure(lua_state, textgen_set_message, 1);
    lua_setfield(lua_state, -2, "set_message");

    lua_pushlightuserdata(lua_state, store);
    lua_pushcclosure(lua_state, textgen_get_message_count, 1);
    lua_setfield(lua_state, -2, "get_message_count");

    lua_pushlightuserdata(lua_state, store);
    lua_pushcclosure(lua_state, textgen_continue, 1);
    lua_setfield(lua_state, -2, "continue");

    return 1;
  }
} // namespace khaos
