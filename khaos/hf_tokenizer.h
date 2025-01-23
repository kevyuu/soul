#pragma once

#include "core/string_view.h"
#include "core/type.h"
#include "core/vector.h"

#include "type.h"

#include <tokenizers_c.h>

using namespace soul;

namespace khaos
{
  class HFTokenizer
  {
  public:

    explicit HFTokenizer(StringView json_str);
    HFTokenizer(const HFTokenizer&)                    = delete;
    HFTokenizer(HFTokenizer&&)                         = delete;
    auto operator=(const HFTokenizer&) -> HFTokenizer& = delete;
    auto operator=(HFTokenizer&&) -> HFTokenizer&      = delete;
    ~HFTokenizer();

    auto encode(StringView text, b8 add_special_tokens = false) const -> Vector<i32>;

    auto get_token_count(StringView text, b8 add_special_tokens = false) const -> u64;

  private:
    TokenizerHandle handle_;
  };
} // namespace khaos
