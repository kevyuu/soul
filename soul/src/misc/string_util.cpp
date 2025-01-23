#include "core/string_view.h"

#include "misc/string_util.h"

#include <string_view>

namespace soul::str
{
  namespace
  {
    auto is_whitespace_character(char c) -> b8
    {
      return c == ' ' || c == '\t' || c == '\n';
    }

    auto to_std(StringView str_view) -> std::string_view
    {
      return std::string_view(str_view.data(), str_view.size());
    }
  } // namespace

  auto trim_start(StringView str_view) -> StringView
  {
    for (usize char_i = 0; char_i < str_view.size(); char_i++)
    {
      if (!is_whitespace_character(str_view[char_i]))
      {
        return StringView(str_view.data() + char_i, str_view.size() - char_i);
      }
    }
    return nilspan;
  }

  auto trim_end(StringView str_view) -> StringView
  {
    const auto size = str_view.size();
    for (usize char_i = 0; char_i < size; char_i++)
    {
      if (!is_whitespace_character(str_view[size - char_i - 1]))
      {
        return StringView(str_view.data(), size - char_i);
      }
    }
    return nilspan;
  }

  auto trim(StringView str_view) -> StringView
  {
    return trim_end(trim_start(str_view));
  }

  auto find_char(StringView str_view, char c, usize offset) -> Option<usize>
  {
    for (usize char_i = offset; char_i < str_view.size(); char_i++)
    {
      if (str_view[char_i] == c)
      {
        return char_i;
      }
    }
    return nilopt;
  }

  auto find_char(StringView str_view, Span<const char*, usize> char_list, usize offset)
    -> Option<usize>
  {
    for (usize char_i = offset; char_i < str_view.size(); char_i++)
    {
      for (const auto char_to_seek : char_list)
      {
        if (str_view[char_i] == char_to_seek)
        {
          return char_i;
        }
      }
    }
    return nilopt;
  }

  auto starts_with(StringView text, StringView sub_text) -> b8
  {
    if (text.size() < sub_text.size())
    {
      return false;
    }
    return StringView(text.data(), sub_text.size()) == sub_text;
  }

  auto ends_with(StringView text, StringView sub_text) -> b8
  {
    if (text.size() < sub_text.size())
    {
      return false;
    }
    return StringView(text.end() - sub_text.size(), sub_text.size()) == sub_text;
  }

  auto into_c_str(StringView str_view, NotNull<String*> str) -> const char*
  {
    if (str_view.is_null_terminated())
    {
      return str_view.data();
    } else
    {
      str->assign(str_view);
      return str->c_str();
    }
  }

  auto replace_char(
    StringView text, char from_char, char to_char, NotNull<memory::Allocator*> allocator) -> String
  {
    auto result = String::WithCapacity(text.size() + 1, allocator);

    for (i32 char_i = 0; char_i < text.size(); char_i++)
    {
      if (text[char_i] == from_char)
      {
        result.push_back(to_char);
      } else
      {
        result.push_back(text[char_i]);
      }
    }

    return result;
  }

  auto replace_substr(
    StringView text,
    StringView from_substr,
    StringView to_substr,
    NotNull<memory::Allocator*> allocator) -> String
  {
    const auto std_str_view = to_std(text);
    String result(allocator);
    u64 find_start_pos = 0;
    u64 find_pos       = std_str_view.find(to_std(from_substr), find_start_pos);
    while (find_pos != std::string_view::npos)
    {
      result.append(StringView(text.data() + find_start_pos, find_pos - find_start_pos));
      result.append(to_substr);
      find_start_pos = find_pos + from_substr.size();
      find_pos       = std_str_view.find(to_std(from_substr), find_start_pos);
    }
    result.append(StringView(text.data() + find_start_pos, text.size() - find_start_pos));
    return result;
  }

} // namespace soul::str
