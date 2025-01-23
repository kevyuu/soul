#include "type.h"

#include "core/log.h"
#include "misc/json.h"
#include <vulkan/vulkan_core.h>

namespace khaos
{
  auto PromptFormatParameter::clone() const -> PromptFormatParameter
  {
    return {
      .header_prefix    = header_prefix.clone(),
      .header_suffix    = header_suffix.clone(),
      .user_prefix      = user_prefix.clone(),
      .user_suffix      = user_suffix.clone(),
      .assistant_prefix = assistant_prefix.clone(),
      .assistant_suffix = assistant_suffix.clone(),
      .system_prefix    = system_prefix.clone(),
      .system_suffix    = system_suffix.clone(),
    };
  }

  void PromptFormatParameter::clone_from(const PromptFormatParameter& other)
  {
    header_prefix.clone_from(other.header_prefix);
    header_suffix.clone_from(other.header_suffix);
    user_prefix.clone_from(other.user_prefix);
    user_suffix.clone_from(other.user_suffix);
    assistant_prefix.clone_from(other.assistant_prefix);
    assistant_suffix.clone_from(other.assistant_suffix);
    system_prefix.clone_from(other.system_prefix);
    system_suffix.clone_from(other.system_suffix);
  }

  auto soul_op_build_json(JsonDoc* doc, const PromptFormatParameter& parameter) -> JsonObjectRef
  {
    auto json_ref = doc->create_empty_object();
    json_ref.add("header_prefix"_str, parameter.header_prefix.cview());
    json_ref.add("header_suffix"_str, parameter.header_suffix.cview());
    json_ref.add("user_prefix"_str, parameter.user_prefix.cview());
    json_ref.add("user_suffix"_str, parameter.user_suffix.cview());
    json_ref.add("assistant_prefix"_str, parameter.assistant_prefix.cview());
    json_ref.add("assistant_suffix"_str, parameter.assistant_suffix.cview());
    json_ref.add("system_prefix"_str, parameter.system_prefix.cview());
    json_ref.add("system_suffix"_str, parameter.system_suffix.cview());
    return json_ref;
  }

  auto PromptFormat::clone() const -> PromptFormat
  {
    return {
      .name      = name.clone(),
      .parameter = parameter.clone(),
    };
  }

  void PromptFormat::clone_from(const PromptFormat& other)
  {
    name.clone_from(other.name);
    parameter.clone_from(other.parameter);
  }

  auto soul_op_build_json(JsonDoc* doc, const PromptFormat& prompt_format) -> JsonObjectRef
  {
    auto json_ref = doc->create_empty_object();
    json_ref.add("name"_str, prompt_format.name.cview());
    json_ref.add("parameter"_str, doc->create_object(prompt_format.parameter));
    return json_ref;
  }

  auto SamplerParameter::clone() const -> SamplerParameter
  {
    return {
      .temperature                = temperature,
      .top_p                      = top_p,
      .min_p                      = min_p,
      .top_k                      = top_k,
      .repetition_penalty         = repetition_penalty,
      .presence_penalty           = presence_penalty,
      .frequency_penalty          = frequency_penalty,
      .repetition_penalty_range   = repetition_penalty_range,
      .typical_p                  = typical_p,
      .tfs                        = tfs,
      .top_a                      = top_a,
      .epsilon_cutoff             = epsilon_cutoff,
      .eta_cutoff                 = eta_cutoff,
      .encoder_repetition_penalty = encoder_repetition_penalty,
      .no_repeat_ngram_size       = no_repeat_ngram_size,
      .smoothing_factor           = smoothing_factor,
      .smoothing_curve            = smoothing_curve,
      .dry_multiplier             = dry_multiplier,
      .dry_base                   = dry_base,
      .dry_allowed_length         = dry_allowed_length,
      .dry_sequence_breakers      = dry_sequence_breakers.clone(),
      .dynamic_temperature        = dynamic_temperature,
      .dynatemp_low               = dynatemp_low,
      .dynatemp_high              = dynatemp_high,
      .dynatemp_exponent          = dynatemp_exponent,
      .mirostat_mode              = mirostat_mode,
      .mirostat_tau               = mirostat_tau,
      .mirostat_eta               = mirostat_eta,
      .penalty_alpha              = penalty_alpha,
      .do_sample                  = do_sample,
      .add_bos_token              = add_bos_token,
      .ban_eos_token              = ban_eos_token,
      .skip_special_tokens        = skip_special_tokens,
      .temperature_last           = temperature_last,
      .seed                       = seed,
      .custom_token_bans          = custom_token_bans.clone(),
    };
  }

