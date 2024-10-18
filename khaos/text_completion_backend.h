#pragma once

#include "streaming_buffer.h"
#include "type.h"

typedef void CURL; // NOLINT

using namespace soul;

namespace khaos
{
  class TextCompletionBackend
  {
  public:
    TextCompletionBackend();
    ~TextCompletionBackend();
    TextCompletionBackend(const TextCompletionBackend&)                    = delete;
    TextCompletionBackend(TextCompletionBackend&&)                         = default;
    auto operator=(const TextCompletionBackend&) -> TextCompletionBackend& = delete;
    auto operator=(TextCompletionBackend&&) -> TextCompletionBackend&      = default;

    void request_streaming_completion(
      StreamingBuffer* buffer,
      StringView url,
      StringView prompt,
      const SamplerParameter& parameter,
      u32 max_token_count,
      StringView grammar_string);

  private:
    CURL* handle_;
  };
} // namespace khaos
