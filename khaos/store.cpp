#include "store.h"
#include "text_completion_system.h"
#include "type.h"

#include "core/array.h"

#include "gpu/gpu.h"

#include "misc/filesystem.h"
#include "misc/image_data.h"
#include "misc/json.h"

#include <algorithm>
#include <filesystem>

namespace khaos
{
  Store::Store(const Path& storage_path, NotNull<gpu::System*> gpu_system)
      : storage_path_(storage_path.clone()),
        app_setting_path_(storage_path / "app_setting.json"_str),
        prompt_format_path_(storage_path_ / "prompt_format_settings"_str),
        sampler_path_(storage_path_ / "sampler_settings"_str),
        active_project_path_(Path::From(""_str)),
        active_prompt_format_index_(0),
        active_sampler_index_(0),
        gpu_system_(gpu_system)
  {
    static const auto PROMPT_FORMAT_SETTINGS = Array{
      PromptFormat{
        .name = "Llama 3"_str,
        .parameter =
          {
            .header_prefix    = "<|begin_of_text|><|start_header_id|>system<|end_header_id|>"_str,
            .header_suffix    = "<|eot_id|>"_str,
            .user_prefix      = "<|start_header_id|>user<|end_header_id|>"_str,
            .user_suffix      = "<|eot_id|>"_str,
            .assistant_prefix = "<|start_header_id|>assistant<|end_header_id|>"_str,
            .assistant_suffix = "<|eot_id|>"_str,
            .system_prefix    = "<|start_header_id|>system<|end_header_id|>"_str,
            .system_suffix    = "<|eot_id|>"_str,
          },
      },
      PromptFormat{
        .name = "ChatML"_str,
        .parameter =
          {
            .header_prefix    = "<|im_start|>system"_str,
            .header_suffix    = "<|im_end|>"_str,
            .user_prefix      = "<|im_start|>user"_str,
            .user_suffix      = "<|im_end|>"_str,
            .assistant_prefix = "<|im_start|>assistant"_str,
            .assistant_suffix = "<|im_end|>"_str,
            .system_prefix    = "<|im_start|>system"_str,
            .system_suffix    = "<|im_end|>"_str,
          },
      },
    };

    static const auto SAMPLER_SETTINGS = Array{
      Sampler{
        .name = "Big O"_str,
        .parameter =
          {
            .temperature        = 0.87,
            .top_p              = 0.99,
            .top_k              = 85,
            .repetition_penalty = 1.01,
            .typical_p          = 0.68,
            .tfs                = 0.68,
          },
      },
      Sampler{
        .name = "Debug-deterministic"_str,
        .parameter =
          {
            .top_k     = 1,
            .do_sample = false,
          },
      },
      Sampler{
        .name = "Divine Intellect"_str,
        .parameter =
          {
            .temperature        = 1.31,
            .top_p              = 0.14,
            .top_k              = 49,
            .repetition_penalty = 1.17,
          },
      },
      Sampler{
        .name = "Midnight Enighma"_str,
        .parameter =
          {
            .temperature        = 0.98,
            .top_p              = 0.37,
            .top_k              = 100,
            .repetition_penalty = 1.18,
          },
      },
      Sampler{
        .name = "Shortwave"_str,
        .parameter =
          {
            .temperature        = 1.53,
            .top_p              = 0.64,
            .top_k              = 33,
            .repetition_penalty = 1.07,
          },
      },
      Sampler{
        .name = "simple-1"_str,
        .parameter =
          {
            .temperature        = 0.7,
            .top_p              = 0.9,
            .top_k              = 20,
            .repetition_penalty = 1.15,
          },
      },
      Sampler{
        .name = "Yara"_str,
        .parameter =
          {
            .temperature        = 0.82,
            .top_p              = 0.21,
            .top_k              = 72,
            .repetition_penalty = 1.19,
          },
      },
    };

    if (!std::filesystem::is_directory(prompt_format_path_))
    {
      std::filesystem::create_directory(prompt_format_path_);
      for (const auto& setting : PROMPT_FORMAT_SETTINGS)
      {
        save_setting_to_file(setting);
      }
    }

    for (const auto& entry : std::filesystem::directory_iterator(prompt_format_path_))
    {
      if (entry.is_regular_file() && entry.path().extension() == ".json")
      {
        String json_string = get_file_content(Path(entry.path()));
        prompt_formats_.push_back(from_json_string<PromptFormat>(json_string.cspan()));
      }
    }

    if (!std::filesystem::is_directory(sampler_path_))
    {
      std::filesystem::create_directory(sampler_path_);
      for (const auto& setting : SAMPLER_SETTINGS)
      {
        save_setting_to_file(setting);
      }
    }

    for (const auto& entry : std::filesystem::directory_iterator(sampler_path_))
    {
      if (entry.is_regular_file() && entry.path().extension() == ".json")
      {
        String json_string = get_file_content(Path(entry.path()));
        samplers_.push_back(from_json_string<Sampler>(json_string.cspan()));
      }
    }

    if (!std::filesystem::is_regular_file(app_setting_path_))
    {
      JsonDoc doc;
      const auto ref         = doc.create_root_object(AppSetting());
      const auto json_string = doc.dump();
      write_to_file(app_setting_path_, json_string.cspan());
    }

    {
      String app_json_string         = get_file_content(app_setting_path_);
      auto app_setting               = from_json_string<AppSetting>(app_json_string.cspan());
      app_setting_.project_metadatas = std::move(app_setting.project_metadatas);
      select_prompt_format(app_setting.active_prompt_format.cspan());
      select_sampler(app_setting.active_sampler.cspan());
    }
  }