  void SamplerParameter::clone_from(const SamplerParameter& other)
  {
    temperature                = other.temperature;
    top_p                      = other.top_p;
    min_p                      = other.min_p;
    top_k                      = other.top_k;
    repetition_penalty         = other.repetition_penalty;
    presence_penalty           = other.presence_penalty;
    frequency_penalty          = other.frequency_penalty;
    repetition_penalty_range   = other.repetition_penalty_range;
    typical_p                  = other.typical_p;
    tfs                        = other.tfs;
    top_a                      = other.top_a;
    epsilon_cutoff             = other.epsilon_cutoff;
    eta_cutoff                 = other.eta_cutoff;
    encoder_repetition_penalty = other.encoder_repetition_penalty;
    no_repeat_ngram_size       = other.no_repeat_ngram_size;
    smoothing_factor           = other.smoothing_factor;
    smoothing_curve            = other.smoothing_curve;
    dry_multiplier             = other.dry_multiplier;
    dry_base                   = other.dry_base;
    dry_allowed_length         = other.dry_allowed_length;
    dry_sequence_breakers.clone_from(dry_sequence_breakers);
    dynamic_temperature = other.dynamic_temperature;
    dynatemp_low        = other.dynatemp_low;
    dynatemp_high       = other.dynatemp_high;
    dynatemp_exponent   = other.dynatemp_exponent;
    mirostat_mode       = other.mirostat_mode;
    mirostat_tau        = other.mirostat_tau;
    mirostat_eta        = other.mirostat_eta;
    penalty_alpha       = other.penalty_alpha;
    do_sample           = other.do_sample;
    add_bos_token       = other.add_bos_token;
    ban_eos_token       = other.ban_eos_token;
    skip_special_tokens = other.skip_special_tokens;
    temperature_last    = other.temperature_last;
    seed                = other.seed;
    custom_token_bans.clone_from(other.custom_token_bans);
  }

  auto Sampler::clone() const -> Sampler
  {
    return {
      .name      = name.clone(),
      .parameter = parameter.clone(),
    };
  }

  void Sampler::clone_from(const Sampler& other)
  {
    name.clone_from(other.name);
    parameter.clone_from(other.parameter);
  }

  auto soul_op_build_json(JsonDoc* doc, const SamplerParameter& parameter) -> JsonObjectRef
  {
    auto json_ref = doc->create_empty_object();
    json_ref.add("temperature"_str, parameter.temperature);
    json_ref.add("top_p"_str, parameter.top_p);
    json_ref.add("min_p"_str, parameter.min_p);
    json_ref.add("top_k"_str, parameter.top_k);
    json_ref.add("repetition_penalty"_str, parameter.repetition_penalty);
    json_ref.add("presence_penalty"_str, parameter.presence_penalty);
    json_ref.add("frequency_penalty"_str, parameter.frequency_penalty);
    json_ref.add("repetition_penalty_range"_str, parameter.repetition_penalty_range);
    json_ref.add("typical_p"_str, parameter.typical_p);
    json_ref.add("tfs"_str, parameter.tfs);
    json_ref.add("top_a"_str, parameter.top_a);
    json_ref.add("epsilon_cutoff"_str, parameter.epsilon_cutoff);
    json_ref.add("eta_cutoff"_str, parameter.eta_cutoff);
    json_ref.add("encoder_repetition_penalty"_str, parameter.encoder_repetition_penalty);
    json_ref.add("no_repeat_ngram_size"_str, parameter.no_repeat_ngram_size);
    json_ref.add("smoothing_factor"_str, parameter.smoothing_factor);
    json_ref.add("smoothing_curve"_str, parameter.smoothing_curve);
    json_ref.add("dry_multipler"_str, parameter.dry_multiplier);
    json_ref.add("dry_base"_str, parameter.dry_base);
    json_ref.add("dry_allowed_length"_str, parameter.dry_allowed_length);
    json_ref.add("dry_sequence_breakers"_str, parameter.dry_sequence_breakers.cview());
    json_ref.add("dynamic_temperature"_str, parameter.dynamic_temperature);
    json_ref.add("dynatemp_low"_str, parameter.dynatemp_low);
    json_ref.add("dynatemp_high"_str, parameter.dynatemp_high);
    json_ref.add("dynatemp_exponent"_str, parameter.dynatemp_exponent);
    json_ref.add("mirostat_mode"_str, parameter.mirostat_mode);
    json_ref.add("mirostat_tau"_str, parameter.mirostat_tau);
    json_ref.add("mirostat_eta"_str, parameter.mirostat_eta);
    json_ref.add("penalty_alpha"_str, parameter.penalty_alpha);
    json_ref.add("do_sample"_str, parameter.do_sample);
    json_ref.add("add_bos_token"_str, parameter.add_bos_token);
    json_ref.add("ban_eos_token"_str, parameter.ban_eos_token);
    json_ref.add("skip_special_tokens"_str, parameter.skip_special_tokens);
    json_ref.add("temperature_last"_str, parameter.temperature_last);
    json_ref.add("seed"_str, parameter.seed);
    json_ref.add("custom_token_bans"_str, parameter.custom_token_bans.cview());
    return json_ref;
  }

