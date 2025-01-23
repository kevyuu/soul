#include "sampler_presets_view.h"

#include "app/icons.h"

namespace khaos::ui
{

  void SamplerPresetsView::on_gui_render(NotNull<app::Gui*> gui, NotNull<Store*> store)
  {
    gui->begin_group();

    const auto samplers        = store->samplers_cspan();
    const auto& active_sampler = store->active_sampler_cref();

    if (gui->button(ICON_MD_ADD))
    {
      new_sampler_popup_.open(gui);
    }
    gui->same_line();
    if (gui->button(ICON_MD_DELETE))
    {
      store->delete_prompt_format();
    }
    gui->same_line();

    gui->same_line();
    if (gui->begin_combo("Sampler"_str, active_sampler.name.cview()))
    {
      for (u32 format_i = 0; format_i < samplers.size(); format_i++)
      {
        const auto& setting  = samplers[format_i];
        const b8 is_selected = setting.name == active_sampler.name;
        if (gui->selectable(setting.name.cview(), is_selected))
        {
          store->select_sampler(format_i);
        }
        if (is_selected)
        {
          gui->set_item_default_focus();
        }
      }
      gui->end_combo();
    }

    render_edit_sampler_view(gui, store);

    gui->end_group();
  }

  void SamplerPresetsView::render_sampler_preset_list(NotNull<app::Gui*> gui, NotNull<Store*> store)
  {
    const auto samplers        = store->samplers_cspan();
    const auto& active_sampler = store->active_sampler_cref();
    for (u32 format_i = 0; format_i < samplers.size(); format_i++)
    {
      const auto& setting  = samplers[format_i];
      const b8 is_selected = setting.name == active_sampler.name;
      if (gui->selectable(setting.name.cview(), is_selected))
      {
        store->select_sampler(format_i);
      }
      if (is_selected)
      {
        gui->set_item_default_focus();
      }
    }
  }

  void SamplerPresetsView::render_edit_sampler_view(NotNull<app::Gui*> gui, NotNull<Store*> store)
  {
    auto& edit_parameter = edit_sampler_.parameter;
    gui->input_text("Name"_str, &edit_sampler_.name);
    gui->slider_f32("Temperature"_str, &edit_parameter.temperature, 0, 5);
    gui->slider_f32("Top P"_str, &edit_parameter.top_p, 0, 1);
    gui->slider_f32("Min P"_str, &edit_parameter.min_p, 0, 1);
    gui->slider_i32("Top K"_str, &edit_parameter.top_k, -1, 200);
    gui->slider_f32("Repetition Penalty"_str, &edit_parameter.repetition_penalty, 0, 1);
    gui->slider_f32("Presence Penalty"_str, &edit_parameter.presence_penalty, 0, 1);
    gui->slider_f32("Frequency Penalty"_str, &edit_parameter.frequency_penalty, 0, 1);
    gui->slider_i32(
      "Repetition Penalty Range"_str, &edit_parameter.repetition_penalty_range, 0, 64000);
    gui->slider_f32("Typical P"_str, &edit_parameter.typical_p, 0, 1);
    gui->slider_f32("TFS"_str, &edit_parameter.tfs, 0, 1);
    gui->slider_f32("Top A"_str, &edit_parameter.top_a, 0, 1);
    gui->slider_f32("Epsilon Cutoff"_str, &edit_parameter.epsilon_cutoff, 0, 1);
    gui->slider_f32("Eta Cutoff"_str, &edit_parameter.eta_cutoff, 0, 1);
    gui->slider_f32(
      "Encoder Repetition Penalty"_str, &edit_parameter.encoder_repetition_penalty, 0, 1);
    gui->slider_i32("No Repetition Ngram Size"_str, &edit_parameter.no_repeat_ngram_size, 0, 64000);
    gui->slider_f32("Smoothing Factor"_str, &edit_parameter.smoothing_factor, 0, 1);
    gui->slider_f32("Smoothing Curve"_str, &edit_parameter.smoothing_curve, 0, 1);
    gui->slider_f32("DRY Multiplier"_str, &edit_parameter.dry_multiplier, 0, 1);
    gui->slider_f32("DRY Base"_str, &edit_parameter.dry_base, 0, 1);
    gui->slider_i32("DRY Allowed Length"_str, &edit_parameter.no_repeat_ngram_size, 0, 64000);
    gui->checkbox("Dynamic Temperature"_str, &edit_parameter.dynamic_temperature);
    gui->slider_f32("Min Temperature"_str, &edit_parameter.dynatemp_low, 0, 1);
    gui->slider_f32("Max Temperature"_str, &edit_parameter.dynatemp_high, 0, 1);
    gui->slider_f32("Exponent"_str, &edit_parameter.dynatemp_exponent, 0, 1);
    gui->slider_i32("Mirostat Mode"_str, &edit_parameter.mirostat_mode, 0, 64000);
    gui->slider_f32("Mirostat Tau"_str, &edit_parameter.mirostat_tau, 0, 1);
    gui->slider_f32("Mirostat Eta"_str, &edit_parameter.mirostat_eta, 0, 1);
    gui->slider_f32("Penalty Alpha"_str, &edit_parameter.penalty_alpha, 0, 1);
    gui->checkbox("Do Sample"_str, &edit_parameter.do_sample);
    gui->checkbox("Add BOS Token"_str, &edit_parameter.add_bos_token);
    gui->checkbox("Ban EOS Token"_str, &edit_parameter.ban_eos_token);
    gui->checkbox("Skip Special Tokens"_str, &edit_parameter.skip_special_tokens);
    gui->checkbox("Temperature Last"_str, &edit_parameter.temperature_last);
  }
} // namespace khaos::ui
