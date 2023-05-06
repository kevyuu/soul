#pragma once

#include <format>

#include "core/cstring.h"
#include "core/flag_map.h"
#include "core/string_util.h"

namespace soul
{
  enum class LogLevel : uint8 { FATAL, WARN, ERROR, INFO, DEBUG, COUNT };
} // namespace soul

namespace soul::memory
{
  class Allocator;
}

namespace soul::impl
{
  // TODO(kevinyu) : Use cstring_view(pointer to char and length) after it is implemented
  auto log(LogLevel log_level, const CString& message) -> void;
  constexpr auto LOG_PREFIX =
    FlagMap<LogLevel, const char*>::from_val_list({"FATAL", "WARN", "ERROR", "INFO", "DEBUG"});

#if defined(SOUL_LOG_LEVEL_FATAL)
  constexpr auto LOG_LEVEL = LogLevel::FATAL;
#elif defined(SOUL_LOG_LEVEL_ERROR)
  constexpr auto LOG_LEVEL = LogLevel::ERROR;
#elif defined(SOUL_LOG_LEVEL_WARN)
  constexpr auto LOG_LEVEL = LogLevel::WARN;
#elif defined(SOUL_LOG_LEVEL_INFO)
  constexpr auto LOG_LEVEL = LogLevel::INFO;
#elif defined(SOUL_LOG_LEVEL_DEBUG)
  constexpr auto LOG_LEVEL = LogLevel::DEBUG;
#else
  constexpr auto LOG_LEVEL = LogLevel::INFO;
#endif

  template <LogLevel log_level, typename... Args>
  auto log(
    const char* const file_name,
    const soul_size line,
    std::format_string<Args...> fmt,
    Args&&... args) -> void
  {
    if (log_level <= impl::LOG_LEVEL) {
      CString message;
      appendf(
        message,
        "[{}]:{}:{}::",
        impl::LOG_PREFIX[log_level],
        relative_from_project_path(file_name),
        line);
      appendf(message, std::move(fmt), std::forward<Args>(args)...);
      impl::log(log_level, message.data());
    }
  }

  template <typename... Args>
  auto log_fatal(
    const char* const file_name,
    const soul_size line,
    std::format_string<Args...> fmt,
    Args&&... args) -> void
  {
    log<LogLevel::FATAL>(file_name, line, std::move(fmt), std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto log_warn(
    const char* const file_name,
    const soul_size line,
    std::format_string<Args...> fmt,
    Args&&... args) -> void
  {
    log<LogLevel::WARN>(file_name, line, std::move(fmt), std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto log_error(
    const char* const file_name,
    const soul_size line,
    std::format_string<Args...> fmt,
    Args&&... args) -> void
  {
    log<LogLevel::ERROR>(file_name, line, std::move(fmt), std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto log_info(
    const char* const file_name,
    const soul_size line,
    std::format_string<Args...> fmt,
    Args&&... args) -> void
  {
    log<LogLevel::INFO>(file_name, line, std::move(fmt), std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto log_debug(
    const char* const file_name,
    const soul_size line,
    std::format_string<Args...> fmt,
    Args&&... args) -> void
  {
    log<LogLevel::DEBUG>(file_name, line, std::move(fmt), std::forward<Args>(args)...);
  }

  auto flush_log(LogLevel log_level) -> void;
  auto flush_logs() -> void;
} // namespace soul::impl

#define SOUL_LOG_DEBUG(...) soul::impl::log_debug(__FILE__, __LINE__, ##__VA_ARGS__)
#define SOUL_LOG_INFO(...) soul::impl::log_info(__FILE__, __LINE__, ##__VA_ARGS__)
#define SOUL_LOG_WARN(...) soul::impl::log_warn(__FILE__, __LINE__, ##__VA_ARGS__)
#define SOUL_LOG_ERROR(...) soul::impl::log_error(__FILE__, __LINE__, ##__VA_ARGS__)
#define SOUL_LOG_FATAL(...) soul::impl::log_fatal(__FILE__, __LINE__, ##__VA_ARGS__)
#define SOUL_FLUSH_LOG(log_level) soul::impl::flush_log(log_level)
#define SOUL_FLUSH_LOGS() soul::impl::flush_logs()