  [[nodiscard]]
  auto Store::api_url_string_view() const -> StringView
  {
    return app_setting_.api_url.cspan();
  }

  void Store::set_api_url(StringView api_url)
  {
    app_setting_.api_url.assign(api_url);
  }

  auto Store::prompt_formats_cspan() const -> Span<const PromptFormat*>
  {
    return prompt_formats_.cspan();
  }

  auto Store::active_prompt_format_cref() const -> const PromptFormat&
  {
    return prompt_formats_[active_prompt_format_index_];
  }

  auto Store::active_prompt_format_index() const -> u32
  {
    return active_prompt_format_index_;
  }

  void Store::select_prompt_format(StringView name)
  {
    active_prompt_format_index_ = [this, name]() -> u32
    {
      for (u32 format_i = 0; format_i < prompt_formats_.size(); format_i++)
      {
        const auto& setting = prompt_formats_[format_i];
        if (setting.name.cspan() == name)
        {
          return format_i;
        }
      }
      unreachable();
      return 0;
    }();
    app_setting_.active_prompt_format.assign(name);
  }

  void Store::select_prompt_format(u32 index)
  {
    active_prompt_format_index_ = index;
    app_setting_.active_prompt_format.assign(
      prompt_formats_[active_prompt_format_index_].name.cspan());
  }

  void Store::update_prompt_format(const PromptFormat& setting)
  {
    if (active_prompt_format_cref().name != setting.name)
    {
      const auto old_filename = String::Format("{}.json", active_prompt_format_cref().name);
      delete_file(prompt_format_path_ / old_filename.cspan());
    }
    prompt_formats_[active_prompt_format_index_].clone_from(setting);
    save_setting_to_file(setting);
    sort_format_settings();
  }

  void Store::create_prompt_format(const PromptFormat& setting)
  {
    prompt_formats_.push_back(setting.clone());
    save_setting_to_file(setting);
    sort_format_settings();
  }

  void Store::delete_prompt_format()
  {
    const auto filename = String::Format("{}.json", active_prompt_format_cref().name);
    delete_file(prompt_format_path_ / filename.cspan());
    prompt_formats_.remove(active_prompt_format_index_);
    sort_format_settings();
  }

  void Store::sort_format_settings()
  {
    std::ranges::sort(
      prompt_formats_,
      [](const PromptFormat& val1, const PromptFormat& val2) -> b8
      {
        const auto& name1 = val1.name.cspan();
        const auto& name2 = val2.name.cspan();
        return std::lexicographical_compare(name1.begin(), name1.end(), name2.begin(), name2.end());
      });
  }

  void Store::save_setting_to_file(const PromptFormat& setting)
  {
    const auto filename = String::Format("{}.json", setting.name);
    JsonDoc doc;
    doc.create_root_object(setting);
    write_to_file(prompt_format_path_ / filename.cspan(), doc.dump().cspan());
  }

  auto Store::samplers_cspan() const -> Span<const Sampler*>
  {
    return samplers_.cspan();
  }

  auto Store::active_sampler_cref() const -> const Sampler&
  {
    return samplers_[active_sampler_index_];
  }

  auto Store::active_sampler_index() const -> u32
  {
    return active_sampler_index_;
  }

  void Store::select_sampler(StringView name)
  {
    active_sampler_index_ = [this, name]() -> u32
    {
      for (u32 format_i = 0; format_i < samplers_.size(); format_i++)
      {
        const auto& setting = samplers_[format_i];
        if (setting.name.cspan() == name)
        {
          return format_i;
        }
      }
      unreachable();
      return 0;
    }();
    app_setting_.active_sampler.assign(samplers_[active_sampler_index_].name.cspan());
  }

