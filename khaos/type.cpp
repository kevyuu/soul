#include "type.h"

#include "misc/json.h"

namespace khaos
{
  auto PromptFormat::to_json_string() const -> String
  {
    return String::Format(
      R"({{
  "name" : "{}",
  "header_prefix" : "{}",
  "header_suffix" : "{}",
  "user_prefix" : "{}",
  "user_suffix" : "{}",
  "assistant_prefix" : "{}",
  "assistant_suffix" : "{}",
  "system_prefix" : "{}",
  "system_suffix" : "{}"
}})",
      name,
      header_prefix,
      header_suffix,
      user_prefix,
      user_suffix,
      assistant_prefix,
      assistant_suffix,
      system_prefix,
      system_suffix);
  }

  auto PromptFormat::clone() const -> PromptFormat
  {
    return {
      .name             = name.clone(),
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

  void PromptFormat::clone_from(const PromptFormat& other)
  {
    name.clone_from(other.name);
    header_prefix.clone_from(other.header_prefix);
    header_suffix.clone_from(other.header_suffix);
    user_prefix.clone_from(other.user_prefix);
    user_suffix.clone_from(other.user_suffix);
    assistant_prefix.clone_from(other.assistant_prefix);
    assistant_suffix.clone_from(other.assistant_suffix);
    system_prefix.clone_from(other.system_prefix);
    system_suffix.clone_from(other.system_suffix);
  }

  auto Sampler::clone() const -> Sampler
  {
    return {
      .name                       = name.clone(),
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
      .dry_multipler              = dry_multipler,
      .dry_base                   = dry_base,
      .dry_allowed_length         = dry_allowed_length,
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
    };
  }

  void Sampler::clone_from(const Sampler& other)
  {
    name.clone_from(other.name);
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
    dry_multipler              = other.dry_multipler;
    dry_base                   = other.dry_base;
    dry_allowed_length         = other.dry_allowed_length;
    dynamic_temperature        = other.dynamic_temperature;
    dynatemp_low               = other.dynatemp_low;
    dynatemp_high              = other.dynatemp_high;
    dynatemp_exponent          = other.dynatemp_exponent;
    mirostat_mode              = other.mirostat_mode;
    mirostat_tau               = other.mirostat_tau;
    mirostat_eta               = other.mirostat_eta;
    penalty_alpha              = other.penalty_alpha;
    do_sample                  = other.do_sample;
    add_bos_token              = other.add_bos_token;
    ban_eos_token              = other.ban_eos_token;
    skip_special_tokens        = other.skip_special_tokens;
    temperature_last           = other.temperature_last;
    seed                       = other.seed;
  }

  auto Sampler::to_json_string() const -> String
  {
    return String::Format(
      R"({{
  "name" : "{}",
  "temperature"                : {},
  "top_p"                      : {},
  "min_p"                      : {},
  "top_k"                      : {},
  "repetition_penalty"         : {},
  "presence_penalty"           : {},
  "frequency_penalty"          : {},
  "repetition_penalty_range"   : {},
  "typical_p"                  : {},
  "tfs"                        : {},
  "top_a"                      : {},
  "epsilon_cutoff"             : {},
  "eta_cutoff"                 : {},
  "encoder_repetition_penalty" : {},
  "no_repeat_ngram_size"       : {},
  "smoothing_factor"           : {},
  "smoothing_curve"            : {},
  "dry_multipler"              : {},
  "dry_base"                   : {},
  "dry_allowed_length"         : {},
  "dynamic_temperature"        : {},
  "dynatemp_low"               : {},
  "dynatemp_high"              : {},
  "dynatemp_exponent"          : {},
  "mirostat_mode"              : {},
  "mirostat_tau"               : {},
  "mirostat_eta"               : {},
  "penalty_alpha"              : {},
  "do_sample"                  : {},
  "add_bos_token"              : {},
  "ban_eos_token"              : {},
  "skip_special_tokens"        : {},
  "temperature_last"           : {},
  "seed"                       : {}
}})",
      name,
      temperature,
      top_p,
      min_p,
      top_k,
      repetition_penalty,
      presence_penalty,
      frequency_penalty,
      repetition_penalty_range,
      typical_p,
      tfs,
      top_a,
      epsilon_cutoff,
      eta_cutoff,
      encoder_repetition_penalty,
      no_repeat_ngram_size,
      smoothing_factor,
      smoothing_curve,
      dry_multipler,
      dry_base,
      dry_allowed_length,
      dynamic_temperature,
      dynatemp_low,
      dynatemp_high,
      dynatemp_exponent,
      mirostat_mode,
      mirostat_tau,
      mirostat_eta,
      penalty_alpha,
      do_sample,
      add_bos_token,
      ban_eos_token,
      skip_special_tokens,
      temperature_last,
      seed);
  }

  auto soul_op_build_json(JsonBuilderRef builder, const ProjectMetadata& project_metadata)
    -> JsonRef
  {
    auto json_ref = builder.create_json_empty_object();
    json_ref.add("Name"_str, project_metadata.name.cspan());
    json_ref.add("Path"_str, StringView(project_metadata.path.string().c_str()));
    return json_ref;
  }

  auto soul_op_build_json(JsonBuilderRef builder, const AppSetting& setting) -> JsonRef
  {
    auto json_ref = builder.create_json_empty_object();
    json_ref.add("api_url"_str, setting.api_url.cspan());
    json_ref.add("context_token_count"_str, setting.context_token_count);
    json_ref.add("response_token_count"_str, setting.response_token_count);
    json_ref.add("active_prompt_format"_str, setting.active_prompt_format.cspan());
    json_ref.add("active_sampler"_str, setting.active_sampler.cspan());
    json_ref.add(
      "project_metadatas"_str, builder.create_json_array(setting.project_metadatas.cspan()));
    return json_ref;
  }

} // namespace khaos

