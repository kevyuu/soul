#pragma once

#include "core/flag_map.h"
#include "core/path.h"
#include "core/string.h"
#include "core/vector.h"

#include "misc/json.h"

using namespace soul;

namespace khaos
{
  struct ProjectMetadata
  {
    String name;
    Path path;

    friend auto soul_op_build_json(JsonDoc* doc, const ProjectMetadata& metadata) -> JsonObjectRef;
  };

  struct Sampler
  {
    String name                    = ""_str;
    f32 temperature                = 1;
    f32 top_p                      = 1;
    f32 min_p                      = 0;
    i32 top_k                      = 0;
    f32 repetition_penalty         = 1;
    f32 presence_penalty           = 0;
    f32 frequency_penalty          = 0;
    i32 repetition_penalty_range   = 0;
    f32 typical_p                  = 1;
    f32 tfs                        = 1;
    f32 top_a                      = 0;
    f32 epsilon_cutoff             = 0;
    f32 eta_cutoff                 = 0;
    f32 encoder_repetition_penalty = 1;
    i32 no_repeat_ngram_size       = 0;
    f32 smoothing_factor           = 0;
    f32 smoothing_curve            = 1;
    f32 dry_multipler              = 0;
    f32 dry_base                   = 1.75_f32;
    i32 dry_allowed_length         = 2;
    b8 dynamic_temperature         = false;
    f32 dynatemp_low               = 1;
    f32 dynatemp_high              = 1;
    f32 dynatemp_exponent          = 1;
    i32 mirostat_mode              = 0;
    f32 mirostat_tau               = 2;
    f32 mirostat_eta               = 0.1;
    f32 penalty_alpha              = 0;
    b8 do_sample                   = true;
    b8 add_bos_token               = true;
    b8 ban_eos_token               = false;
    b8 skip_special_tokens         = true;
    b8 temperature_last            = true;
    i32 seed                       = -1;

    [[nodiscard]]
    auto clone() const -> Sampler;

    void clone_from(const Sampler& other);

    [[nodiscard]]
    auto to_json_string() const -> String;
  };

  struct PromptFormat
  {
    String name = ""_str;

    String header_prefix = ""_str;
    String header_suffix = ""_str;

    String user_prefix = ""_str;
    String user_suffix = ""_str;

    String assistant_prefix = ""_str;
    String assistant_suffix = ""_str;

    String system_prefix = ""_str;
    String system_suffix = ""_str;

    [[nodiscard]]
    auto clone() const -> PromptFormat;

    void clone_from(const PromptFormat& other);

    [[nodiscard]]
    auto to_json_string() const -> String;
  };

  enum class Role : u8
  {
    SYSTEM,
    USER,
    ASSISTANT,
    COUNT
  };
  static constexpr FlagMap<Role, CompStr> ROLE_LABELS = {
    "SYSTEM"_str,
    "USER"_str,
    "ASSISTANT"_str,
  };

  struct Message
  {
    Role role;
    String content;

    auto clone() const -> Message
    {
      return Message{
        .role    = role,
        .content = content.clone(),
      };
    }

    void clone_from(const Message& other)
    {
      role = other.role;
      content.clone_from(other.content);
    }

    friend auto soul_op_build_json(JsonDoc* doc, const Message& message) -> JsonObjectRef;
  };

  struct Journey
  {
    String name;
    String user_name;
    Vector<Message> messages;

    friend auto soul_op_build_json(JsonDoc* doc, const Journey& journey) -> JsonObjectRef;
  };

  struct Project
  {
    String name;
    String header_prompt;
    Message first_message = {Role::ASSISTANT, String::From(""_str)};
    Vector<Journey> journeys;

    friend auto soul_op_build_json(JsonDoc* doc, const Project& project) -> JsonObjectRef;
  };

  struct AppSetting
  {
    String api_url              = "http:127.0.0.1:5000"_str;
    u32 context_token_count     = 16384;
    u32 response_token_count    = 250;
    String active_prompt_format = "Llama 3"_str;
    String active_sampler       = "Big O"_str;
    Vector<ProjectMetadata> project_metadatas;

    friend auto soul_op_build_json(JsonDoc* doc, const AppSetting& setting) -> JsonObjectRef;
  };

  struct CompletionRequest
  {
    String model;
    String prompt;
  };

} // namespace khaos

template <>
auto soul_op_construct_from_json<khaos::ProjectMetadata>(JsonReadRef val_ref)
  -> khaos::ProjectMetadata;

template <>
auto soul_op_construct_from_json<khaos::Project>(JsonReadRef val_ref) -> khaos::Project;

template <>
auto soul_op_construct_from_json<khaos::AppSetting>(JsonReadRef val_ref) -> khaos::AppSetting;

template <>
auto soul_op_construct_from_json<khaos::PromptFormat>(JsonReadRef val_ref) -> khaos::PromptFormat;

template <>
auto soul_op_construct_from_json<khaos::Sampler>(JsonReadRef val_ref) -> khaos::Sampler;
