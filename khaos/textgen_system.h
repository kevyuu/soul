#pragma once

#include "core/function.h"
#include "core/deque.h"

#include "streaming_buffer.h"
#include "type.h"

using namespace soul;

namespace khaos
{
  struct TextgenTask
  {
    String header_prompt;
    Vector<Message> messages;
    String api_url;
    PromptFormatParameter prompt_format_parameter;
    SamplerParameter sampler_parameter;
    String grammar_string;
    u32 max_token_count;
    TokenizerType tokenizer_type;
    Function<void(StringView)> callback;
  };

  class TextgenSystem
  {
  public:
    void on_new_frame();

    void push_task(OwnRef<TextgenTask> task);

    auto is_any_pending_response() const -> b8;

    auto streaming_buffer_cview() const -> StringView;

    void consume(NotNull<String*> dst);

  private:
    std::thread thread_;
    std::atomic_bool is_task_running_ = false;
    StreamingBuffer streaming_buffer_;
    Deque<TextgenTask> textgen_task_queue_;
    Option<TextgenTask> active_task_;
  };
} // namespace khaos