using namespace khaos;

template <>
auto soul_op_construct_from_json<ProjectMetadata>(JsonReadRef val_ref) -> ProjectMetadata
{
  return ProjectMetadata{
    .name = String::From(val_ref.ref("name"_str).as_string_view()),
    .path = Path::From(val_ref.ref("path"_str).as_string_view()),
  };
}

template <>
auto soul_op_construct_from_json<AppSetting>(JsonReadRef val_ref) -> AppSetting
{
  auto setting = AppSetting{
    .api_url              = String::From(val_ref.ref("api_url"_str).as_string_view()),
    .context_token_count  = val_ref.ref("context_token_count"_str).as_u32(),
    .response_token_count = val_ref.ref("response_token_count"_str).as_u32(),
    .active_prompt_format = String::From(val_ref.ref("active_prompt_format"_str).as_string_view()),
    .active_sampler       = String::From(val_ref.ref("active_sampler"_str).as_string_view()),
    .project_metadatas    = Vector<ProjectMetadata>(),
  };
  val_ref.as_array_for_each(
    [&](u32 idx, JsonReadRef metadata_json_ref)
    {
      setting.project_metadatas.push_back(
        soul_op_construct_from_json<ProjectMetadata>(metadata_json_ref));
    });
  return setting;
}

template <>
auto soul_op_construct_from_json<PromptFormat>(JsonReadRef val_ref) -> PromptFormat
{
  PromptFormat format_setting  = {};
  format_setting.name          = String::From(val_ref.ref("name"_str).as_string_view());
  format_setting.header_prefix = String::From(val_ref.ref("header_prefix"_str).as_string_view());
  format_setting.header_suffix = String::From(val_ref.ref("header_suffix"_str).as_string_view());
  format_setting.user_prefix   = String::From(val_ref.ref("user_prefix"_str).as_string_view());
  format_setting.user_suffix   = String::From(val_ref.ref("user_suffix"_str).as_string_view());
  format_setting.assistant_prefix =
    String::From(val_ref.ref("assistant_prefix"_str).as_string_view());
  format_setting.assistant_suffix =
    String::From(val_ref.ref("assistant_suffix"_str).as_string_view());
  format_setting.system_prefix = String::From(val_ref.ref("system_prefix"_str).as_string_view());
  format_setting.system_suffix = String::From(val_ref.ref("system_suffix"_str).as_string_view());
  return format_setting;
}

