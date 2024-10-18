#pragma once

#include "core/path.h"
#include "core/string_view.h"
#include "core/vector.h"

#include "gpu/id.h"

#include "text_completion_system.h"
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

    [[nodiscard]]
    auto api_url_string_view() const -> StringView;

    void set_api_url(StringView api_url);

    [[nodiscard]]
    auto get_context_token_count() const -> u32;

    void set_context_token_count(u32 token_count);

    [[nodiscard]]
    auto get_response_token_count() const -> u32;

    void set_response_token_count(u32 token_count);

    [[nodiscard]]
    auto prompt_formats_cspan() const -> Span<const PromptFormat*>;

    [[nodiscard]]
    auto active_prompt_format_cref() const -> const PromptFormat&;

    [[nodiscard]]
    auto active_prompt_format_index() const -> u32;

    void select_prompt_format(StringView name);

    void select_prompt_format(u32 index);

    void update_prompt_format(const PromptFormat& setting);

    void create_prompt_format(const PromptFormat& setting);

    void delete_prompt_format();

    [[nodiscard]]
    auto samplers_cspan() const -> Span<const Sampler*>;

    [[nodiscard]]
    auto active_sampler_cref() const -> const Sampler&;

    [[nodiscard]]
    auto active_sampler_index() const -> u32;

    void select_sampler(StringView name);

    void select_sampler(u32 index);

    void update_sampler(const Sampler& setting);

    void create_sampler(const Sampler& setting);

    void delete_sampler();

    [[nodiscard]]
    auto impersonate_action_prompt_cspan() -> StringView;

    void set_impersonate_action_prompt(StringView prompt);

    [[nodiscard]]
    auto choice_prompt_cspan() -> StringView;

    void set_choice_prompt(StringView prompt);

    [[nodiscard]]
    auto header_prompt_cspan() const -> StringView;

    void set_header_prompt(StringView header_prompt);

    [[nodiscard]]
    auto first_message_cspan() const -> StringView;

    void set_first_message(StringView first_message);

    [[nodiscard]]
    auto is_any_project_active() const -> b8;

    auto project_metadatas_cspan() const -> Span<const ProjectMetadata*>;

    void create_new_project(StringView name, const Path& path);

    void import_new_project(const Path& path);

    void load_project(const Path& path);

    void save_project();

    [[nodiscard]]
    auto is_any_journey_active() const -> b8;

    void create_new_journey();

    auto active_journey_cref() const -> const Journey&;

    void add_message(Role role, StringView label);

    void run_task_completion();

    void save_app_settings();

    [[nodiscard]]
    auto has_active_completion_task() const -> b8;

    gpu::TextureID background_texture_id;

    void on_new_frame();

    [[nodiscard]]
    auto get_game_state() const -> GameState;

  private:
    Path storage_path_;
    Path app_setting_path_;
    Path prompt_format_path_;
    Path sampler_path_;
    Option<Project> active_project_;
    Path active_project_path_;
    Path active_project_filepath_ = Path::From(""_str);
    Vector<PromptFormat> prompt_formats_;
    Vector<Sampler> samplers_;

    AppSetting app_setting_;
    u32 active_prompt_format_index_;
    u32 active_sampler_index_;

    Option<Journey> active_journey_;
    GameState game_state_ = GameState::WAITING_USER_RESPONSE;
    TextCompletionSystem text_completion_system_;

    void sort_format_settings();
    void save_setting_to_file(const PromptFormat& setting);
    void sort_sampler_settings();
    void save_setting_to_file(const Sampler& setting);
    auto load_texture(const Path& path) -> gpu::TextureID;
    void load_background();

    NotNull<gpu::System*> gpu_system_;
  };

} // namespace khaos
