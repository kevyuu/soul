#include "store.h"

#include "core/array.h"
#include "core/log.h"
#include "misc/filesystem.h"
#include "misc/json.h"
#include "type.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>

namespace khaos
{
  Store::Store(const Path& storage_path)
      : storage_path_(storage_path.clone()),
        app_setting_path_(storage_path / "app_setting.json"_str),
        prompt_format_path_(storage_path_ / "prompt_format_settings"_str),
        sampler_path_(storage_path_ / "sampler_settings"_str),
        active_prompt_format_index_(0),
        active_sampler_index_(0),
        api_url_("http://127.0.0.1:5000"_str),
        context_token_count_(16384),
        response_token_count_(250)
  {
    static const auto PROMPT_FORMAT_SETTINGS = Array{
      PromptFormat{
        .name             = "Llama 3"_str,
        .header_prefix    = "<|begin_of_text|><|start_header_id|>system<|end_header_id|>"_str,
        .header_suffix    = "<|eot_id|>"_str,
        .user_prefix      = "<|start_header_id|>user<|end_header_id|>"_str,
        .user_suffix      = "<|eot_id|>"_str,
        .assistant_prefix = "<|start_header_id|>assistant<|end_header_id|>"_str,
        .assistant_suffix = "<|eot_id|>"_str,
        .system_prefix    = "<|start_header_id|>system<|end_header_id|>"_str,
        .system_suffix    = "<|eot_id|>"_str,
      },
      PromptFormat{
        .name             = "ChatML"_str,
        .header_prefix    = "<|im_start|>system"_str,
        .header_suffix    = "<|im_end|>"_str,
        .user_prefix      = "<|im_start|>user"_str,
        .user_suffix      = "<|im_end|>"_str,
        .assistant_prefix = "<|im_start|>assistant"_str,
        .assistant_suffix = "<|im_end|>"_str,
        .system_prefix    = "<|im_start|>system"_str,
        .system_suffix    = "<|im_end|>"_str,
      },
    };

    static const auto SAMPLER_SETTINGS = Array{
      Sampler{
        .name               = "Big O"_str,
        .temperature        = 0.87,
        .top_p              = 0.99,
        .top_k              = 85,
        .repetition_penalty = 1.01,
        .typical_p          = 0.68,
        .tfs                = 0.68,
      },
      Sampler{
        .name      = "Debug-deterministic"_str,
        .top_k     = 1,
        .do_sample = false,
      },
      Sampler{
        .name               = "Divine Intellect"_str,
        .temperature        = 1.31,
        .top_p              = 0.14,
        .top_k              = 49,
        .repetition_penalty = 1.17,
      },
      Sampler{
        .name               = "Midnight Enighma"_str,
        .temperature        = 0.98,
        .top_p              = 0.37,
        .top_k              = 100,
        .repetition_penalty = 1.18,
      },
      Sampler{
        .name               = "Shortwave"_str,
        .temperature        = 1.53,
        .top_p              = 0.64,
        .top_k              = 33,
        .repetition_penalty = 1.07,
      },
      Sampler{
        .name               = "simple-1"_str,
        .temperature        = 0.7,
        .top_p              = 0.9,
        .top_k              = 20,
        .repetition_penalty = 1.15,
      },
      Sampler{
        .name               = "Yara"_str,
        .temperature        = 0.82,
        .top_p              = 0.21,
        .top_k              = 72,
        .repetition_penalty = 1.19,
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
      const String json_string = to_json_string(AppSetting());
      write_to_file(app_setting_path_, json_string.cspan());
    }

    {
      String app_json_string = get_file_content(app_setting_path_);
      auto app_setting       = from_json_string<AppSetting>(app_json_string.cspan());
      api_url_               = std::move(app_setting.api_url);
      context_token_count_   = app_setting.context_token_count;
      response_token_count_  = app_setting.response_token_count;
      select_prompt_format(app_setting.active_prompt_format.cspan());
      select_sampler(app_setting.active_sampler.cspan());
    }
  }

  [[nodiscard]]
  auto Store::api_url_string_view() const -> StringView
  {
    return api_url_.cspan();
  }

  void Store::set_api_url(StringView api_url)
  {
    api_url_.assign(api_url);
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
        SOUL_LOG_INFO("Setting name : {}, Name : {}", setting.name.cspan(), name);
        if (setting.name.cspan() == name)
        {
          return format_i;
        }
      }
      unreachable();
      return 0;
    }();
  }

  void Store::select_prompt_format(u32 index)
  {
    active_prompt_format_index_ = index;
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
    write_to_file(prompt_format_path_ / filename.cspan(), setting.to_json_string().cspan());
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
  }

  void Store::select_sampler(u32 index)
  {
    active_sampler_index_ = index;
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

  auto Store::is_any_project_active() const -> b8
  {
    return active_project_.is_some();
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
    write_to_file(sampler_path_ / filename.cspan(), setting.to_json_string().cspan());
  }

  auto Store::get_context_token_count() const -> u32
  {
    return context_token_count_;
  }

  void Store::set_context_token_count(u32 token_count)
  {
    context_token_count_ = token_count;
  }

  auto Store::get_response_token_count() const -> u32
  {
    return response_token_count_;
  }

  void Store::set_response_token_count(u32 token_count)
  {
    response_token_count_ = token_count;
  }
} // namespace khaos
