#include "panels/chatbot_setting_panel.h"
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
      gui->input_text("Header Prefix"_str, &edit_prompt_format_.header_prefix);
      gui->input_text("Header Suffix"_str, &edit_prompt_format_.header_suffix);
      gui->input_text("User Prefix"_str, &edit_prompt_format_.user_prefix);
      gui->input_text("User Suffix"_str, &edit_prompt_format_.user_suffix);
      gui->input_text("Assistant Prefix"_str, &edit_prompt_format_.assistant_prefix);
      gui->input_text("Assistant Suffix"_str, &edit_prompt_format_.assistant_suffix);
      gui->input_text("System Prefix"_str, &edit_prompt_format_.system_prefix);
      gui->input_text("System Suffix"_str, &edit_prompt_format_.system_suffix);

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

    if (gui->begin_popup_modal("Edit Sampler"_str))
    {
      gui->input_text("Name"_str, &edit_sampler_.name);
      gui->slider_f32("Temperature"_str, &edit_sampler_.temperature, 0, 5);
      gui->slider_f32("Top P"_str, &edit_sampler_.top_p, 0, 1);
      gui->slider_f32("Min P"_str, &edit_sampler_.min_p, 0, 1);
      gui->slider_i32("Top K"_str, &edit_sampler_.top_k, -1, 200);
      gui->slider_f32("Min P"_str, &edit_sampler_.min_p, 0, 1);
      gui->slider_f32("Repetition Penalty"_str, &edit_sampler_.repetition_penalty, 0, 1);
      gui->slider_f32("Presence Penalty"_str, &edit_sampler_.presence_penalty, 0, 1);
      gui->slider_f32("Frequency Penalty"_str, &edit_sampler_.frequency_penalty, 0, 1);
      gui->slider_i32(
        "Repetition Penalty Range"_str, &edit_sampler_.repetition_penalty_range, 0, 64000);
      gui->slider_f32("Typcial P"_str, &edit_sampler_.typical_p, 0, 1);
      gui->slider_f32("TFS"_str, &edit_sampler_.tfs, 0, 1);
      gui->slider_f32("Top A"_str, &edit_sampler_.top_a, 0, 1);
      gui->slider_f32("Epsilon Cutoff"_str, &edit_sampler_.epsilon_cutoff, 0, 1);
      gui->slider_f32("Eta Cutoff"_str, &edit_sampler_.eta_cutoff, 0, 1);
      gui->slider_f32(
        "Encoder Repetition Penalty"_str, &edit_sampler_.encoder_repetition_penalty, 0, 1);
      gui->slider_i32(
        "No Repetition Ngram Size"_str, &edit_sampler_.no_repeat_ngram_size, 0, 64000);
      gui->slider_f32("Smoothing Factor"_str, &edit_sampler_.smoothing_factor, 0, 1);
      gui->slider_f32("Smoothing Curve"_str, &edit_sampler_.smoothing_curve, 0, 1);
      gui->slider_f32("DRY Multiplier"_str, &edit_sampler_.dry_multipler, 0, 1);
      gui->slider_f32("DRY Base"_str, &edit_sampler_.dry_base, 0, 1);
      gui->slider_i32("DRY Allowed Length"_str, &edit_sampler_.no_repeat_ngram_size, 0, 64000);
      gui->checkbox("Dynamic Temperature"_str, &edit_sampler_.dynamic_temperature);
      gui->slider_f32("Min Temperature"_str, &edit_sampler_.dynatemp_low, 0, 1);
      gui->slider_f32("Max Temperature"_str, &edit_sampler_.dynatemp_high, 0, 1);
      gui->slider_f32("Exponent"_str, &edit_sampler_.dynatemp_exponent, 0, 1);
      gui->slider_i32("Mirostat Mode"_str, &edit_sampler_.mirostat_mode, 0, 64000);
      gui->slider_f32("Mirostat Tau"_str, &edit_sampler_.mirostat_tau, 0, 1);
      gui->slider_f32("Mirostat Eta"_str, &edit_sampler_.mirostat_eta, 0, 1);
      gui->slider_f32("Penalty Alpha"_str, &edit_sampler_.penalty_alpha, 0, 1);
      gui->checkbox("Do Sample"_str, &edit_sampler_.do_sample);
      gui->checkbox("Add BOS Token"_str, &edit_sampler_.add_bos_token);
      gui->checkbox("Ban EOS Token"_str, &edit_sampler_.ban_eos_token);
      gui->checkbox("Skip Special Tokens"_str, &edit_sampler_.skip_special_tokens);
      gui->checkbox("Temperature Last"_str, &edit_sampler_.temperature_last);

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
    if (gui->begin_window(
          "Chatbot Setting"_str,
          vec2f32(1400, 1040),
          vec2f32(20, 40),
          {
            app::Gui::WindowFlag::SHOW_TITLE_BAR,
            app::Gui::WindowFlag::ALLOW_MOVE,
            app::Gui::WindowFlag::NO_SCROLLBAR,
          }))
    {

      gui->push_item_width(200);
      api_url_.assign(store->api_url_string_view());
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
      gui->pop_item_width();
    }
    gui->end_window();
  }
} // namespace khaos