  auto soul_op_build_json(JsonDoc* doc, const Sampler& sampler) -> JsonObjectRef
  {
    auto json_ref = doc->create_empty_object();
    json_ref.add("name"_str, sampler.name.cview());
    json_ref.add("parameter"_str, doc->create_object(sampler.parameter));
    return json_ref;
  }

  auto soul_op_build_json(JsonDoc* doc, const ProjectMetadata& project_metadata) -> JsonObjectRef
  {
    auto json_ref = doc->create_empty_object();
    json_ref.add("name"_str, project_metadata.name.cview());
    const auto std_string = project_metadata.path.string();
    json_ref.add("path"_str, StringView(std_string.c_str()));
    return json_ref;
  }

  auto soul_op_build_json(JsonDoc* doc, const Message& message) -> JsonObjectRef
  {
    auto json_ref = doc->create_empty_object();
    json_ref.add("role"_str, ROLE_LABELS[message.role]);
    json_ref.add("label"_str, message.label.cview());
    json_ref.add("content"_str, message.content.cview());
    json_ref.add("flags"_str, message.flags.to_i32());
    return json_ref;
  }

  auto soul_op_build_json(JsonDoc* doc, const Journey& journey) -> JsonObjectRef
  {
    auto json_ref = doc->create_empty_object();
    json_ref.add("message"_str, doc->create_array(journey.messages.cspan()));
    return json_ref;
  }

  auto soul_op_build_json(JsonDoc* doc, const Project& project) -> JsonObjectRef
  {
    auto json_ref = doc->create_empty_object();
    json_ref.add("name"_str, project.name.cview());
    return json_ref;
  }

} // namespace khaos

using namespace khaos;

template <>
auto soul_op_construct_from_json<khaos::SamplerParameter>(JsonReadRef val_ref)
  -> khaos::SamplerParameter
{
  SamplerParameter parameter;
  parameter.temperature                = val_ref.ref("temperature"_str).as_f32();
  parameter.top_p                      = val_ref.ref("top_p"_str).as_f32();
  parameter.min_p                      = val_ref.ref("min_p"_str).as_f32();
  parameter.top_k                      = val_ref.ref("top_k"_str).as_i32();
  parameter.repetition_penalty         = val_ref.ref("repetition_penalty"_str).as_f32();
  parameter.presence_penalty           = val_ref.ref("presence_penalty"_str).as_f32();
  parameter.frequency_penalty          = val_ref.ref("frequency_penalty"_str).as_f32();
  parameter.repetition_penalty_range   = val_ref.ref("repetition_penalty_range"_str).as_i32();
  parameter.typical_p                  = val_ref.ref("typical_p"_str).as_f32();
  parameter.tfs                        = val_ref.ref("tfs"_str).as_f32();
  parameter.top_a                      = val_ref.ref("top_a"_str).as_f32();
  parameter.epsilon_cutoff             = val_ref.ref("epsilon_cutoff"_str).as_f32();
  parameter.eta_cutoff                 = val_ref.ref("eta_cutoff"_str).as_f32();
  parameter.encoder_repetition_penalty = val_ref.ref("encoder_repetition_penalty"_str).as_f32();
  parameter.no_repeat_ngram_size       = val_ref.ref("no_repeat_ngram_size"_str).as_i32();
  parameter.smoothing_factor           = val_ref.ref("smoothing_factor"_str).as_f32();
  parameter.smoothing_curve            = val_ref.ref("smoothing_curve"_str).as_f32();
  parameter.dry_multiplier             = val_ref.ref("dry_multipler"_str).as_f32();
  parameter.dry_base                   = val_ref.ref("dry_base"_str).as_f32();
  parameter.dry_allowed_length         = val_ref.ref("dry_allowed_length"_str).as_i32();
  parameter.dry_sequence_breakers =
    String::From(val_ref.ref("dry_sequence_breakers"_str).as_string_view());
  parameter.dynamic_temperature = val_ref.ref("dynamic_temperature"_str).as_b8();
  parameter.dynatemp_low        = val_ref.ref("dynatemp_low"_str).as_f32();
  parameter.dynatemp_high       = val_ref.ref("dynatemp_high"_str).as_f32();
  parameter.dynatemp_exponent   = val_ref.ref("dynatemp_exponent"_str).as_f32();
  parameter.mirostat_mode       = val_ref.ref("mirostat_mode"_str).as_i32();
  parameter.mirostat_tau        = val_ref.ref("mirostat_tau"_str).as_f32();
  parameter.mirostat_eta        = val_ref.ref("mirostat_eta"_str).as_f32();
  parameter.penalty_alpha       = val_ref.ref("penalty_alpha"_str).as_f32();
  parameter.do_sample           = val_ref.ref("do_sample"_str).as_b8();
  parameter.add_bos_token       = val_ref.ref("add_bos_token"_str).as_b8();
  parameter.ban_eos_token       = val_ref.ref("ban_eos_token"_str).as_b8();
  parameter.skip_special_tokens = val_ref.ref("skip_special_tokens"_str).as_b8();
  parameter.temperature_last    = val_ref.ref("temperature_last"_str).as_b8();
  parameter.seed                = val_ref.ref("seed"_str).as_i32();
  parameter.custom_token_bans = String::From(val_ref.ref("custom_token_bans"_str).as_string_view());
  return parameter;
}

