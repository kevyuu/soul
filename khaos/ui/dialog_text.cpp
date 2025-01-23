#include "dialog_text.h"

#include "app/icons.h"

#include "misc/string_util.h"

namespace khaos
{
  namespace
  {
    void dialog_line(NotNull<app::Gui*> gui, StringView text)
    {
      if (text.size() == 0)
      {
        return;
      }
      for (usize char_i = 0; char_i < text.size();)
      {
        const auto current_char = text[char_i];
        if (current_char == '"')
        {
          auto result = str::find_char(text, '"', char_i + 1);
          usize end_i = result.unwrap_or(text.size() - 1);
          gui->push_style_color(app::Gui::ColorVar::TEXT, vec4f32(0.9, 0.5, 0.3, 1.0));
          gui->subtext_wrapped(StringView(text.begin() + char_i, end_i - char_i + 1));
          gui->pop_style_color();
          char_i = end_i + 1;
        } else if (current_char == '*')
        {
          auto result = str::find_char(text, '*', char_i + 1);
          usize end_i = result.unwrap_or(text.size() - 1);
          gui->push_style_color(app::Gui::ColorVar::TEXT, vec4f32(0.7, 0.7, 0.7, 1.0));
          gui->subtext_wrapped(StringView(text.begin() + char_i + 1, end_i - char_i - 1));
          gui->pop_style_color();
          char_i = end_i + 1;
        } else
        {
          static const auto special_characters = Array{'*', '"'};
          auto result = str::find_char(text, special_characters.cspan(), char_i + 1);
          usize end_i = result.unwrap_or(text.size());
          gui->subtext_wrapped(StringView(text.begin() + char_i, end_i - char_i));
          char_i = end_i;
        }
      }
    }

    void choice_line(NotNull<app::Gui*> gui, usize choice_index, StringView text)
    {
      gui->push_id(choice_index);
      gui->frameless_button(ICON_MD_INPUT);
      gui->same_line();
      gui->text(text);
      gui->pop_id();
    }
  } // namespace

  void dialog_text(NotNull<app::Gui*> gui, StringView text)
  {
    auto choice_index = 0;
    b8 first_line     = true;
    str::for_each_line(
      text,
      [gui, &choice_index, &first_line](usize line_i, StringView line)
      {
        const auto choice_prefix = "<choice>"_str;
        const auto choice_suffix = "</choice>"_str;
        if (str::starts_with(line, choice_prefix))
        {
          choice_line(
            gui,
            choice_index,
            StringView(
              line.data() + choice_prefix.size(),
              line.size() - choice_prefix.size() - choice_suffix.size()));
          choice_index++;
        } else
        {
          if (line.size() != 0)
          {
            if (!first_line)
            {
              gui->new_line();
            } else
            {
              first_line = false;
            }
            dialog_line(gui, line);
          }
        }
      });
  }
} // namespace khaos
