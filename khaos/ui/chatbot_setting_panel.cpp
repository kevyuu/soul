#include "ui/chatbot_setting_panel.h"
#include "core/util.h"
#include "store.h"

#include "core/type.h"

#include "app/gui.h"
#include "app/icons.h"

#include "misc/json.h"

#include "httplib.h"

using namespace soul;

namespace khaos
{
  void ChatbotSettingPanel::render_prompt_formatting_widget(
    NotNull<soul::app::Gui*> gui, Store* store)
  {
    gui->push_id("Prompt Format"_str);
    SCOPE_EXIT(gui->pop_id());

    const auto prompt_format_settings = store->prompt_formats_cspan();
    const auto& active_format_setting = store->active_prompt_format_cref();

    if (gui->begin_popup_modal("Edit Prompt Format"_str))
    {
      gui->input_text("Name"_str, &edit_prompt_format_.name);
      const vec2f32 input_area_size = vec2f32(0, 60);
      gui->input_text_multiline(
        "Header Prefix"_str, &edit_prompt_format_.parameter.header_prefix, input_area_size);
      gui->input_text_multiline(
        "Header Suffix"_str, &edit_prompt_format_.parameter.header_suffix, input_area_size);
      gui->input_text_multiline(
        "User Prefix"_str, &edit_prompt_format_.parameter.user_prefix, input_area_size);
      gui->input_text_multiline(
        "User Suffix"_str, &edit_prompt_format_.parameter.user_suffix, input_area_size);
      gui->input_text_multiline(
        "Assistant Prefix"_str, &edit_prompt_format_.parameter.assistant_prefix, input_area_size);
      gui->input_text_multiline(
        "Assistant Suffix"_str, &edit_prompt_format_.parameter.assistant_suffix, input_area_size);
      gui->input_text_multiline(
        "System Prefix"_str, &edit_prompt_format_.parameter.system_prefix, input_area_size);
      gui->input_text_multiline(
        "System Suffix"_str, &edit_prompt_format_.parameter.system_suffix, input_area_size);

      if (gui->button("Save"_str, vec2f32(120, 0)))
      {
        store->update_prompt_format(edit_prompt_format_);
        gui->close_current_popup();
      }
      gui->same_line();
      if (gui->button("Cancel"_str, vec2f32(120, 0)))
      {
        gui->close_current_popup();
      }
      gui->end_popup();
    }

    if (gui->begin_popup_modal("Create New Prompt Format"_str))
    {
      gui->input_text("Name"_str, &new_prompt_format_name_);
      if (gui->button("Save"_str, vec2f32(120, 0)))
      {
        auto setting = active_format_setting.clone();
        setting.name = new_prompt_format_name_.clone();
        store->create_prompt_format(setting);
        gui->close_current_popup();
      }
      gui->same_line();
      if (gui->button("Cancel"_str, vec2f32(120, 0)))
      {
        gui->close_current_popup();
      }
      gui->end_popup();
    }

    if (gui->button(ICON_MD_ADD))
    {
      new_prompt_format_name_ = ""_str;
      gui->open_popup("Create New Prompt Format"_str);
    }
    gui->same_line();
    if (gui->button(ICON_MD_DELETE))
    {
      store->delete_prompt_format();
    }
    gui->same_line();

    if (gui->button(ICON_MD_EDIT))
    {
      edit_prompt_format_.clone_from(active_format_setting);
      gui->open_popup("Edit Prompt Format"_str);
    }
    gui->same_line();

    if (gui->begin_combo("Prompt Formatting"_str, active_format_setting.name.cspan()))
    {
      for (u32 format_i = 0; format_i < prompt_format_settings.size(); format_i++)
      {
        const auto& setting  = prompt_format_settings[format_i];
        const b8 is_selected = setting.name == active_format_setting.name;
        if (gui->selectable(setting.name.cspan(), is_selected))
        {
          store->select_prompt_format(format_i);
        }
        if (is_selected)
        {
          gui->set_item_default_focus();
        }
      }
      gui->end_combo();
    }
  }