  void Store::select_sampler(u32 index)
  {
    active_sampler_index_ = index;
    app_setting_.active_sampler.assign(samplers_[active_sampler_index_].name.cspan());
  }

  void Store::update_sampler(const Sampler& setting)
  {
    if (active_sampler_cref().name != setting.name)
    {
      const auto old_filename = String::Format("{}.json", active_sampler_cref().name);
      delete_file(sampler_path_ / old_filename.cspan());
    }
    samplers_[active_sampler_index_].clone_from(setting);
    save_setting_to_file(setting);
    sort_sampler_settings();
  }

  void Store::create_sampler(const Sampler& setting)
  {
    samplers_.push_back(setting.clone());
    save_setting_to_file(setting);
    sort_sampler_settings();
  }

  void Store::delete_sampler()
  {
    const auto filename = String::Format("{}.json", active_sampler_cref().name);
    delete_file(sampler_path_ / filename.cspan());
    samplers_.remove(active_sampler_index_);
    sort_sampler_settings();
  }

  [[nodiscard]]
  auto Store::impersonate_action_prompt_cspan() -> StringView
  {
    return app_setting_.impersonate_action_prompt.cspan();
  }

  void Store::set_impersonate_action_prompt(StringView prompt)
  {
    app_setting_.impersonate_action_prompt.assign(prompt);
    save_app_settings();
  }

  [[nodiscard]]
  auto Store::choice_prompt_cspan() -> StringView
  {
    return app_setting_.choice_prompt.cspan();
  }

  void Store::set_choice_prompt(StringView prompt)
  {
    app_setting_.choice_prompt.assign(prompt);
    save_app_settings();
  }

  [[nodiscard]]
  auto Store::header_prompt_cspan() const -> StringView
  {
    return active_project_.some_ref().header_prompt.cspan();
  }

  void Store::set_header_prompt(StringView header_prompt)
  {
    active_project_.some_ref().header_prompt.assign(header_prompt);
    save_project();
  }

  [[nodiscard]]
  auto Store::first_message_cspan() const -> StringView
  {
    return active_project_.some_ref().first_message.content.cspan();
  }

  void Store::set_first_message(StringView first_message)
  {
    active_project_.some_ref().first_message.content.assign(first_message);
    save_project();
  }

  auto Store::is_any_project_active() const -> b8
  {
    return active_project_.is_some();
  }

  auto Store::active_journey_cref() const -> const Journey&
  {
    return active_journey_.some_ref();
  }

  auto Store::project_metadatas_cspan() const -> Span<const ProjectMetadata*>
  {
    return app_setting_.project_metadatas.cspan();
  }

  void Store::create_new_project(StringView name, const Path& path)
  {
    JsonDoc doc;
    const auto ref               = doc.create_root_object(Project());
    const String json_string     = doc.dump();
    const auto project_directory = path / name;
    const auto filename          = String::Format("{}.kosmos", name);
    app_setting_.project_metadatas.push_back(ProjectMetadata{
      .name = String::From(name),
      .path = project_directory / filename.cspan(),
    });
    std::filesystem::create_directory(project_directory);
    write_to_file(app_setting_.project_metadatas.back().path, json_string.cspan());
    save_app_settings();
    load_project(app_setting_.project_metadatas.back().path);
  }

  void Store::import_new_project(const Path& path)
  {
    app_setting_.project_metadatas.push_back(ProjectMetadata{
      .name = String::From(path.stem().string().data()),
      .path = path.clone(),
    });
    save_app_settings();
  }

  void Store::load_project(const Path& path)
  {
    const auto project_json_string = get_file_content(path);
    active_project_                = from_json_string<Project>(project_json_string.cspan());
    active_project_path_           = path.parent_path();
    active_project_filepath_       = path.clone();
    load_background();
  }

  void Store::save_project()
  {
    JsonDoc doc;
    doc.create_root_object(active_project_.some_ref());
    const auto project_json_string = doc.dump();
    write_to_file(active_project_filepath_, project_json_string.cspan());
  }

  [[nodiscard]]
  auto Store::is_any_journey_active() const -> b8
  {
    return active_journey_.is_some();
  }

  void Store::create_new_journey()
  {
    active_journey_ = Journey{
      .name      = "Journey"_str,
      .user_name = "Kevin"_str,
      .messages  = Vector<Message>(),
    };
    active_journey_.some_ref().messages.push_back(active_project_.some_ref().first_message.clone());
  }

  void Store::add_message(Role role, StringView label)
  {
    active_journey_.some_ref().messages.push_back(Message{
      .role    = role,
      .content = String::From(label),
    });
  }

