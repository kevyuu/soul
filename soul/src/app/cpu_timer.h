#pragma once

#include <chrono>

namespace soul::app
{
  class CpuTimer
  {
  public:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
    using Duration  = std::chrono::duration<double>;

  private:
    TimePoint current_time_;
    Duration elapsed_time_ = Duration::zero();

  public:
    static auto get_current_timepoint() -> TimePoint
    {
      return std::chrono::high_resolution_clock::now();
    }

    void tick()
    {
      auto now = get_current_timepoint();
      if (current_time_ == TimePoint())
      {
        current_time_ = now;
        elapsed_time_ = Duration::zero();
      } else
      {
        elapsed_time_ = now - current_time_;
        current_time_ = now;
      }
    }

    [[nodiscard]]
    auto delta_in_seconds() const -> double
    {
      return elapsed_time_.count();
    }
  };
} // namespace soul::app
