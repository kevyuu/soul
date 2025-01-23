#pragma once

#include "core/flag_map.h"
#include "core/flag_set.h"
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

  struct PromptFormatParameter
  {
    String header_prefix = ""_str;
    String header_suffix = ""_str;

    String user_prefix = ""_str;
    String user_suffix = ""_str;

    String assistant_prefix = ""_str;
    String assistant_suffix = ""_str;

    String system_prefix = ""_str;
    String system_suffix = ""_str;

    [[nodiscard]]
    auto clone() const -> PromptFormatParameter;

    void clone_from(const PromptFormatParameter& other);

    friend auto soul_op_build_json(JsonDoc* doc, const PromptFormatParameter& parameter)
      -> JsonObjectRef;
  };

  struct PromptFormat
  {
    String name = ""_str;

    PromptFormatParameter parameter;

    [[nodiscard]]
    auto clone() const -> PromptFormat;

    void clone_from(const PromptFormat& other);

    friend auto soul_op_build_json(JsonDoc* doc, const PromptFormat& prompt_format)
      -> JsonObjectRef;
  };

  struct SamplerParameter
  {
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
    f32 dry_multiplier             = 0;
    f32 dry_base                   = 1.75_f32;
    i32 dry_allowed_length         = 2;
    String dry_sequence_breakers   = R"("\n", ":", "\"", "*")"_str;
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
    String custom_token_bans       = ""_str;

    [[nodiscard]]
    auto clone() const -> SamplerParameter;

    void clone_from(const SamplerParameter& other);

    friend auto soul_op_build_json(JsonDoc* doc, const SamplerParameter& parameter)
      -> JsonObjectRef;
  };

  struct Sampler
  {
    String name = ""_str;
    SamplerParameter parameter;

    [[nodiscard]]
    auto clone() const -> Sampler;

    void clone_from(const Sampler& other);

    friend auto soul_op_build_json(JsonDoc* doc, const Sampler& sampler) -> JsonObjectRef;
  };

  struct Persona
  {
    String name        = ""_str;
    String description = ""_str;
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

  enum class MessageFlag : u8
  {
    INVISIBLE_TO_TEXTGEN,
    INVISIBLE_TO_USER,
    COUNT
  };
  using MessageFlags = FlagSet<MessageFlag>;

  struct Message
  {
    Role role;
    String label;
    String content;
    MessageFlags flags;

    auto clone() const -> Message
    {
      return Message{
        .role    = role,
        .label   = label.clone(),
        .content = content.clone(),
        .flags   = flags,
      };
    }

    void clone_from(const Message& other)
    {
      role = other.role;
      label.clone_from(other.label);
      content.clone_from(other.content);
      flags = other.flags;
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
    Path path;
    Vector<Journey> journeys;

    friend auto soul_op_build_json(JsonDoc* doc, const Project& project) -> JsonObjectRef;
  };

  enum class TokenizerType : u8
  {
    CLAUDE,
    COMMAND_R,
    DEEPSEEK,
    LLAMA3,
    NEMO,
    QWEN2,
    YI,
    COUNT
  };

  static constexpr FlagMap<TokenizerType, CompStr> TOKENIZER_TYPE_LABELS = {
    "Claude"_str,
    "Command-R"_str,
    "Deepseek"_str,
    "Llama3"_str,
    "Nemo"_str,
    "Qwen2"_str,
    "Yi"_str,
  };

  struct ChatbotSetting
  {
    String api_url                 = R"(http://127.0.0.1:5001)"_str;
    u32 context_token_count        = 16384;
    u32 response_token_count       = 250;
    u32 active_prompt_format_index = 0;
    u32 active_sampler_index       = 0;
    TokenizerType tokenizer_type   = TokenizerType::CLAUDE;
  };

  struct AppSetting
  {
    ChatbotSetting chatbot_setting;

    Vector<Sampler> samplers;
    Vector<ProjectMetadata> project_metadatas;

    friend auto soul_op_build_json(JsonDoc* doc, const AppSetting& setting) -> JsonObjectRef;
  };

  enum class GameState
  {
    WAITING_USER_RESPONSE,
    GENERATING_ASSISTANT_RESPONSE,
    COUNT
  };

} // namespace khaos

template <>
auto soul_op_construct_from_json<khaos::SamplerParameter>(JsonReadRef val_ref)
  -> khaos::SamplerParameter;

template <>
auto soul_op_construct_from_json<khaos::Sampler>(JsonReadRef val_ref) -> khaos::Sampler;

template <>
auto soul_op_construct_from_json<khaos::ProjectMetadata>(JsonReadRef val_ref)
  -> khaos::ProjectMetadata;

template <>
auto soul_op_construct_from_json<khaos::Message>(JsonReadRef val_ref) -> khaos::Message;

template <>
auto soul_op_construct_from_json<khaos::Journey>(JsonReadRef val_ref) -> khaos::Journey;

template <>
auto soul_op_construct_from_json<khaos::AppSetting>(JsonReadRef val_ref) -> khaos::AppSetting;

template <>
auto soul_op_construct_from_json<khaos::PromptFormatParameter>(JsonReadRef val_ref)
  -> khaos::PromptFormatParameter;

template <>
auto soul_op_construct_from_json<khaos::PromptFormat>(JsonReadRef val_ref) -> khaos::PromptFormat;
