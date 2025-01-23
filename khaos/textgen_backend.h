#pragma once

#include "streaming_buffer.h"
#include "type.h"

typedef void CURL; // NOLINT

using namespace soul;

namespace khaos
{
  class TextgenBackend
  {
  public:
    TextgenBackend();
    ~TextgenBackend();
    TextgenBackend(const TextgenBackend&)                    = delete;
    TextgenBackend(TextgenBackend&&)                         = default;
    auto operator=(const TextgenBackend&) -> TextgenBackend& = delete;
    auto operator=(TextgenBackend&&) -> TextgenBackend&      = default;

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
