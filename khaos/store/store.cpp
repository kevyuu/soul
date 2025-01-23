#include "store/store.h"
#include "textgen_system.h"
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
        String json_string = fs::get_file_content(Path(entry.path()));
        prompt_formats_.push_back(from_json_string<PromptFormat>(json_string.cview()));
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
        String json_string = fs::get_file_content(Path(entry.path()));
        app_setting_.samplers.push_back(from_json_string<Sampler>(json_string.cview()));
      }
    }

    if (!std::filesystem::is_regular_file(app_setting_path_))
    {
      save_app_settings();
    }

    load_app_settings();
  }

  auto Store::app_setting_ref() -> AppSetting&
  {
    return app_setting_;
  }

  auto Store::app_setting_cref() const -> const AppSetting&
  {
    return app_setting_;
  }

  void Store::set_chatbot_api_url(StringView api_url)
  {
    app_setting_.chatbot_setting.api_url.assign(api_url);
  }

  auto Store::prompt_formats_cspan() const -> Span<const PromptFormat*>
  {
    return prompt_formats_.cspan();
  }

  auto Store::active_prompt_format_cref() const -> const PromptFormat&
  {
    return prompt_formats_[app_setting_.chatbot_setting.active_prompt_format_index];
  }

  auto Store::active_prompt_format_index() const -> u32
  {
    return app_setting_.chatbot_setting.active_prompt_format_index;
  }

  auto Store::find_prompt_format_index(StringView name) -> Option<u32>
  {
    for (u32 format_i = 0; format_i < prompt_formats_.size(); format_i++)
    {
      const auto& setting = prompt_formats_[format_i];
      if (setting.name.cview() == name)
      {
        return format_i;
      }
    }
    return nilopt;
  }

  void Store::select_prompt_format(u32 index)
  {
    app_setting_.chatbot_setting.active_prompt_format_index = index;
  }

  void Store::update_prompt_format(const PromptFormat& setting)
  {
    if (active_prompt_format_cref().name != setting.name)
    {
      const auto old_filename = String::Format("{}.json", active_prompt_format_cref().name);
      fs::delete_file(prompt_format_path_ / old_filename.cview());
    }
    prompt_formats_[app_setting_.chatbot_setting.active_prompt_format_index].clone_from(setting);
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
    fs::delete_file(prompt_format_path_ / filename.cview());
    prompt_formats_.remove(app_setting_.chatbot_setting.active_prompt_format_index);
    sort_format_settings();
  }

  void Store::sort_format_settings()
  {
    std::ranges::sort(
      prompt_formats_,
      [](const PromptFormat& val1, const PromptFormat& val2) -> b8
      {
        const auto& name1 = val1.name.cview();
        const auto& name2 = val2.name.cview();
        return std::lexicographical_compare(name1.begin(), name1.end(), name2.begin(), name2.end());
      });
  }

  void Store::save_setting_to_file(const PromptFormat& setting)
  {
    const auto filename = String::Format("{}.json", setting.name);
    JsonDoc doc;
    doc.create_root_object(setting);
    fs::write_file(prompt_format_path_ / filename.cview(), doc.dump().cview());
  }

  auto Store::samplers_cspan() const -> Span<const Sampler*>
  {
    return app_setting_.samplers.cspan();
  }

  auto Store::active_sampler_cref() const -> const Sampler&
  {
    return app_setting_.samplers[app_setting_.chatbot_setting.active_sampler_index];
  }

  auto Store::active_sampler_ref() -> Sampler&
  {
    return app_setting_.samplers[app_setting_.chatbot_setting.active_sampler_index];
  }

  auto Store::active_sampler_index() const -> u32
  {
    return app_setting_.chatbot_setting.active_sampler_index;
  }

  auto Store::find_sampler_index(StringView name) -> Option<u32>
  {
    for (u32 format_i = 0; format_i < app_setting_.samplers.size(); format_i++)
    {
      const auto& setting = app_setting_.samplers[format_i];
      if (setting.name.cview() == name)
      {
        return format_i;
      }
    }
    return nilopt;
  }

  void Store::select_sampler(u32 index)
  {
    app_setting_.chatbot_setting.active_sampler_index = index;
  }

  void Store::update_sampler(const Sampler& setting)
  {
    if (active_sampler_cref().name != setting.name)
    {
      const auto old_filename = String::Format("{}.json", active_sampler_cref().name);
      fs::delete_file(sampler_path_ / old_filename.cview());
    }
    app_setting_.samplers[app_setting_.chatbot_setting.active_sampler_index].clone_from(setting);
    save_setting_to_file(setting);
    sort_sampler_settings();
  }

  void Store::create_sampler(const Sampler& setting)
  {
    app_setting_.samplers.push_back(setting.clone());
    save_setting_to_file(setting);
    sort_sampler_settings();
  }

  void Store::delete_sampler()
  {
    const auto filename = String::Format("{}.json", active_sampler_cref().name);
    fs::delete_file(sampler_path_ / filename.cview());
    app_setting_.samplers.remove(app_setting_.chatbot_setting.active_sampler_index);
    sort_sampler_settings();
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
    active_project_ = Project{
      .name     = ""_str,
      .path     = path.clone(),
      .journeys = Vector<Journey>(),
    };
    script_system_.init(path, this);
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
    script_system_.on_new_journey();
  }

  auto Store::message_cref(u64 idx) -> const Message&
  {
    return active_journey_.some_ref().messages[idx];
  }

  void Store::push_message(OwnRef<Message> message)
  {
    SOUL_ASSERT(0, active_journey_.is_some());
    active_journey_.some_ref().messages.push_back(std::move(message));
  }

  void Store::set_message(u64 idx, OwnRef<Message> message)
  {
    SOUL_ASSERT(0, active_journey_.is_some());
    active_journey_.some_ref().messages[idx] = std::move(message);
  }

  auto Store::get_message(u64 message_index) -> const Message&
  {
    return active_journey_.some_ref().messages[message_index];
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

  void Store::load_app_settings()
  {
    SOUL_LOG_INFO("Load App Settings");
    String app_json_string = fs::get_file_content(app_setting_path_);
    JsonReadDoc doc(app_json_string.cview());
    JsonReadRef root_ref            = doc.get_root_ref();
    JsonReadRef chatbot_setting_ref = root_ref.ref("chatbot_setting"_str);

    auto& chatbot_setting   = app_setting_.chatbot_setting;
    chatbot_setting.api_url = String::From(chatbot_setting_ref.ref("api_url"_str).as_string_view());
    chatbot_setting.context_token_count =
      chatbot_setting_ref.ref("context_token_count"_str).as_u32();
    chatbot_setting.response_token_count =
      chatbot_setting_ref.ref("response_token_count"_str).as_u32();
    chatbot_setting.active_prompt_format_index =
      find_prompt_format_index(chatbot_setting_ref.ref("active_prompt_format"_str).as_string_view())
        .unwrap_or(0);
    chatbot_setting.active_sampler_index =
      find_sampler_index(chatbot_setting_ref.ref("active_sampler"_str).as_string_view())
        .unwrap_or(0);

    app_setting_.project_metadatas =
      root_ref.ref("project_metadatas"_str).into_vector<ProjectMetadata>();
  }

  void Store::save_app_settings()
  {
    JsonDoc doc;
    auto chatbot_setting_ref = doc.create_empty_object();
    chatbot_setting_ref.add("api_url"_str, app_setting_.chatbot_setting.api_url.cview());
    chatbot_setting_ref.add(
      "context_token_count"_str, app_setting_.chatbot_setting.context_token_count);
    chatbot_setting_ref.add(
      "response_token_count"_str, app_setting_.chatbot_setting.response_token_count);
    chatbot_setting_ref.add(
      "active_prompt_format"_str,
      prompt_formats_[app_setting_.chatbot_setting.active_prompt_format_index].name.cview());
    chatbot_setting_ref.add(
      "active_sampler"_str,
      app_setting_.samplers[app_setting_.chatbot_setting.active_sampler_index].name.cview());

    JsonObjectRef root_ref = doc.create_root_empty_object();
    root_ref.add("project_metadatas"_str, doc.create_array(app_setting_.project_metadatas.cspan()));
    root_ref.add("chatbot_setting"_str, chatbot_setting_ref);

    const auto json_string = doc.dump();
    SOUL_LOG_INFO("App Setting JSON: {}", json_string);
    fs::write_file(app_setting_path_, json_string.cview());
  }

  void Store::on_new_frame()
  {
    textgen_system_.on_new_frame();
  }

  void Store::sort_sampler_settings()
  {
    std::ranges::sort(
      app_setting_.samplers,
      [](const Sampler& val1, const Sampler& val2) -> b8
      {
        const auto& name1 = val1.name.cview();
        const auto& name2 = val2.name.cview();
        return std::lexicographical_compare(name1.begin(), name1.end(), name2.begin(), name2.end());
      });
  }

  void Store::save_setting_to_file(const Sampler& setting)
  {
    const auto filename = String::Format("{}.json", setting.name);
    JsonDoc doc;
    doc.create_root_object(setting);
    fs::write_file(sampler_path_ / filename.cview(), doc.dump().cview());
  }

  void Store::set_chatbot_context_token_count(u32 token_count)
  {
    app_setting_.chatbot_setting.context_token_count = token_count;
  }

  void Store::set_chatbot_response_token_count(u32 token_count)
  {
    app_setting_.chatbot_setting.response_token_count = token_count;
  }

  void Store::set_chatbot_tokenizer_type(TokenizerType tokenizer_type)
  {
    app_setting_.chatbot_setting.tokenizer_type = tokenizer_type;
  }

  auto Store::textgen_system_ref() -> TextgenSystem&
  {
    return textgen_system_;
  }

  auto Store::script_system_ref() -> ScriptSystem&
  {
    return script_system_;
  }
} // namespace khaos