  void ChatbotSettingPanel::render_sampler_setting(NotNull<app::Gui*> gui, Store* store)
  {
    gui->push_id("Sampler Setting"_str);
    SCOPE_EXIT(gui->pop_id());

    const auto samplers        = store->samplers_cspan();
    const auto& active_sampler = store->active_sampler_cref();
    auto& edit_parameter       = edit_sampler_.parameter;

    if (gui->begin_popup_modal("Edit Sampler"_str))
    {
      gui->input_text("Name"_str, &edit_sampler_.name);
      gui->slider_f32("Temperature"_str, &edit_parameter.temperature, 0, 5);
      gui->slider_f32("Top P"_str, &edit_parameter.top_p, 0, 1);
      gui->slider_f32("Min P"_str, &edit_parameter.min_p, 0, 1);
      gui->slider_i32("Top K"_str, &edit_parameter.top_k, -1, 200);
      gui->slider_f32("Min P"_str, &edit_parameter.min_p, 0, 1);
      gui->slider_f32("Repetition Penalty"_str, &edit_parameter.repetition_penalty, 0, 1);
      gui->slider_f32("Presence Penalty"_str, &edit_parameter.presence_penalty, 0, 1);
      gui->slider_f32("Frequency Penalty"_str, &edit_parameter.frequency_penalty, 0, 1);
      gui->slider_i32(
        "Repetition Penalty Range"_str, &edit_parameter.repetition_penalty_range, 0, 64000);
      gui->slider_f32("Typcial P"_str, &edit_parameter.typical_p, 0, 1);
      gui->slider_f32("TFS"_str, &edit_parameter.tfs, 0, 1);
      gui->slider_f32("Top A"_str, &edit_parameter.top_a, 0, 1);
      gui->slider_f32("Epsilon Cutoff"_str, &edit_parameter.epsilon_cutoff, 0, 1);
      gui->slider_f32("Eta Cutoff"_str, &edit_parameter.eta_cutoff, 0, 1);
      gui->slider_f32(
        "Encoder Repetition Penalty"_str, &edit_parameter.encoder_repetition_penalty, 0, 1);
      gui->slider_i32(
        "No Repetition Ngram Size"_str, &edit_parameter.no_repeat_ngram_size, 0, 64000);
      gui->slider_f32("Smoothing Factor"_str, &edit_parameter.smoothing_factor, 0, 1);
      gui->slider_f32("Smoothing Curve"_str, &edit_parameter.smoothing_curve, 0, 1);
      gui->slider_f32("DRY Multiplier"_str, &edit_parameter.dry_multipler, 0, 1);
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

      if (gui->button("Save"_str, vec2f32(120, 0)))
      {
        store->update_sampler(edit_sampler_);
        gui->close_current_popup();
      }
      gui->same_line();
      if (gui->button("Cancel"_str, vec2f32(120, 0)))
      {
        gui->close_current_popup();
      }
      gui->end_popup();
    }

    if (gui->begin_popup_modal("Create Sampler"_str))
    {
      gui->input_text("Name"_str, &new_sampler_name_);
      if (gui->button("Save"_str, vec2f32(120, 0)))
      {
        auto setting = active_sampler.clone();
        setting.name = new_sampler_name_.clone();
        store->create_sampler(setting);
        gui->close_current_popup();
      }
      gui->same_line();
      if (gui->button("Cancel"_str, vec2f32(120, 0)))
      {
        gui->close_current_popup();
      }
      gui->end_popup();
    }

    if (gui->button(ICON_MD_ADD))
    {
      new_sampler_name_ = ""_str;
      gui->open_popup("Create New Prompt Format"_str);
    }
    gui->same_line();
    if (gui->button(ICON_MD_DELETE))
    {
      store->delete_prompt_format();
    }
    gui->same_line();

    if (gui->button(ICON_MD_EDIT))
    {
      edit_sampler_.clone_from(active_sampler);
      gui->open_popup("Edit Sampler"_str);
    }
    gui->same_line();

    if (gui->begin_combo("Sampler"_str, active_sampler.name.cspan()))
    {
      for (u32 format_i = 0; format_i < samplers.size(); format_i++)
      {
        const auto& setting  = samplers[format_i];
        const b8 is_selected = setting.name == active_sampler.name;
        if (gui->selectable(setting.name.cspan(), is_selected))
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
  }

  void ChatbotSettingPanel::on_gui_render(NotNull<app::Gui*> gui, Store* store)
  {
    if (gui->begin_window("Chatbot Setting"_str, vec2f32(1400, 1040), vec2f32(20, 40)))
    {
      if (gui->is_window_appearing())
      {
        api_url_.assign(store->api_url_string_view());
        impersonate_action_prompt_.assign(store->impersonate_action_prompt_cspan());
        choice_prompt_.assign(store->choice_prompt_cspan());
      }

      if (gui->input_text("Api Url"_str, &api_url_))
      {
        store->set_api_url(api_url_.cspan());
      }

      i32 context_token_count = cast<i32>(store->get_context_token_count());
      if (gui->slider_i32("Context Token Count"_str, &context_token_count, 0, 131072))
      {
        store->set_context_token_count(context_token_count);
      }

      i32 response_token_count = cast<i32>(store->get_response_token_count());
      if (gui->slider_i32("Respone Token Count"_str, &response_token_count, 0, 8192))
      {
        store->set_response_token_count(response_token_count);
      }

      render_prompt_formatting_widget(gui, store);
      render_sampler_setting(gui, store);
      gui->separator_text("Default prompt"_str);
      gui->input_text_multiline_full_width(
        "Impersonate Action Prompt"_str, &impersonate_action_prompt_, 4 * gui->get_frame_height());
      if (gui->is_item_deactivated_after_edit())
      {
        store->set_impersonate_action_prompt(impersonate_action_prompt_.cspan());
      }
      gui->input_text_multiline_full_width(
        "Choice Prompt"_str, &choice_prompt_, 4 * gui->get_frame_height());
      if (gui->is_item_deactivated_after_edit())
      {
        store->set_choice_prompt(choice_prompt_.cspan());
      }
    }
    gui->end_window();
  }
} // namespace khaos
