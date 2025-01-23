#pragma once

#include "core/path.h"
#include "core/sbo_vector.h"
#include "core/string_view.h"

using namespace soul;
struct lua_State;

namespace khaos
{
  class Store;

  class ScriptSystem
  {
  public:
    ScriptSystem();
    ~ScriptSystem();

    ScriptSystem(const ScriptSystem&)                    = delete;
    ScriptSystem(ScriptSystem&&)                         = delete;
    auto operator=(const ScriptSystem&) -> ScriptSystem& = delete;
    auto operator=(ScriptSystem&&) -> ScriptSystem&      = delete;

    void init(const Path& path, Store* store);
    void on_user_text_input(StringView user_input);
    void on_textgen_response(StringView textgen_response);
    void on_new_journey();
    auto require_path_cspan() -> Span<const String*>;

    // auto messages_refs() -> MessageRefs;

  private:
    MaybeNull<lua_State*> lua_state_;
    SBOVector<String> require_paths_;
  };
} // namespace khaos