  void Store::run_task_completion()
  {
    game_state_ = GameState::GENERATING_ASSISTANT_RESPONSE;
    active_journey_.some_ref().messages.push_back(
      Message{.role = Role::ASSISTANT, .content = String::From(""_str)});
    const auto messages = active_journey_cref().messages.cspan();
    SOUL_LOG_INFO(
      "Repetition penalty : {}, {}",
      active_sampler_cref().name,
      active_sampler_cref().parameter.repetition_penalty);
    text_completion_system_.run(TextCompletionTask{
      .api_url                 = app_setting_.api_url.clone(),
      .header_prompt           = active_project_.some_ref().header_prompt.clone(),
      .messages                = u64span(messages.begin(), messages.size() - 1),
      .prompt_format_parameter = active_prompt_format_cref().parameter.clone(),
      .sampler_parameter       = active_sampler_cref().parameter.clone(),
      .grammar_string          = String::From(""_str),
      .max_token_count         = app_setting_.response_token_count,
    });
  }

  auto Store::load_texture(const Path& path) -> gpu::TextureID
  {
    const auto& image_data          = ImageData::FromFile(path, 4);
    const gpu::TextureFormat format = [&]
    {
      if (image_data.channel_count() == 1)
      {
        return gpu::TextureFormat::R8;
      } else
      {
        SOUL_ASSERT(0, image_data.channel_count() == 4);
        return gpu::TextureFormat::RGBA8;
      }
    }();

    const auto usage        = gpu::TextureUsageFlags({gpu::TextureUsage::SAMPLED});
    const auto texture_desc = gpu::TextureDesc::d2(
      format,
      1,
      usage,
      {
        gpu::QueueType::GRAPHIC,
        gpu::QueueType::COMPUTE,
      },
      image_data.dimension());

    const gpu::TextureRegionUpdate region_load = {
      .subresource = {.layer_count = 1},
      .extent      = vec3u32(image_data.dimension(), 1),
    };

    const auto raw_data = image_data.cspan();

    const gpu::TextureLoadDesc load_desc = {
      .data            = raw_data.data(),
      .data_size       = raw_data.size_in_bytes(),
      .regions         = {&region_load, 1},
      .generate_mipmap = false,
    };

    const auto texture_id = gpu_system_->create_texture(""_str, texture_desc, load_desc);
    gpu_system_->flush_texture(texture_id, texture_desc.usage_flags);
    return texture_id;
  }

  void Store::load_background()
  {
    background_texture_id =
      load_texture(active_project_path_ / "backgrounds"_str / "tavern day.jpg"_str);
  }

  void Store::save_app_settings()
  {
    JsonDoc doc;
    JsonObjectRef ref      = doc.create_root_object(app_setting_);
    const auto json_string = doc.dump();
    write_to_file(app_setting_path_, json_string.cspan());
  }

  auto Store::has_active_completion_task() const -> b8
  {
    return text_completion_system_.is_any_pending_response();
  }

  void Store::on_new_frame()
  {
    if (game_state_ == GameState::GENERATING_ASSISTANT_RESPONSE)
    {
      if (text_completion_system_.is_any_pending_response())
      {
        auto& active_journey = active_journey_.some_ref();
        auto& messages       = active_journey.messages;
        text_completion_system_.consume(&messages.back().content);
      } else
      {
        game_state_ = GameState::WAITING_USER_RESPONSE;
      }
    }
  }

  auto Store::get_game_state() const -> GameState
  {
    return game_state_;
  }

  void Store::sort_sampler_settings()
  {
    std::ranges::sort(
      samplers_,
      [](const Sampler& val1, const Sampler& val2) -> b8
      {
        const auto& name1 = val1.name.cspan();
        const auto& name2 = val2.name.cspan();
        return std::lexicographical_compare(name1.begin(), name1.end(), name2.begin(), name2.end());
      });
  }

  void Store::save_setting_to_file(const Sampler& setting)
  {
    const auto filename = String::Format("{}.json", setting.name);
    JsonDoc doc;
    doc.create_root_object(setting);
    write_to_file(sampler_path_ / filename.cspan(), doc.dump().cspan());
  }

  auto Store::get_context_token_count() const -> u32
  {
    return app_setting_.context_token_count;
  }

  void Store::set_context_token_count(u32 token_count)
  {
    app_setting_.context_token_count = token_count;
  }

  auto Store::get_response_token_count() const -> u32
  {
    return app_setting_.response_token_count;
  }

  void Store::set_response_token_count(u32 token_count)
  {
    app_setting_.response_token_count = token_count;
  }
} // namespace khaos
