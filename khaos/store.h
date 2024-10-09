#pragma once

#include "core/path.h"
#include "core/string_view.h"
#include "core/vector.h"
#include "type.h"

using namespace soul;

namespace khaos
{
  class Store
  {
  public:
    explicit Store(const Path& storage_path);

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
    auto is_any_project_active() const -> b8;

  private:
    Path storage_path_;
    Path app_setting_path_;
    Path prompt_format_path_;
    Path sampler_path_;
    Vector<ProjectMetadata> project_metadatas_;
    Option<ProjectMetadata> active_project_;
    Vector<PromptFormat> prompt_formats_;
    Vector<Sampler> samplers_;

    String api_url_;
    u32 context_token_count_;
    u32 response_token_count_;
    u32 active_prompt_format_index_;
    u32 active_sampler_index_;

    void sort_format_settings();
    void save_setting_to_file(const PromptFormat& setting);
    void sort_sampler_settings();
    void save_setting_to_file(const Sampler& setting);
  };

} // namespace khaos