template <>
auto soul_op_construct_from_json<Sampler>(JsonReadRef val_ref) -> Sampler
{
  Sampler setting                    = {};
  setting.name                       = String::From(val_ref.ref("name"_str).as_string_view());
  setting.temperature                = val_ref.ref("temperature"_str).as_f32();
  setting.top_p                      = val_ref.ref("top_p"_str).as_f32();
  setting.min_p                      = val_ref.ref("min_p"_str).as_f32();
  setting.top_k                      = val_ref.ref("top_k"_str).as_i32();
  setting.repetition_penalty         = val_ref.ref("repetition_penalty"_str).as_f32();
  setting.presence_penalty           = val_ref.ref("presence_penalty"_str).as_f32();
  setting.frequency_penalty          = val_ref.ref("frequency_penalty"_str).as_f32();
  setting.repetition_penalty_range   = val_ref.ref("repetition_penalty_range"_str).as_i32();
  setting.typical_p                  = val_ref.ref("typical_p"_str).as_f32();
  setting.tfs                        = val_ref.ref("tfs"_str).as_f32();
  setting.top_a                      = val_ref.ref("top_a"_str).as_f32();
  setting.epsilon_cutoff             = val_ref.ref("epsilon_cutoff"_str).as_f32();
  setting.eta_cutoff                 = val_ref.ref("eta_cutoff"_str).as_f32();
  setting.encoder_repetition_penalty = val_ref.ref("encoder_repetition_penalty"_str).as_f32();
  setting.no_repeat_ngram_size       = val_ref.ref("no_repeat_ngram_size"_str).as_i32();
  setting.smoothing_factor           = val_ref.ref("smoothing_factor"_str).as_f32();
  setting.smoothing_curve            = val_ref.ref("smoothing_curve"_str).as_f32();
  setting.dry_multipler              = val_ref.ref("dry_multipler"_str).as_f32();
  setting.dry_base                   = val_ref.ref("dry_base"_str).as_f32();
  setting.dry_allowed_length         = val_ref.ref("dry_allowed_length"_str).as_i32();
  setting.dynamic_temperature        = val_ref.ref("dynamic_temperature"_str).as_b8();
  setting.dynatemp_low               = val_ref.ref("dynatemp_low"_str).as_f32();
  setting.dynatemp_high              = val_ref.ref("dynatemp_high"_str).as_f32();
  setting.dynatemp_exponent          = val_ref.ref("dynatemp_exponent"_str).as_f32();
  setting.mirostat_mode              = val_ref.ref("mirostat_mode"_str).as_i32();
  setting.mirostat_tau               = val_ref.ref("mirostat_tau"_str).as_f32();
  setting.mirostat_eta               = val_ref.ref("mirostat_eta"_str).as_f32();
  setting.penalty_alpha              = val_ref.ref("penalty_alpha"_str).as_f32();
  setting.do_sample                  = val_ref.ref("do_sample"_str).as_b8();
  setting.add_bos_token              = val_ref.ref("add_bos_token"_str).as_b8();
  setting.ban_eos_token              = val_ref.ref("ban_eos_token"_str).as_b8();
  setting.skip_special_tokens        = val_ref.ref("skip_special_tokens"_str).as_b8();
  setting.temperature_last           = val_ref.ref("temperature_last"_str).as_b8();
  setting.seed                       = val_ref.ref("seed"_str).as_i32();
  return setting;
}
