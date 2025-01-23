#pragma once

#include "core/config.h"
#include "core/option.h"
#include "core/string.h"
#include "core/string_view.h"

namespace soul::str
{
  auto trim_start(StringView str_view) -> StringView;
  auto trim_end(StringView str_view) -> StringView;
  auto trim(StringView str_view) -> StringView;
  auto find_char(StringView str_view, char c, usize offset = 0) -> Option<usize>;
  auto find_char(StringView str_view, Span<const char*, usize> char_list, usize offset = 0)
    -> Option<usize>;
  auto starts_with(StringView text, StringView sub_text) -> b8;
  auto ends_with(StringView text, StringView sub_text) -> b8;
  auto into_c_str(StringView str_view, NotNull<String*> str) -> const char*;

  template <typename Fn>
  void for_each_line(StringView text, Fn fn)
  {
    usize start_cursor = 0;
    usize line_i       = 0;
    for (usize char_i = 0; char_i < text.size(); char_i++)
    {
      const char c = text[char_i];
      if (c == '\n')
      {
        fn(line_i, StringView(text.data() + start_cursor, char_i - start_cursor));
        line_i++;
        start_cursor = char_i + 1;
      }
    }
    if (start_cursor < text.size())
    {
      fn(line_i, StringView(text.data() + start_cursor, text.size() - start_cursor));
    }
  }

  auto replace_char(
    StringView text,
    char from,
    char to,
    NotNull<memory::Allocator*> allocator = get_default_allocator()) -> String;

  auto replace_substr(
    StringView text,
    StringView from,
    StringView to,
    NotNull<memory::Allocator*> allocator = get_default_allocator()) -> String;
} // namespace soul::str
