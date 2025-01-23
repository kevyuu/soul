#pragma once

#include "core/path.h"
#include "core/string_view.h"
#include "core/vector.h"

#include "gpu/id.h"

#include "script_system.h"
#include "textgen_system.h"
#include "type.h"

using namespace soul;

namespace soul::gpu
{
  class System;
}

namespace khaos
{
  class Store
  {
  public:
    explicit Store(const Path& storage_path, NotNull<gpu::System*> gpu_system);

    auto app_setting_ref() -> AppSetting&;
    auto app_setting_cref() const -> const AppSetting&;
    void set_chatbot_api_url(StringView api_url);
    void set_chatbot_context_token_count(u32 token_count);
    void set_chatbot_response_token_count(u32 token_count);
    void set_chatbot_tokenizer_type(TokenizerType tokenizer_type);

    auto prompt_formats_cspan() const -> Span<const PromptFormat*>;
    auto active_prompt_format_cref() const -> const PromptFormat&;
    auto active_prompt_format_index() const -> u32;
    void select_prompt_format(u32 index);
    void update_prompt_format(const PromptFormat& setting);
    void create_prompt_format(const PromptFormat& setting);
    void delete_prompt_format();
    auto find_prompt_format_index(StringView name) -> Option<u32>;

    auto samplers_cspan() const -> Span<const Sampler*>;
    auto active_sampler_cref() const -> const Sampler&;
    auto active_sampler_ref() -> Sampler&;
    auto active_sampler_index() const -> u32;
    void select_sampler(u32 index);
    void update_sampler(const Sampler& setting);
    void create_sampler(const Sampler& setting);
    void delete_sampler();
    auto find_sampler_index(StringView name) -> Option<u32>;

    [[nodiscard]]
    auto is_any_project_active() const -> b8;

    auto project_metadatas_cspan() const -> Span<const ProjectMetadata*>;

    void import_new_project(const Path& path);

    void load_project(const Path& path);

    [[nodiscard]]
    auto is_any_journey_active() const -> b8;
    void create_new_journey();
    auto active_journey_cref() const -> const Journey&;

    auto message_cref(u64 idx) -> const Message&;
    void push_message(OwnRef<Message> message);
    void set_message(u64 idx, OwnRef<Message> message);
    auto get_message(u64 message_index) -> const Message&;

    void load_app_settings();
    void save_app_settings();

    void on_new_frame();

    auto textgen_system_ref() -> TextgenSystem&;
    auto script_system_ref() -> ScriptSystem&;

  private:
    Path storage_path_;
    Path app_setting_path_;
    Path prompt_format_path_;
    Path sampler_path_;

    Vector<PromptFormat> prompt_formats_;

    AppSetting app_setting_;

    Option<Project> active_project_;

    Option<Journey> active_journey_;
    GameState game_state_ = GameState::WAITING_USER_RESPONSE;
    TextgenSystem textgen_system_;
    ScriptSystem script_system_;

    void sort_format_settings();
    void save_setting_to_file(const PromptFormat& setting);
    void sort_sampler_settings();
    void save_setting_to_file(const Sampler& setting);
    auto load_texture(const Path& path) -> gpu::TextureID;

    NotNull<gpu::System*> gpu_system_;
  };

} // namespace khaos
