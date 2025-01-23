#include "hf_tokenizer.h"

#include "core/string_view.h"
#include "tokenizers_c.h"

namespace khaos
{
  HFTokenizer::HFTokenizer(StringView json_str)
  {
    handle_ = tokenizers_new_from_str(json_str.data(), json_str.size());
  }

  HFTokenizer::~HFTokenizer()
  {
    tokenizers_free(handle_);
  }

  auto HFTokenizer::get_token_count(StringView text, b8 add_special_tokens) const -> u64
  {
    TokenizerEncodeResult result;
    tokenizers_encode(
      handle_, text.data(), text.size(), static_cast<int>(add_special_tokens), &result);
    const u64 token_count = result.len;
    tokenizers_free_encode_results(&result, 1);
    return token_count;
  }

} // namespace khaos
