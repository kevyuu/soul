#pragma once

#include "streaming_buffer.h"
#include "text_completion_backend.h"
#include "type.h"

#include "core/log.h"
#include "core/string_view.h"

using namespace soul;

namespace khaos
{
  struct TextCompletionTask
  {
    String api_url;
    String header_prompt;
    Span<const Message*> messages;
    PromptFormatParameter prompt_format_parameter;
    SamplerParameter sampler_parameter;
    String grammar_string;
    u32 max_token_count;
  };

  struct CyoaTask
  {
    String api_url;
    String header_prompt;
    Span<const Message*> messages;
    PromptFormatParameter prompt_format_parameter;
    SamplerParameter sampler_parameter;
    String grammar_string;
    u32 max_token_count;
    String cyoa_prompt;
  };

  class TextCompletionSystem
  {
  public:
    void run(TextCompletionTask&& task)
    {
      SOUL_LOG_INFO("Repetition penalty : {}", task.sampler_parameter.repetition_penalty);
      is_task_running_      = true;
      text_completion_task_ = std::move(task);
      SOUL_LOG_INFO(
        "Repetition penalty : {}",
        text_completion_task_.some_ref().sampler_parameter.repetition_penalty);
      thread_ = std::thread(
        [this]()
        {
          const auto& task = text_completion_task_.some_ref();
          String prompt;
          prompt.append(task.prompt_format_parameter.header_prefix.cspan());
          prompt.append(task.header_prompt);
          prompt.append(task.prompt_format_parameter.header_suffix.cspan());
          FlagMap<Role, StringView> prefix_map = {
            task.prompt_format_parameter.system_prefix.cspan(),
            task.prompt_format_parameter.user_prefix.cspan(),
            task.prompt_format_parameter.assistant_prefix.cspan(),
          };
          FlagMap<Role, StringView> suffix_map = {
            task.prompt_format_parameter.system_suffix.cspan(),
            task.prompt_format_parameter.user_suffix.cspan(),
            task.prompt_format_parameter.assistant_suffix.cspan(),
          };
          for (const auto& message : task.messages)
          {
            const auto prefix = prefix_map[message.role];
            const auto suffix = suffix_map[message.role];
            prompt.append(prefix);
            prompt.append(message.content);
            prompt.append(suffix);
          }
          prompt.append(task.prompt_format_parameter.assistant_prefix);
          SOUL_LOG_INFO("Prompt : {}", prompt);
          TextCompletionBackend backend;
          backend.request_streaming_completion(
            &streaming_buffer_,
            task.api_url.cspan(),
            prompt.cspan(),
            task.sampler_parameter,
            task.max_token_count,
            task.grammar_string.cspan());
          is_task_running_ = false;
        });
      thread_.detach();
    }

    auto is_any_pending_response() const -> b8
    {
      return (is_task_running_ || streaming_buffer_.size() != 0);
    }

    void consume(NotNull<String*> dst)
    {
      streaming_buffer_.consume(dst);
    }

  private:
    std::thread thread_;
    std::atomic_bool is_task_running_ = false;
    StreamingBuffer streaming_buffer_;
    Option<TextCompletionTask> text_completion_task_;
  };
} // namespace khaos
