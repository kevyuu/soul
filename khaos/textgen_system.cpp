#include "textgen_system.h"
#include "textgen_backend.h"

namespace khaos
{
  void TextgenSystem::on_new_frame()
  {
    if (is_task_running_)
    {
      return;
    }

    if (active_task_.is_some())
    {
      active_task_.some_ref().callback(streaming_buffer_.cview());
      streaming_buffer_.clear();
      active_task_ = nilopt;
    }

    if (textgen_task_queue_.empty())
    {
      return;
    }

    is_task_running_ = true;
    active_task_     = textgen_task_queue_.pop_back();
    thread_          = std::thread(
      [this]()
      {
        const auto& task = active_task_.some_ref();
        String prompt;
        prompt.append(task.prompt_format_parameter.header_prefix.cview());
        prompt.append(task.header_prompt);
        prompt.append(task.prompt_format_parameter.header_suffix.cview());
        FlagMap<Role, StringView> prefix_map = {
          task.prompt_format_parameter.system_prefix.cview(),
          task.prompt_format_parameter.user_prefix.cview(),
          task.prompt_format_parameter.assistant_prefix.cview(),
        };
        FlagMap<Role, StringView> suffix_map = {
          task.prompt_format_parameter.system_suffix.cview(),
          task.prompt_format_parameter.user_suffix.cview(),
          task.prompt_format_parameter.assistant_suffix.cview(),
        };
        for (const auto& message : task.messages)
        {
          const auto prefix = prefix_map[message.role];
          const auto suffix = suffix_map[message.role];
          prompt.append(prefix);
          if (message.label.size() > 0)
          {
            prompt.append(message.label);
            prompt.append(" : ");
          }
          prompt.append(message.content);
          prompt.append(suffix);
        }
        prompt.append(task.prompt_format_parameter.assistant_prefix);
        TextgenBackend backend;
        backend.request_streaming_completion(
          &streaming_buffer_,
          task.api_url.cview(),
          prompt.cview(),
          task.sampler_parameter,
          task.max_token_count,
          task.grammar_string.cview());
        is_task_running_ = false;
      });
    thread_.detach();
  }
  
  void TextgenSystem::push_task(OwnRef<TextgenTask> task)
  {
    textgen_task_queue_.push_back(std::move(task)); 
  }

  auto TextgenSystem::is_any_pending_response() const -> b8
  {
    return (is_task_running_ || !textgen_task_queue_.empty());
  }

  auto TextgenSystem::streaming_buffer_cview() const -> StringView
  {
    return streaming_buffer_.cview();
  }

  void TextgenSystem::consume(NotNull<String*> dst)
  {
    streaming_buffer_.consume(dst);
  }

} // namespace khaos