template <>
auto soul_op_construct_from_json<Sampler>(JsonReadRef val_ref) -> Sampler
{
  Sampler setting   = {};
  setting.name      = String::From(val_ref.ref("name"_str).as_string_view());
  setting.parameter = soul_op_construct_from_json<SamplerParameter>(val_ref.ref("parameter"_str));
  return setting;
}

template <>
auto soul_op_construct_from_json<ProjectMetadata>(JsonReadRef val_ref) -> ProjectMetadata
{
  return ProjectMetadata{
    .name = String::From(val_ref.ref("name"_str).as_string_view()),
    .path = Path::From(val_ref.ref("path"_str).as_string_view()),
  };
}

template <>
auto soul_op_construct_from_json<Message>(JsonReadRef val_ref) -> Message
{
  return Message{
    .role    = ROLE_LABELS.find_first_key_with_val(val_ref.ref("role"_str).as_string_view()),
    .label   = String::From(val_ref.ref("label"_str).as_string_view()),
    .content = String::From(val_ref.ref("content"_str).as_string_view()),
    .flags   = MessageFlags::FromU64(val_ref.ref("flags"_str).as_u64()),
  };
}

template <>
auto soul_op_construct_from_json<Journey>(JsonReadRef val_ref) -> Journey
{
  auto journey = Journey{
    .name      = "New Journey"_str,
    .user_name = "Kevin"_str,
    .messages  = val_ref.ref("messages"_str).into_vector<Message>(),
  };
  return journey;
}

template <>
auto soul_op_construct_from_json<PromptFormatParameter>(JsonReadRef val_ref)
  -> PromptFormatParameter
{
  PromptFormatParameter parameter = {};
  parameter.header_prefix         = String::From(val_ref.ref("header_prefix"_str).as_string_view());
  parameter.header_suffix         = String::From(val_ref.ref("header_suffix"_str).as_string_view());
  parameter.user_prefix           = String::From(val_ref.ref("user_prefix"_str).as_string_view());
  parameter.user_suffix           = String::From(val_ref.ref("user_suffix"_str).as_string_view());
  parameter.assistant_prefix = String::From(val_ref.ref("assistant_prefix"_str).as_string_view());
  parameter.assistant_suffix = String::From(val_ref.ref("assistant_suffix"_str).as_string_view());
  parameter.system_prefix    = String::From(val_ref.ref("system_prefix"_str).as_string_view());
  parameter.system_suffix    = String::From(val_ref.ref("system_suffix"_str).as_string_view());
  return parameter;
}

template <>
auto soul_op_construct_from_json<PromptFormat>(JsonReadRef val_ref) -> PromptFormat
{
  PromptFormat format_setting = {};
  format_setting.name         = String::From(val_ref.ref("name"_str).as_string_view());
  format_setting.parameter =
    soul_op_construct_from_json<PromptFormatParameter>(val_ref.ref("parameter"_str));
  return format_setting;
}
